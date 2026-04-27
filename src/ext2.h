#ifndef EXT2_H
#define EXT2_H

#include <stdio.h>
#include <time.h>

typedef struct {
    unsigned int numInodes;
    unsigned int numBlocks;
    unsigned int reservedBlocks;
    unsigned int freeBlocks;
    unsigned int freeInodes;
    unsigned int firstBlock;
    unsigned int logBlockSize;
    unsigned int blocksPerGroup;
    unsigned int fragsPerGroup;
    unsigned int inodesPerGroup;
    unsigned int lastMount;
    unsigned int lastWrite;
    unsigned int lastCheck;
    unsigned int firstInode;
    unsigned short inodeSize;
    char volumeName[17];
} EXT2;

void ext2_info (FILE *fp);
void ext2_tree (FILE *fp);

#endif
