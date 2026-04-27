#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Structure representing the information of an FAT16 file system
 */
typedef struct {
    char systemName[9];
    uint16_t sectorSize;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t maxRootEntries;
    uint16_t sectorsPerFAT;
    char label[12];
} FAT16Info;

/*
 * Structure representing the tree structure of an FAT16 file system
 */
typedef struct {
    uint16_t sectorSize;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t maxRootEntries;
    uint16_t sectorsPerFAT;

    uint32_t rootDirSectors;
    uint32_t fatStart;
    uint32_t rootStart;
    uint32_t dataStart;
    uint32_t clusterSize;
} FAT16Tree;

/*
 * Structure representing a node in the FAT16 tree
 */
typedef struct Node {
    char name[13];
    int isDirectory;
    struct Node *child;
    struct Node *next;
} Node;

/*
 * FAT16 file system functions
 */
void fat16_info (FILE *fp);
void fat16_tree (FILE *fp);

#endif
