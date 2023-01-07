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

HP_info *HP_OpenFile(char *fileName) { 
  int file_desc;
  CALL_BF(BF_OpenFile(fileName, &file_desc));
  HP_info* info = malloc(sizeof(HP_info));
  info->fileDesc = file_desc;

  BF_Block* block;
  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(file_desc, 0, block));
  void* data = BF_Block_GetData(block);

  memcpy(info, data, sizeof(HP_info));

  return info; 
}

int HP_CloseFile(HP_info *hp_info) { 
  BF_Block* block;
  BF_Block_Init(&block);
  
  CALL_BF(BF_GetBlock(hp_info->fileDesc, 0, block));
  
  int block_info_offset = MAX_RECS * sizeof(Record);
  HP_block_info block_info;
  void* data = BF_Block_GetData(block);
  memcpy(&block_info, data + block_info_offset, sizeof(HP_block_info));
  
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  while (block_info.nextBlock != NULL) {
    BF_Block* next_block;
    BF_Block_Init(next_block);
    
    int counter;
    CALL_BF(BF_GetBlockCounter(block_info.nextBlock, &counter));
    CALL_BF(BF_GetBlock(hp_info->fileDesc, counter, next_block));
    
    void* data = BF_Block_GetData(next_block);
    memcpy(&block_info, data + block_info_offset, sizeof(HP_block_info));

    CALL_BF(BF_UnpinBlock(next_block));
    BF_Block_Destroy(&next_block);
  }
  
  int file_desc = hp_info->fileDesc;
  free(hp_info);

  CALL_BF(BF_CloseFile(file_desc));
  return 0; 
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

