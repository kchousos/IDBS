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
  /* info.hashtable[buckets]; */
  memcpy(data, &info, sizeof(info));

  BF_Block_SetDirty(block);

  CALL_OR_DIE(BF_UnpinBlock(block));

  CALL_OR_DIE(BF_CloseFile(file_desc));

  return 0;

}

HT_info *HT_OpenFile(char *fileName) { return NULL; }

int HT_CloseFile(HT_info *HT_info) { return 0; }

int HT_InsertEntry(HT_info *ht_info, Record record) { return 0; }

int HT_GetAllEntries(HT_info *ht_info, void *value) { return 0; }
