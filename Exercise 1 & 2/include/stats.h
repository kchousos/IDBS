#ifndef STATS_H
#define STATS_H

int STATS_GetFiletype(char *filename);

int STATS_NumOfBlocks(char *filename, int filetype);

int STATS_RecordsNum(char *filename, int filetype, int max_min);

int STATS_BlocksNum(char *filename, int filetype, int max_min);

int STATS_PrintOverflowStats(char *filename, int filetype);

#endif // STATS_H
