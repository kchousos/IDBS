#ifndef STATS_H
#define STATS_H

int STATS_GetFiletype(void *info);

int STATS_NumberOfBlocks(void *info, int filetype);

int STATS_MinRecordsNum(void *info, int filetype);

int STATS_MaxRecordsNum(void *info, int filetype);

int STATS_BucketsNum(void *info, int filetype);

int STATS_PrintOverflowStats(void *info, int filetype);

int HashStatistics(void *info);

#endif // STATS_H
