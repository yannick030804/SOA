#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Structure representing the information of an EXT2 file system
 */
typedef struct {
    uint32_t numInodes;
    uint32_t numBlocks;
    uint32_t reservedBlocks;
    uint32_t freeBlocks;
    uint32_t freeInodes;
    uint32_t firstBlock;
    uint32_t logBlockSize;
    uint32_t blocksPerGroup;
    uint32_t fragsPerGroup;
    uint32_t inodesPerGroup;
    uint32_t lastMount;
    uint32_t lastWrite;
    uint32_t lastCheck;
    uint32_t firstInode;
    uint16_t inodeSize;
    char volumeName[17];
} EXT2Info;

/*
 * EXT2 file system functions
 */
void ext2_info (FILE *fp);
void ext2_tree (FILE *fp);

#endif
