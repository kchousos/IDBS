#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "ht_table.h"
#include "record.h"
#include "sht_table.h"
#include "stats.h"

#define CALL_OR_DIE(call)                                                      \
  {                                                                            \
    BF_ErrorCode code = call;                                                  \
    if (code != BF_OK) {                                                       \
      BF_PrintError(code);                                                     \
      exit(code);                                                              \
    }                                                                          \
  }

int STATS_GetFiletype(char *filename, void *info) {

  int filetype = -1;

  HP_info *hp_info = (HP_info *)info;
  HT_info *ht_info = (HT_info *)info;
  SHT_info *sht_info = (SHT_info *)info;

  if (strcmp(hp_info->filetype, "heap") == 0)
    filetype = 1;
  else if (strcmp(ht_info->filetype, "hashtable") == 0)
    filetype = 2;
  else if (strcmp(sht_info->filetype, "sec-index") == 0)
    filetype = 3;

  return filetype;
}

int STATS_NumberOfBlocks(char *filename, void *info, int filetype) {
  int blocks_num = 0;

  if (filetype == 1) {
    HP_info *local_info = (HP_info* ) info;
    CALL_OR_DIE(BF_GetBlockCounter(local_info->fileDesc, &blocks_num));
  } else if (filetype == 2) {
    HT_info *local_info = (HT_info* ) info;
    CALL_OR_DIE(BF_GetBlockCounter(local_info->fileDesc, &blocks_num));
  } else if (filetype == 3) {
    SHT_info *local_info = (SHT_info* ) info;
    CALL_OR_DIE(BF_GetBlockCounter(local_info->fileDesc, &blocks_num));
  }
  
  return blocks_num;
}

int STATS_MinRecordsNum(char *filename, void *info, int filetype) {

  int min_records = INT_MAX;

  /* Hashtable */
  if (filetype == 2) {

    HT_info *local_info = (HT_info *)info;

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->hashtable[bucket], block));

      HT_block_info block_info;

      int block_info_offset = HT_MAX_RECS * sizeof(Record);

      int block_to_read = local_info->hashtable[bucket];

      /* Προσπέλαση των blocks του bucket */
      while (block_to_read != -1) {

        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το HT_block_info,
         * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
         * αυτό. */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(HT_block_info));

        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      if (bucket_records < min_records)
        min_records = bucket_records;

      BF_Block_Destroy(&block);
    }

    /* Secondary Index */
  } else if (filetype == 3) {

    SHT_info *local_info = (SHT_info *)info;

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->sht_hashtable[bucket], block));

      SHT_block_info block_info;

      int block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);

      int block_to_read = local_info->sht_hashtable[bucket];

      /* Προσπέλαση των blocks του bucket */
      while (block_to_read != -1) {

        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το SHT_block_info,
         * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
         * αυτό. */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(SHT_block_info));

        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      if (bucket_records < min_records)
        min_records = bucket_records;

      BF_Block_Destroy(&block);
    }
  }

  return min_records;
}

int STATS_MaxRecordsNum(char *filename, void *info, int filetype) {

  int max_records = INT_MIN;

  /* Hashtable */
  if (filetype == 2) {

    HT_info *local_info = (HT_info *)info;

    BF_OpenFile(filename, &local_info->fileDesc);

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->hashtable[bucket], block));

      HT_block_info block_info;

      int block_info_offset = HT_MAX_RECS * sizeof(Record);

      int block_to_read = local_info->hashtable[bucket];

      /* Προσπέλαση των blocks του bucket */
      while (block_to_read != -1) {

        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το HT_block_info,
         * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
         * αυτό. */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(HT_block_info));

        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      if (bucket_records > max_records)
        max_records = bucket_records;

      BF_Block_Destroy(&block);
    }

  } else if (filetype == 3) {

    SHT_info *local_info = (SHT_info *)info;

    BF_OpenFile(filename, &local_info->fileDesc);

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->sht_hashtable[bucket], block));

      SHT_block_info block_info;

      int block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);

      int block_to_read = local_info->sht_hashtable[bucket];

      /* Προσπέλαση των blocks του bucket */
      while (block_to_read != -1) {

        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το HT_block_info,
         * όπου είναι αποθηκευμένος ο αριθμός των εγγραφών που έχουν γίνει σε
         * αυτό. */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(SHT_block_info));

        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      if (bucket_records > max_records)
        max_records = bucket_records;

      BF_Block_Destroy(&block);
    }
  }

  return max_records;
}

int HashStatistics(char *filename, void *info) {

  int filetype = STATS_GetFiletype(filename, info);

  switch (filetype) {

  case -1:
    printf("Σφάλμα\n");
    return -1;

  case 1:
    printf("====== Αρχείο Σωρού ======\n");
    printf("Πλήθος από blocks = %d\n",
           STATS_NumberOfBlocks(filename, info, filetype));
    return 0;

  case 2:
    printf("====== Αρχείο Κατακερματισμού ======\n");
    break;

  case 3:
    printf("====== Αρχείο Δευτερεύοντος Ευρετηρίου ======\n");
    break;
  }

  printf("Πλήθος από blocks = %d\n",
         STATS_NumberOfBlocks(filename, info, filetype));
  int min_num_of_records = STATS_MinRecordsNum(filename, info, filetype);
  int max_num_of_records = STATS_MaxRecordsNum(filename, info, filetype);

  printf("Μέγιστο πλήθος εγγραφών κάθε bucket = %d\n", max_num_of_records);
  printf("Ελάχιστο πλήθος εγγραφών κάθε bucket = %d\n", min_num_of_records);
  printf("Μέσο πλήθος εγγραφών κάθε bucket = %d\n",
         (max_num_of_records + min_num_of_records) / 2);

  /* int max_num_of_blocks = STATS_MaxBlocksNum(filename,info, filetype);
  int min_num_of_blocks = STATS_MinBlocksNum(filename,info, filetype);
  printf("Μέσος αριθμός blocks κάθε bucket = %d\n",
         (max_num_of_blocks + min_num_of_blocks) / 2); */

  /* if (STATS_PrintOverflowStats(filename, info, filetype))
    return -1; */

  return 0;
}
