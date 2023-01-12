#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"
#include "sht_table.h"

#define CALL_OR_DIE(call)                                                      \
  {                                                                            \
    BF_ErrorCode code = call;                                                  \
    if (code != BF_OK) {                                                       \
      BF_PrintError(code);                                                     \
      exit(code);                                                              \
    }                                                                          \
  }

int SHT_Hash(char *name, int buckets) {

  /* Error handling */
  if (name[0] == '\0')
    return -1;

  int temp = 0;

  for (int i = 0; i < 15; i++) {

	if (name[i] == '\0')
	  break;

    temp += name[i] % buckets;
  }

  return temp % buckets;
}

int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char *fileName) {

  CALL_OR_DIE(BF_CreateFile(sfileName));
  int file_desc;
  CALL_OR_DIE(BF_OpenFile(sfileName, &file_desc));

  BF_Block *block;
  BF_Block_Init(&block);

  void *data;
  CALL_OR_DIE(BF_AllocateBlock(file_desc, block));
  data = BF_Block_GetData(block);

  /* Αρχικοποίηση block_info. Πρόκειται για το πρώτο block, άρα το block 0 το
   * οποίο έχει 0 records. Επίσης, εφόσον είναι το μόνο block αρχικά, δεν
   * υπάρχει επόμενο block */
  int block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);
  SHT_block_info block_info;
  block_info.blockDesc = 0;
  block_info.prevBlockDesc = -1;
  memcpy(data + block_info_offset, &block_info, sizeof(SHT_block_info));

  SHT_info info;
  info.fileDesc = file_desc;
  info.numBuckets = buckets;
  info.isHT = 1;
  info.lastBlockDesc = 0;
  memcpy(data, &info, sizeof(SHT_info));

  BF_Block_SetDirty(block);

  CALL_OR_DIE(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);

  CALL_OR_DIE(BF_CloseFile(file_desc));

  return 0;
}

SHT_info *SHT_OpenSecondaryIndex(char *indexName) {

  /* Δεν χρησιμοποιείται η CALL_OR_DIE διότι η συνάρτηση σε περίπτωση λάθους
   * πρέπει να επιστρέφει NULL, όχι int */

  int file_desc;
  BF_OpenFile(indexName, &file_desc);

  /* Πρόσβαση στο block 0 */
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(file_desc, 0, block);
  void *data = BF_Block_GetData(block);

  /* Πρόσβαση στο SHT_info του block 0 */
  SHT_info *sht_info = malloc(sizeof(SHT_info));
  memcpy(sht_info, data, sizeof(SHT_info));

  /* έλεγχος για αρχείο κατακερματισμού */
  if (!sht_info->isHT)
    return NULL;

  /* Αρχικοποίηση sht_hashtable στην μνήμη */
  sht_info->sht_hashtable = malloc(sizeof(int) * sht_info->numBuckets);
  for (int bucket = 0; bucket < sht_info->numBuckets; bucket++)
    sht_info->sht_hashtable[bucket] = -1;

  /* Το file_desc του info αλλάζει από την κλήση της BF_OpenFile, άρα χρειάζεται
   * να ενημερώσουμε την τιμή στο SHT_info  */
  sht_info->fileDesc = file_desc;
  /* Ενημέρωση του SHT_info του block 0 */
  memcpy(data, sht_info, sizeof(SHT_info));

  BF_Block_SetDirty(block);

  BF_UnpinBlock(block);

  BF_Block_Destroy(&block);

  return sht_info;
}

