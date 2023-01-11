#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call)                                                      \
  {                                                                            \
    BF_ErrorCode code = call;                                                  \
    if (code != BF_OK) {                                                       \
      BF_PrintError(code);                                                     \
      exit(code);                                                              \
    }                                                                          \
  }

int HT_CreateFile(char *fileName, int buckets) {

  CALL_OR_DIE(BF_CreateFile(fileName));
  int file_desc;
  CALL_OR_DIE(BF_OpenFile(fileName, &file_desc));

  BF_Block *block;
  BF_Block_Init(&block);

  void *data;
  CALL_OR_DIE(BF_AllocateBlock(file_desc, block));
  data = BF_Block_GetData(block);

  /* Αρχικοποίηση block_info. Πρόκειται για το πρώτο block, άρα το block 0 το
   * οποίο έχει 0 records. Επίσης, εφόσον είναι το μόνο block αρχικά, δεν
   * υπάρχει επόμενο block */
  int block_info_offset = MAX_RECS * sizeof(Record);
  HT_block_info block_info;
  block_info.blockDesc = 0;
  block_info.prevBlockDesc = -1;
  memcpy(data + block_info_offset, &block_info, sizeof(block_info));

  HT_info info;
  info.fileDesc = file_desc;
  info.numBuckets = buckets;
  info.isHT = 1;
  info.lastBlockDesc = 0;
  memcpy(data, &info, sizeof(info));

  BF_Block_SetDirty(block);

  CALL_OR_DIE(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);

  CALL_OR_DIE(BF_CloseFile(file_desc));

  return 0;
}

HT_info *HT_OpenFile(char *fileName) {

  /* Δεν χρησιμοποιείται η CALL_OR_DIE διότι η συνάρτηση σε περίπτωση λάθους
   * πρέπει να επιστρέφει NULL, όχι int */

  int file_desc;
  BF_OpenFile(fileName, &file_desc);

  /* Πρόσβαση στο block 0 */
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(file_desc, 0, block);
  void *data = BF_Block_GetData(block);

  /* Πρόσβαση στο HT_info του block 0 */
  HT_info *ht_info = malloc(sizeof(HT_info));
  memcpy(ht_info, data, sizeof(HT_info));

  /* έλεγχος για αρχείο κατακερματισμού */
  if (!ht_info->isHT)
    return NULL;

  /* Αρχικοποίηση hashtable στην μνήμη */
  ht_info->hashtable = malloc(sizeof(int) * ht_info->numBuckets);
  for (int bucket = 0; bucket < ht_info->numBuckets; bucket++)
    ht_info->hashtable[bucket] = -1;

  /* Το file_desc του info αλλάζει από την κλήση της BF_OpenFile, άρα χρειάζεται
   * να ενημερώσουμε την τιμή στο HT_info  */
  ht_info->fileDesc = file_desc;
  /* Ενημέρωση του HT_info του block 0 */
  memcpy(data, ht_info, sizeof(HT_info));

  BF_Block_SetDirty(block);

  BF_UnpinBlock(block);

  BF_Block_Destroy(&block);

  return ht_info;
}

int HT_CloseFile(HT_info *ht_info) {

  /* Βρίσκουμε πόσα blocks έχει το αρχείο */
  int file_desc = ht_info->fileDesc;
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
  free(ht_info->hashtable); /* TODO: test if it is necessary */
  free(ht_info);

  return 0;
}

int HT_InsertEntry(HT_info *ht_info, Record record) {

  int blockId;

  /* Συνάρτηση κατακερματισμού */
  int bucket = record.id % ht_info->numBuckets;

  /* Έλεγχος για την περίπτωση όπου ο κάδος δεν έχει ακόμα blocks */
  if (ht_info->hashtable[bucket] == -1) {

    BF_Block *new_block;
    BF_Block_Init(&new_block);
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, new_block));

    ht_info->lastBlockDesc++;
    ht_info->hashtable[bucket] = ht_info->lastBlockDesc;

    /* Αρχικοποίηση block_info */
    void *data = BF_Block_GetData(new_block);
    HT_block_info block_info;
    block_info.blockDesc = ht_info->lastBlockDesc;
    block_info.prevBlockDesc = -1;
    block_info.recsNum = 0;
    int block_info_offset = MAX_RECS * sizeof(Record);
    memcpy(data + block_info_offset, &block_info, sizeof(block_info));

    BF_Block_SetDirty(new_block);
    CALL_OR_DIE(BF_UnpinBlock(new_block));
    BF_Block_Destroy(&new_block);
  }

  /* Δημιουργία block */
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_OR_DIE(
      BF_GetBlock(ht_info->fileDesc, ht_info->hashtable[bucket], block));

  /* Πρόσβαση στο block_info του τελευταίου block του κάδου */
  HT_block_info block_info;
  void *data = BF_Block_GetData(block);
  memcpy(&block_info, data + MAX_RECS * sizeof(Record), sizeof(HT_block_info));
  CALL_OR_DIE(BF_UnpinBlock(block));

  /* Έλεγχος διαθέσιμου χώρου */
  int recs_size = block_info.recsNum * sizeof(Record);
  int space_left_in_block = BF_BLOCK_SIZE - (recs_size + sizeof(HT_block_info));

  /* Αν υπάρχει διαθέσιμος χώρος, γράψε στο πιο πρόσφατο block το record, αλλιώς
   * φτιάξε καινούργιο block στον κάδο */
  if (space_left_in_block >= sizeof(Record) && block_info.blockDesc != 0) {

    /* Εγγραφή record */
    memcpy(data + recs_size, &record, sizeof(Record));

    /* Ενημέρωση block_info */
    block_info.recsNum++;
    memcpy(data + MAX_RECS * sizeof(Record), &block_info,
           sizeof(HT_block_info));

    blockId = block_info.blockDesc;
  } else {

    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));

    /* Εισαγωγή record */
    data = BF_Block_GetData(block);
    memcpy(data, &record, sizeof(Record));

    /* Ενημέρωση block_info καινούργιου block */
    block_info.recsNum = 1;
    block_info.blockDesc = ht_info->lastBlockDesc + 1;
    block_info.prevBlockDesc = ht_info->hashtable[bucket];

    /* Ενημέρωση ht_info */
    ht_info->lastBlockDesc = block_info.blockDesc;
    ht_info->hashtable[bucket] = block_info.blockDesc;

    memcpy(data + MAX_RECS * sizeof(Record), &block_info,
           sizeof(HT_block_info));

    blockId = block_info.blockDesc;

    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return blockId;
}

int HT_GetAllEntries(HT_info *ht_info, void *value) { return 0; }
