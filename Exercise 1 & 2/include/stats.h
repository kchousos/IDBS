#ifndef STATS_H
#define STATS_H

int STATS_GetFiletype(char *filename, void *info);

int STATS_NumOfBlocks(char *filename, int filetype);

int STATS_RecordsNum(char *filename, int filetype, int max_min);

int STATS_BlocksNum(char *filename, int filetype, int max_min);

int STATS_PrintOverflowStats(char *filename, int filetype);

int HashStatistics(char *filename, void *info);

#endif // STATS_H
