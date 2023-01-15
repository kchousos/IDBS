#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
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

int STATS_GetFiletype(char *filename, void *info) {

  int return_code = -1;

  HP_info *hp_info = (HP_info *)info;

  if (!strcmp(hp_info->filetype, "heap"))
    return_code = 1;

  return return_code;
}

int HashStatistics(char *filename, void *info) {

  int filetype = STATS_GetFiletype(filename, info);

  switch (filetype) {

  case -1:
    printf("Σφάλμα\n");
    return -1;

  case 1:
    printf("====== Αρχείο Σωρού ======\n\n");
    /* printf("Πλήθος από blocks = %d\n", STATS_NumOfBlocks(filename, filetype)); */
    return 0;

  case 2:
    printf("====== Αρχείο Κατακερματισμού ======\n\n");

  case 3:
    printf("====== Αρχείο Δευτερεύοντος Ευρετηρίου ======\n\n");
  }

  /* int max_num_of_records = STATS_RecordsNum(filename, filetype, 1); */
  /* int min_num_of_records = STATS_RecordsNum(filename, filetype, 2); */

  /* printf("Μέγιστο πλήθος εγγραφών κάθε bucket = %d\n", max_num_of_records); */
  /* printf("Ελάχιστο πλήθος εγγραφών κάθε bucket = %d\n", min_num_of_records); */
  /* printf("Μέσο πλήθος εγγραφών κάθε bucket = %d\n", */
  /*        (max_num_of_records + min_num_of_records) / 2); */

  /* int max_num_of_blocks = STATS_BlocksNum(filename, filetype, 1); */
  /* int min_num_of_blocks = STATS_BlocksNum(filename, filetype, 2); */

  /* printf("Μέσος αριθμός blocks κάθε bucket = %d\n", */
  /*        (max_num_of_blocks + min_num_of_blocks) / 2); */

  /* if (STATS_PrintOverflowStats(filename, filetype)) */
  /*   return -1; */

  return 0;
}
