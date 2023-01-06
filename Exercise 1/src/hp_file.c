#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName){
  return CALL_BF(BF_CreateFile(fileName));
}

HP_info* HP_OpenFile(char *fileName){
  HP_info* info = malloc(sizeof(HP_info));
  
  CALL_BF(BF_OpenFile(fileName, info->file));
  CALL_BF(BF_GetBlockCounter(file_desc,info->number_of_blocks));
  
  return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
  return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

