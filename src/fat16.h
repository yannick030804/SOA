#ifndef FAT16_H
#define FAT16_H

#include <stdio.h>

typedef struct {
    char systemName[9];
    unsigned short sectorSize;
    unsigned char sectorsPerCluster;
    unsigned short reservedSectors;
    unsigned char numFATs;
    unsigned short maxRootEntries;
    unsigned short sectorsPerFAT;
    char label[12];
} FAT16;

void fat16_info (FILE *fp);

#endif