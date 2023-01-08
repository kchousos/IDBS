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

  /* Αρχικοποίηση block_info. Πρόκειται για το πρώτο block, άρα το block 0 το
   * οποίο έχει 0 records. Επίσης, εφόσον είναι το μόνο block αρχικά, δεν
   * υπάρχει επόμενο block */
  int block_info_offset = MAX_RECS * sizeof(Record);
  HP_block_info block_info;
  block_info.blockDesc = 0;
  block_info.recsNum = 0;
  block_info.nextBlock = -1;
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

  /* TODO: έλεγχος για αρχείο κατακερματισμού */

  /* Δεν χρησιμοποιείται η CALL_BF διότι η συνάρτηση σε περίπτωση λάθους πρέπει
   * να επιστρέφει NULL, όχι int */

  int file_desc;
  BF_OpenFile(fileName, &file_desc);

  /* Πρόσβαση στο block 0 */
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(file_desc, 0, block);
  void *data = BF_Block_GetData(block);

  /* Πρόσβαση στο HP_info του block 0 */
  HP_info *hp_info = malloc(sizeof(HP_info));
  memcpy(hp_info, data, sizeof(HP_info));

  /* Το file_desc του info αλλάζει από την κλήση της BF_OpenFile, άρα χρειάζεται
   * να ενημερώσουμε την τιμή στο HP_info  */
  hp_info->fileDesc = file_desc;
  /* Ενημέρωση του HP_info του block 0 */
  memcpy(data, hp_info, sizeof(HP_info));

  BF_Block_SetDirty(block);

  BF_UnpinBlock(block);

  return hp_info;
}

int HP_CloseFile(HP_info *hp_info) {

  /* Βρίσκουμε πόσα blocks έχει το αρχείο */
  int file_desc = hp_info->fileDesc;
  int blocks_num;
  CALL_BF(BF_GetBlockCounter(file_desc, &blocks_num));

  BF_Block *block;
  BF_Block_Init(&block);

  /* Κάνουμε unpin όλα τα blocks */
  for (int block_number = 0; block_number < blocks_num; block_number++) {

    CALL_BF(BF_GetBlock(file_desc, block_number, block));
    CALL_BF(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);

  /* κλείσιμο αρχείου */
  CALL_BF(BF_CloseFile(file_desc));
  free(hp_info);

  return 0;
}

int HP_InsertEntry(HP_info *hp_info, Record record) {

  int blockId;

  /* Δημιουργία block */
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->lastBlockDesc, block));

  /* Πρόσβαση στο block_info του τελευταίου block */
  HP_block_info block_info;
  void *data = BF_Block_GetData(block);
  memcpy(&block_info, data + MAX_RECS * sizeof(Record), sizeof(HP_block_info));
  CALL_BF(BF_UnpinBlock(block));

  /* Έλεγχος διαθέσιμου χώρου */
  int recs_size = block_info.recsNum * sizeof(Record);
  int space_left_in_block = BF_BLOCK_SIZE - (recs_size + sizeof(HP_block_info));

  /* Αν υπάρχει διαθέσιμος χώρος, γράψε στο τελευταίο block το record, αλλιώς
   * φτιάξε καινούργιο block */
  if (space_left_in_block >= sizeof(Record) && block_info.blockDesc != 0) {

    /* Εγγραφή record */
    memcpy(data + recs_size, &record, sizeof(Record));

    /* Ενημέρωση block_info */
    block_info.recsNum++;
    memcpy(data + MAX_RECS * sizeof(Record), &block_info,
           sizeof(HP_block_info));

    blockId = block_info.blockDesc;
  } else {

    CALL_BF(BF_AllocateBlock(hp_info->fileDesc, block));

    /* Εισαγωγή record */
    data = BF_Block_GetData(block);
    memcpy(data, &record, sizeof(Record));

    /* Ενημέρωση block_info καινούργιου block */
    block_info.recsNum = 1;
    block_info.blockDesc = hp_info->lastBlockDesc + 1;
    block_info.nextBlock = block_info.blockDesc + 1;

    memcpy(data + MAX_RECS * sizeof(Record), &block_info,
           sizeof(HP_block_info));

    /* Ενημέρωση hp_info */
    hp_info->lastBlockDesc = block_info.blockDesc;
    
    CALL_BF(BF_UnpinBlock(block));
  }

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return blockId;
}

int HP_GetAllEntries(HP_info *hp_info, int value) {

  /* Αρχικοποιούμε μια μεταβλητή όπου θα κρατάμε τον αριθμό των blocks που
   * διαβάστηκαν σε -1. Αν δεν αλλάξει κατά την εκτέλεση της συνάρτησης,
   * σημαίνει υπήρξε λάθος στην εκτέλεση και θα επιστραφεί -1. */
  int read_blocks = -1;
  int file_desc = hp_info->fileDesc;

  /* Κρατάμε σε μια μεταβλητή των αριθμό όλων των block του αρχείου */
  int blocks_num;
  CALL_BF(BF_GetBlockCounter(file_desc, &blocks_num));

  BF_Block *block;
  BF_Block_Init(&block);

  HP_block_info block_info;

  void *data;
  int block_info_offset = MAX_RECS * sizeof(Record);

  /* Προσπέλαση των blocks */
  for (int block_number = 1; block_number < blocks_num; block_number++) {

    /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το HP_block_info,
     * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
     * αυτό. */
    CALL_BF(BF_GetBlock(file_desc, block_number, block));
    data = BF_Block_GetData(block);
    memcpy(&block_info, data + block_info_offset, sizeof(HP_block_info));

    Record *recs = data;
    /* Για κάθε εγγραφή του block, ελέγχουμε αν το id της ταυτίζεται με το
     * δοθέν value. Αν η σύγκριση είναι αληθής τυπώνουμε την εγγραφή αυτή */
    for (int records = 0; records < block_info.recsNum; records++) {

      if (recs[records].id == value) {

        printRecord(recs[records]);
        read_blocks = block_number;
      }
    }

    CALL_BF(BF_UnpinBlock(block));
  }

  return read_blocks;
}
