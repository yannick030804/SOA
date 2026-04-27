#ifndef FAT16_H
#define FAT16_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Structure representing the information of an FAT16 file system
 */
typedef struct {
    char systemName[9];
    unsigned short sectorSize;
    unsigned char sectorsPerCluster;
    unsigned short reservedSectors;
    unsigned char numFATs;
    unsigned short maxRootEntries;
    unsigned short sectorsPerFAT;
    char label[12];
} FAT16Info;

/*
 * Structure representing the tree structure of an FAT16 file system
 */
typedef struct {
    unsigned short sectorSize;
    unsigned char sectorsPerCluster;
    unsigned short reservedSectors;
    unsigned char numFATs;
    unsigned short maxRootEntries;
    unsigned short sectorsPerFAT;

    unsigned int rootDirSectors;
    unsigned int fatStart;
    unsigned int rootStart;
    unsigned int dataStart;
    unsigned int clusterSize;
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
