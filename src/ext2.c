#include "ext2.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint16_t mode;
    uint32_t blocks[15];
} EXT2Inode;

int readTwoBytes(FILE *fp, long offset, uint16_t *value) {
    unsigned char bytes[2];

    if ((fseek(fp, offset, SEEK_SET) != 0) || (fread(bytes, sizeof(unsigned char), 2, fp) != 2)) {
        printf("Error reading file\n");
        return 0;
    }

    *value = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);
    return 1;
}

int readFourBytes(FILE *fp, long offset, uint32_t *value) {
    unsigned char bytes[4];

    if ((fseek(fp, offset, SEEK_SET) != 0) || (fread(bytes, sizeof(unsigned char), 4, fp) != 4)) {
        printf("Error reading file\n");
        return 0;
    }

    *value = (uint32_t) bytes[0] |
             ((uint32_t) bytes[1] << 8) |
             ((uint32_t) bytes[2] << 16) |
             ((uint32_t) bytes[3] << 24);
    return 1;
}

void trim_trailing_spaces(char *text) {
    size_t len = strlen(text);

    while (len > 0 && text[len - 1] == ' ') {
        text[len - 1] = '\0';
        len--;
    }
}

void appendNode(EXT2Node *parent, EXT2Node *child) {
    EXT2Node *current;

    if (parent->child == NULL) {
        parent->child = child;
        return;
    }

    current = parent->child;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = child;
}

int readInode(FILE *fp, EXT2Tree ext2, uint32_t inodeNumber, EXT2Inode *inode) {
    uint32_t group;
    uint32_t inodeIndex;
    uint32_t inodeTableBlock;
    long groupDescriptorEntry;
    long inodeOffset;
    int i;

    group = (inodeNumber - 1) / ext2.inodesPerGroup;
    inodeIndex = (inodeNumber - 1) % ext2.inodesPerGroup;
    groupDescriptorEntry = ext2.groupDescriptorOffset + group * 32;

    if(!readFourBytes(fp, groupDescriptorEntry + 8, &inodeTableBlock)) {
        return 0;
    }

    inodeOffset = (long) inodeTableBlock * ext2.blockSize + (long) inodeIndex * ext2.inodeSize;

    if(!readTwoBytes(fp, inodeOffset, &inode->mode)) {
        return 0;
    }

    for (i = 0; i < 15; i++) {
        if(!readFourBytes(fp, inodeOffset + 40 + i * 4, &inode->blocks[i])) {
            return 0;
        }
    }

    return 1;
}

int isDirectoryInode(uint16_t mode) {
    return (mode & 0xF000) == 0x4000;
}

void readExt2Directory(FILE *fp, EXT2Tree ext2, EXT2Node *parent, uint32_t inodeNumber) {
    EXT2Inode inode;
    int i;

    if(!readInode(fp, ext2, inodeNumber, &inode)) {
        return;
    }

    for (i = 0; i < 12; i++) {
        uint32_t block = inode.blocks[i];
        uint32_t position = 0;
        long blockOffset;

        if (block == 0) {
            continue;
        }

        blockOffset = (long) block * ext2.blockSize;

        while (position < ext2.blockSize) {
            uint32_t entryInode;
            uint16_t recLen;
            unsigned char nameLen;
            unsigned char fileType;
            char name[256];
            EXT2Node *child;
            unsigned int copyLen;

            if(!readFourBytes(fp, blockOffset + position, &entryInode)) {
                return;
            }

            if(!readTwoBytes(fp, blockOffset + position + 4, &recLen)) {
                return;
            }

            if ((fseek(fp, blockOffset + position + 6, SEEK_SET) != 0) ||
                (fread(&nameLen, sizeof(unsigned char), 1, fp) != 1) ||
                (fread(&fileType, sizeof(unsigned char), 1, fp) != 1)) {
                printf("Error reading file\n");
                return;
            }

            if (recLen == 0) {
                break;
            }

            if (entryInode == 0) {
                position += recLen;
                continue;
            }

            copyLen = nameLen;

            if (copyLen >= sizeof(name)) {
                copyLen = sizeof(name) - 1;
            }

            if ((fseek(fp, blockOffset + position + 8, SEEK_SET) != 0) ||
                (fread(name, sizeof(char), copyLen, fp) != copyLen)) {
                printf("Error reading file\n");
                return;
            }

            name[copyLen] = '\0';

            if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
                position += recLen;
                continue;
            }

            child = malloc(sizeof(EXT2Node));
            if (child == NULL) {
                return;
            }

            strcpy(child->name, name);
            child->isDirectory = (fileType == 2);
            child->child = NULL;
            child->next = NULL;

            if (fileType == 0) {
                EXT2Inode childInode;

                if(!readInode(fp, ext2, entryInode, &childInode)) {
                    free(child);
                    return;
                }

                child->isDirectory = isDirectoryInode(childInode.mode);
            }

            appendNode(parent, child);

            if (child->isDirectory) {
                readExt2Directory(fp, ext2, child, entryInode);
            }

            position += recLen;
        }
    }
}

