#ifndef STATS_H
#define STATS_H

int STATS_GetFiletype(char *filename, void *info);

int STATS_NumberOfBlocks(char *filename, void *info, int filetype);

int STATS_MinRecordsNum(char *filename, void *info, int filetype);

int STATS_MaxRecordsNum(char *filename, void *info, int filetype);

int STATS_MeanBlocksNum(char *filename, void *info, int filetype);

int STATS_PrintOverflowStats(char *filename, void *info, int filetype);

int HashStatistics(char *filename, void *info);

#endif // STATS_H
