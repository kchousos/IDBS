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

int STATS_GetFiletype(void *info) {

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

int STATS_NumberOfBlocks(void *info, int filetype) {
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

int STATS_MinRecordsNum(void *info, int filetype) {

  int min_records = INT_MAX;

  /* Hashtable */
  if (filetype == 2) {

    HT_info *local_info = (HT_info *)info;

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      if (local_info->hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->hashtable[bucket], block));
      }else {
        continue;
      }

      /* Εύρεση του block_info του εκάστοτε block */
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

        /* Ενημέρωση του block ελέγχου και της μεταβλητής όπου αποθηκεύεται ο
         * συνολικός αριθμλος των εγγραφών του εκάστοτε block */
        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      /* Ενημέρωση της τιμής της ελάχιστης εγγραφής */
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
      if (local_info->sht_hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->sht_hashtable[bucket], block));
      }else {
        continue;
      }

      /* Εύρεση του block_info του εκάστοτε block */
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

        /* Ενημέρωση του block ελέγχου και της μεταβλητής όπου αποθηκεύεται ο
         * συνολικός αριθμλος των εγγραφών του εκάστοτε block */
        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      /* Ενημέρωση της τιμής της ελάχιστης εγγραφής */
      if (bucket_records < min_records)
        min_records = bucket_records;

      BF_Block_Destroy(&block);
    }
  }

  return min_records;
}

int STATS_MaxRecordsNum(void *info, int filetype) {

  int max_records = INT_MIN;

  /* Hashtable */
  if (filetype == 2) {

    HT_info *local_info = (HT_info *)info;

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      if (local_info->hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->hashtable[bucket], block));
      }else {
        continue;
      }

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

        /* Ενημέρωση του block ελέγχου και της μεταβλητής όπου αποθηκεύεται ο
         * συνολικός αριθμλος των εγγραφών του εκάστοτε block */
        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      /* Ενημέρωση της τιμής της μέγιστης εγγραφής */
      if (bucket_records > max_records)
        max_records = bucket_records;

      BF_Block_Destroy(&block);
    }

  } else if (filetype == 3) {

    SHT_info *local_info = (SHT_info *)info;

    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {

      int bucket_records = 0;

      /* Δημιουργία block */
      BF_Block *block;
      BF_Block_Init(&block);
      if (local_info->sht_hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->sht_hashtable[bucket], block));
      }else {
        continue;
      }

      /* Εύρεση του block_info του εκάστοτε block */
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

        /* Ενημέρωση του block ελέγχου και της μεταβλητής όπου αποθηκεύεται ο
         * συνολικός αριθμλος των εγγραφών του εκάστοτε block */
        block_to_read = block_info.prevBlockDesc;
        bucket_records += block_info.recsNum;
      }

      /* Ενημέρωση της τιμής της ελάχιστης εγγραφής */
      if (bucket_records > max_records)
        max_records = bucket_records;

      BF_Block_Destroy(&block);
    }
  }

  return max_records;
}

int STATS_BucketsNum(void *info, int filetype) {
  int buckets_num = 0;
  
  if (filetype == 2) {
    HT_info* local_info = (HT_info* ) info;
    buckets_num = local_info->numBuckets;
  } else if (filetype == 3) {
    SHT_info* local_info = (SHT_info* ) info;
    buckets_num = local_info->numBuckets;
  }

  return buckets_num;
}