void printExt2Tree(EXT2Node *node, const char *prefix, int isLast) {
    char nextPrefix[256];

    if (node == NULL) {
        return;
    }

    printf("%s", prefix);
    printf("%s", isLast ? "└── " : "├── ");
    printf("%s\n", node->name);

    snprintf(nextPrefix, sizeof(nextPrefix), "%s%s", prefix, isLast ? "    " : "│   ");

    if (node->child != NULL) {
        EXT2Node *child = node->child;

        while (child != NULL) {
            printExt2Tree(child, nextPrefix, child->next == NULL);
            child = child->next;
        }
    }
}

void freeExt2Tree(EXT2Node *node) {
    if (node == NULL) {
        return;
    }

    freeExt2Tree(node->child);
    freeExt2Tree(node->next);
    free(node);
}

/*
 * Format a raw timestamp into a human-readable string
 */
void format_timestamp(uint32_t rawTime, char *buffer, size_t bufferSize) {
    time_t timestamp = (time_t) rawTime;
    struct tm *timeInfo = localtime(&timestamp);

    if (timeInfo == NULL || strftime(buffer, bufferSize, "%a %b %d %H:%M:%S %Y", timeInfo) == 0) {
        snprintf(buffer, bufferSize, "Unavailable");
        return;
    }

    buffer[bufferSize - 1] = '\0';
}

/*
 * Display and show detailed information about an EXT2 filesystem
 */
void showInfoEXT2 (EXT2Info ext2, uint32_t blockSize, const char *lastMountStr, const char *lastWriteStr, const char *lastCheckStr) {
    printf("\n------ Filesystem Information ------\n\n");
    printf("Filesystem: EXT2\n\n");

    printf("INODE INFO\n");
    printf("  Size: %u\n", ext2.inodeSize);
    printf("  Num Inodes: %u\n", ext2.numInodes);
    printf("  First Inode: %u\n", ext2.firstInode);
    printf("  Inodes Group: %u\n", ext2.inodesPerGroup);
    printf("  Free Inodes: %u\n\n", ext2.freeInodes);

    printf("INFO BLOCK\n");
    printf("  Block size: %u\n", blockSize);
    printf("  Reserved blocks: %u\n", ext2.reservedBlocks);
    printf("  Free blocks: %u\n", ext2.freeBlocks);
    printf("  Total blocks: %u\n", ext2.numBlocks);
    printf("  First block: %u\n", ext2.firstBlock);
    printf("  Group blocks: %u\n", ext2.blocksPerGroup);
    printf("  Group frags: %u\n\n", ext2.fragsPerGroup);

    printf("INFO VOLUME\n");
    printf("  Volume name: %s\n", ext2.volumeName);
    printf("  Last Checked: %s\n", lastCheckStr);
    printf("  Last Mounted: %s\n", lastMountStr);
    printf("  Last Written: %s\n", lastWriteStr);
}

/*
 * Get and find information about an EXT2 filesystem
 */
