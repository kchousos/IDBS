#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)                                                          \
  {                                                                            \
    BF_ErrorCode code = call;                                                  \
    if (code != BF_OK) {                                                       \
      BF_PrintError(code);                                                     \
      return HP_ERROR;                                                         \
    }                                                                          \
  }

int HP_CreateFile(char *fileName) {

  CALL_BF(BF_CreateFile(fileName));
  int file_desc;
  CALL_BF(BF_OpenFile(fileName, &file_desc));

  BF_Block *block;
  BF_Block_Init(&block);
  
  void *data;
  CALL_BF(BF_AllocateBlock(file_desc, block));
  data = BF_Block_GetData(block);

  int block_info_offset = MAX_RECS * sizeof(Record);
  HP_block_info block_info;
  block_info.blockDesc = 0;
  block_info.recsNum = 0;
  block_info.nextBlock = NULL;
  memcpy(data + block_info_offset, &block_info, sizeof(block_info));

  HP_info info;
  info.fileDesc = file_desc;
  info.lastBlockDesc = 0;
  memcpy(data, &info, sizeof(info));

  BF_Block_SetDirty(block);

  CALL_BF(BF_UnpinBlock(block));

  CALL_BF(BF_CloseFile(file_desc));

  return 0;
}

HP_info *HP_OpenFile(char *fileName) { return NULL; }

int HP_CloseFile(HP_info *hp_info) { return 0; }

int HP_InsertEntry(HP_info *hp_info, Record record) { return 0; }

int HP_GetAllEntries(HP_info *hp_info, int value) { return 0; }