int STATS_PrintOverflowStats(void *info, int filetype) {
  if (filetype == 2) {
    HT_info* local_info = (HT_info* ) info;

    int ovf_buckets_num = 0;
    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {
      BF_Block *block;
      BF_Block_Init(&block);
      if (local_info->hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->hashtable[bucket], block));
      }else {
        continue;
      }

      HT_block_info block_info;

      int block_info_offset = HT_MAX_RECS * sizeof(Record);

      int block_to_read = local_info->hashtable[bucket];

      int ovf_blocks_num = 0;
      int first_block_of_bucket = 1;
      
      while (block_to_read != -1) {
        if (!first_block_of_bucket)
          ovf_blocks_num++;

        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το HT_block_info,
         * όπου είναι αποθηκευμένο το προηγούμενο block του bucket (block
         * υπερχείλισης) */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(HT_block_info));

        block_to_read = block_info.prevBlockDesc;

        if (first_block_of_bucket && block_to_read != -1) {
          ovf_buckets_num++;
          first_block_of_bucket = 0;
        }
      }
      printf("Το πλήθος των μπλοκ υπερχείλισης του bucket %d είναι %d\n", bucket, ovf_blocks_num);
    }

    printf("Το πλήθος των buckets που έχουν μπλοκ υπερχείλισης είναι %d\n", ovf_buckets_num);

  } else if (filetype == 3) {
    SHT_info* local_info = (SHT_info* ) info;

    int ovf_buckets_num = 0;
    for (int bucket = 0; bucket < local_info->numBuckets; bucket++) {
      BF_Block *block;
      BF_Block_Init(&block);
      if (local_info->sht_hashtable[bucket] != -1) {
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc,
                              local_info->sht_hashtable[bucket], block));
      }else {
        continue;
      }

      SHT_block_info block_info;

      int block_info_offset = SHT_MAX_RECS * sizeof(SHT_Record);

      int block_to_read = local_info->sht_hashtable[bucket];

      int ovf_blocks_num = 0;
      int first_block_of_bucket = 1;
      
      while (block_to_read != -1) {
        if (!first_block_of_bucket)
          ovf_blocks_num++;
        
        /* Για το block που διαβάζουμε κάθε φορά, βρίσκουμε το SHT_block_info,
         * όπου είναι αποθηκευμένο το προηγούμενο block του bucket (block
         * υπερχείλισης) */
        CALL_OR_DIE(BF_GetBlock(local_info->fileDesc, block_to_read, block));
        void *data = BF_Block_GetData(block);
        CALL_OR_DIE(BF_UnpinBlock(block));

        memcpy(&block_info, data + block_info_offset, sizeof(SHT_block_info));

        block_to_read = block_info.prevBlockDesc;
        if (first_block_of_bucket && block_to_read != -1) {
          ovf_buckets_num++;
          first_block_of_bucket = 0;
        }
      }
      printf("Το πλήθος των μπλοκ υπερχείλισης του bucket %d είναι %d\n", bucket, ovf_blocks_num);
    }

    printf("Το πλήθος των buckets που έχουν μπλοκ υπερχείλισης είναι %d\n", ovf_buckets_num);
  }

  return 0;
}

int HashStatistics(void *info) {

  int filetype = STATS_GetFiletype(info);

  switch (filetype) {

  case -1:
    printf("Σφάλμα\n");
    return -1;

  case 1:
    printf("====== Αρχείο Σωρού ======\n");
    printf("Πλήθος από blocks = %d\n",
           STATS_NumberOfBlocks(info, filetype));
    return 0;

  case 2:
    printf("====== Αρχείο Κατακερματισμού ======\n");
    break;

  case 3:
    printf("====== Αρχείο Δευτερεύοντος Ευρετηρίου ======\n");
    break;
  }

  int blocks_num = STATS_NumberOfBlocks(info, filetype);

  printf("Πλήθος από blocks = %d\n", blocks_num);
  int min_num_of_records = STATS_MinRecordsNum(info, filetype);
  int max_num_of_records = STATS_MaxRecordsNum(info, filetype);

  printf("Μέγιστο πλήθος εγγραφών κάθε bucket = %d\n", max_num_of_records);
  printf("Ελάχιστο πλήθος εγγραφών κάθε bucket = %d\n", min_num_of_records);
  printf("Μέσο πλήθος εγγραφών κάθε bucket = %d\n",
         (max_num_of_records + min_num_of_records) / 2);

  int buckets_num = STATS_BucketsNum(info, filetype);
  printf("Μέσος αριθμός blocks κάθε bucket = %f\n", (float)blocks_num / (float)buckets_num);

  if (STATS_PrintOverflowStats(info, filetype))
    return -1;

  return 0;
}