void ext2_info (FILE *fp) {
    EXT2Info ext2;
    char lastMountStr[32];
    char lastWriteStr[32];
    char lastCheckStr[32];

    // Número de inodos
    if(!readFourBytes(fp, 1024 + 0, &ext2.numInodes)) {
        return;
    }

    // Número de bloques
    if(!readFourBytes(fp, 1024 + 4, &ext2.numBlocks)) {
        return;
    }

    // Bloques reservados
    if(!readFourBytes(fp, 1024 + 8, &ext2.reservedBlocks)) {
        return;
    }

    // Bloques libres
    if(!readFourBytes(fp, 1024 + 12, &ext2.freeBlocks)) {
        return;
    }

    // Inodos libres
    if(!readFourBytes(fp, 1024 + 16, &ext2.freeInodes)) {
        return;
    }

    // Primer bloque
    if(!readFourBytes(fp, 1024 + 20, &ext2.firstBlock)) {
        return;
    }

    // log block size
    if(!readFourBytes(fp, 1024 + 24, &ext2.logBlockSize)) {
        return;
    }

    // bloques por grupo
    if(!readFourBytes(fp, 1024 + 32, &ext2.blocksPerGroup)) {
        return;
    }

    // fragmentos por grupo
    if(!readFourBytes(fp, 1024 + 36, &ext2.fragsPerGroup)) {
        return;
    }

    // inodos por grupo
    if(!readFourBytes(fp, 1024 + 40, &ext2.inodesPerGroup)) {
        return;
    }

    // última montura
    if(!readFourBytes(fp, 1024 + 44, &ext2.lastMount)) {
        return;
    }

    // última escritura
    if(!readFourBytes(fp, 1024 + 48, &ext2.lastWrite)) {
        return;
    }

    // última comprobación
    if(!readFourBytes(fp, 1024 + 64, &ext2.lastCheck)) {
        return;
    }

    // primer inode
    if(!readFourBytes(fp, 1024 + 84, &ext2.firstInode)) {
        return;
    }

    // tamaño inode
    if(!readTwoBytes(fp, 1024 + 88, &ext2.inodeSize)) {
        return;
    }

    // nombre volumen
    if((fseek(fp, 1024 + 120, SEEK_SET) != 0) || (fread(ext2.volumeName, sizeof(char), 16, fp) != 16)) {
        printf("Error reading file\n");
        return;
    }

    ext2.volumeName[16] = '\0';
    trim_trailing_spaces(ext2.volumeName);

    // calcular block size real
    uint32_t blockSize = 1024U << ext2.logBlockSize;

    format_timestamp(ext2.lastMount, lastMountStr, sizeof(lastMountStr));
    format_timestamp(ext2.lastWrite, lastWriteStr, sizeof(lastWriteStr));
    format_timestamp(ext2.lastCheck, lastCheckStr, sizeof(lastCheckStr));

    showInfoEXT2(ext2, blockSize, lastMountStr, lastWriteStr, lastCheckStr);
}

/*
 * Show the directory tree of an EXT2 file system
 */

void ext2_tree(FILE *fp) {
    EXT2Tree ext2;
    EXT2Node *root;

    if(!readFourBytes(fp, 1024 + 24, &ext2.blockSize)) {
        return;
    }

    ext2.blockSize = 1024U << ext2.blockSize;

    if(!readFourBytes(fp, 1024 + 40, &ext2.inodesPerGroup)) {
        return;
    }

    if(!readTwoBytes(fp, 1024 + 88, &ext2.inodeSize)) {
        return;
    }

    if (ext2.blockSize == 1024) {
        ext2.groupDescriptorOffset = 2048;
    } else {
        ext2.groupDescriptorOffset = ext2.blockSize;
    }

    root = malloc(sizeof(EXT2Node));
    if (root == NULL) {
        return;
    }

    strcpy(root->name, ".");
    root->isDirectory = 1;
    root->child = NULL;
    root->next = NULL;

    readExt2Directory(fp, ext2, root, 2);

    printf("%s\n", root->name);

    if (root->child != NULL) {
        EXT2Node *child = root->child;

        while (child != NULL) {
            printExt2Tree(child, "", child->next == NULL);
            child = child->next;
        }
    }

    freeExt2Tree(root);
}