int SHT_CloseSecondaryIndex(SHT_info *sht_info) {

  /* Βρίσκουμε πόσα blocks έχει το αρχείο */
  int file_desc = sht_info->fileDesc;
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(file_desc, &blocks_num));

  BF_Block *block;
  BF_Block_Init(&block);

  /* Κάνουμε unpin όλα τα blocks */
  for (int block_number = 0; block_number < blocks_num; block_number++) {

    CALL_OR_DIE(BF_GetBlock(file_desc, block_number, block));
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);

  /* κλείσιμο αρχείου */
  CALL_OR_DIE(BF_CloseFile(file_desc));
  free(sht_info->sht_hashtable);
  free(sht_info);

  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info *sht_info, Record record, int block_id) {

  /* Δημιουργία ζευγαριού νέας εγγραφής */
  SHT_Record sht_record;
  strcpy(sht_record.name, record.name);
  sht_record.blockDesc = block_id;

  /* Εύρεση κάδου δευτερεύοντος ευρετηρίου */
  int bucket;
  bucket = SHT_Hash(record.name, sht_info->numBuckets);
  if (bucket == -1)
    return -1;

  /* Έλεγχος για την περίπτωση όπου ο κάδος δεν έχει ακόμα blocks */
  if (sht_info->sht_hashtable[bucket] == -1) {

    BF_Block *new_block;
    BF_Block_Init(&new_block);
    CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, new_block));

    sht_info->lastBlockDesc++;
    sht_info->sht_hashtable[bucket] = sht_info->lastBlockDesc;

    /* Αρχικοποίηση block_info */
    void *data = BF_Block_GetData(new_block);
    SHT_block_info block_info;
    block_info.blockDesc = sht_info->lastBlockDesc;
    block_info.prevBlockDesc = -1;
    block_info.recsNum = 0;
    int block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);
    memcpy(data + block_info_offset, &block_info, sizeof(SHT_block_info));

    BF_Block_SetDirty(new_block);
    CALL_OR_DIE(BF_UnpinBlock(new_block));
    BF_Block_Destroy(&new_block);
  }

  /* Δημιουργία block */
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_OR_DIE(
      BF_GetBlock(sht_info->fileDesc, sht_info->sht_hashtable[bucket], block));

  /* Πρόσβαση στο block_info του τελευταίου block του κάδου */
  SHT_block_info block_info;
  void *data = BF_Block_GetData(block);
  memcpy(&block_info, data + SHT_MAX_RECS * sizeof(SHT_Record),
         sizeof(SHT_block_info));
  CALL_OR_DIE(BF_UnpinBlock(block));

  /* Έλεγχος διαθέσιμου χώρου */
  int recs_size = block_info.recsNum * sizeof(SHT_Record);
  int space_left_in_block =
      BF_BLOCK_SIZE - (recs_size + sizeof(SHT_block_info));

  /* Αν υπάρχει διαθέσιμος χώρος, γράψε στο πιο πρόσφατο block το sht_record,
   * αλλιώς φτιάξε καινούργιο block στον κάδο */
  if (space_left_in_block >= sizeof(SHT_Record)) {

    /* Εγγραφή sht_record */
    memcpy(data + recs_size, &sht_record, sizeof(SHT_Record));

    /* Ενημέρωση block_info */
    block_info.recsNum++;
    memcpy(data + SHT_MAX_RECS * sizeof(SHT_Record), &block_info,
           sizeof(SHT_block_info));
  } else {

    CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, block));

    /* Εισαγωγή sht_record */
    data = BF_Block_GetData(block);
    memcpy(data, &sht_record, sizeof(SHT_Record));

    /* Ενημέρωση block_info καινούργιου block */
    block_info.recsNum = 1;
    block_info.blockDesc = sht_info->lastBlockDesc + 1;
    block_info.prevBlockDesc = sht_info->sht_hashtable[bucket];

    /* Ενημέρωση sht_info */
    sht_info->lastBlockDesc = block_info.blockDesc;
    sht_info->sht_hashtable[bucket] = block_info.blockDesc;

    memcpy(data + MAX_RECS * sizeof(SHT_Record), &block_info,
           sizeof(SHT_block_info));

    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return 0;
}

int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info,
                               char *name) {

  /* Αρχικοποιούμε μια μεταβλητή όπου θα κρατάμε τον αριθμό των blocks που
   * διαβάστηκαν σε -1. Αν δεν αλλάξει κατά την εκτέλεση της συνάρτησης,
   * σημαίνει υπήρξε λάθος στην εκτέλεση και θα επιστραφεί -1. */
  int read_blocks = -1;
  int file_desc = sht_info->fileDesc;

  /* Έυρεση του bucket στον οποίο (πιθανώς να) περιέχονται τα στοιχεία με όνομα == name */
  int bucket = SHT_Hash(name, sht_info->numBuckets);

  BF_Block *block;
  BF_Block_Init(&block);

  SHT_block_info block_info;

  void *data;
  int sht_block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);

  int block_to_read = sht_info->sht_hashtable[bucket];

  int blocks_that_were_read = 0;

  /* Προσπέλαση των blocks του bucket */
  while (block_to_read != -1) {

    blocks_that_were_read++;

    /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το SHT_block_info,
     * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
     * αυτό. */
    CALL_OR_DIE(BF_GetBlock(file_desc, block_to_read, block));
    data = BF_Block_GetData(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

    memcpy(&block_info, data + sht_block_info_offset, sizeof(SHT_block_info));

    SHT_Record *sht_recs = data;

    /* Για κάθε εγγραφή του block, ελέγχουμε αν το name της ταυτίζεται με το
     * δοθέν name. Αν η σύγκριση είναι αληθής τυπώνουμε την εγγραφή αυτή */
    for (int sht_records = 0; sht_records < block_info.recsNum; sht_records++) {

      if (!strcmp(sht_recs[sht_records].name, name)) {

		blocks_that_were_read++;

		/* Προσπέλαση block του data.db όπου βρίσκεται το record με όνομα name */
		BF_Block *data_block;
		BF_Block_Init(&data_block);

		CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, sht_recs[sht_records].blockDesc, data_block));
		data = BF_Block_GetData(data_block);
		CALL_OR_DIE(BF_UnpinBlock(data_block));
		
		Record *recs = data;
		HT_block_info data_block_info;
		int block_info_offset = MAX_RECS * sizeof(Record);
		memcpy(&data_block_info, data + block_info_offset, sizeof(HT_block_info));

		for (int records = 0; records < data_block_info.recsNum; records++) {
		  
		  if (!(strcmp(recs[records].name, name))) {
			printRecord(recs[records]);
		  }
		}
		BF_Block_Destroy(&data_block);
      }
    }
    block_to_read = block_info.prevBlockDesc;
	read_blocks = blocks_that_were_read;
  }

  BF_Block_Destroy(&block);

  return read_blocks;
}
