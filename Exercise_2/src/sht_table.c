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
  if (*name == '\0') {
    printf("NULL string to hash. Exiting...\n");
    return -1;
  }

  int temp = 0;

  while (*name != '\0') {
    temp += *name % buckets;
    name++;
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

}

int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info, char *name) {

}
