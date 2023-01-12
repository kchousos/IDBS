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

int SHT_Hash(char *name, long int buckets) {

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

SHT_info *SHT_OpenSecondaryIndex(char *indexName) {}

int SHT_CloseSecondaryIndex(SHT_info *SHT_info) {}

int SHT_SecondaryInsertEntry(SHT_info *sht_info, Record record, int block_id) {}

int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info,
                               char *name) {}
