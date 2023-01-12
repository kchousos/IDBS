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

int hash_func(char* string) {
  if (string == NULL) {
    printf("NULL string to hash. Exiting...\n");
    return -1;
  }

  int temp = 0;
  
  if (string[1] != NULL)


  return string[1] == NULL ? string[0] % buckets : string[0] % buckets + hash_func(string+sizeof(char));
}


int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char *fileName) {}

SHT_info *SHT_OpenSecondaryIndex(char *indexName) {}

int SHT_CloseSecondaryIndex(SHT_info *SHT_info) {}

int SHT_SecondaryInsertEntry(SHT_info *sht_info, Record record, int block_id) {}

int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info,
                               char *name) {}
