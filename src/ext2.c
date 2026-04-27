#include "ext2.h"
#include <stdint.h>
#include <string.h>

static int readTwoBytes(FILE *fp, long offset, uint16_t *value) {
    unsigned char bytes[2];

    if ((fseek(fp, offset, SEEK_SET) != 0) || (fread(bytes, sizeof(unsigned char), 2, fp) != 2)) {
        printf("Error reading file\n");
        return 0;
    }

    *value = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);
    return 1;
}

static int readFourBytes(FILE *fp, long offset, uint32_t *value) {
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

static void trim_trailing_spaces(char *text) {
    size_t len = strlen(text);

    while (len > 0 && text[len - 1] == ' ') {
        text[len - 1] = '\0';
        len--;
    }
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
    printf("------ Filesystem Information ------\n\n");
    printf("Filesystem: EXT2\n\n");

    printf("INODE INFO\n");
    printf("Size: %u\n", ext2.inodeSize);
    printf("Num Inodes: %u\n", ext2.numInodes);
    printf("First Inode: %u\n", ext2.firstInode);
    printf("Inodes Group: %u\n", ext2.inodesPerGroup);
    printf("Free Inodes: %u\n\n", ext2.freeInodes);

    printf("INFO BLOCK\n");
    printf("Block size: %u\n", blockSize);
    printf("Reserved blocks: %u\n", ext2.reservedBlocks);
    printf("Free blocks: %u\n", ext2.freeBlocks);
    printf("Total blocks: %u\n", ext2.numBlocks);
    printf("First block: %u\n", ext2.firstBlock);
    printf("Group blocks: %u\n", ext2.blocksPerGroup);
    printf("Group frags: %u\n\n", ext2.fragsPerGroup);

    printf("INFO VOLUME\n");
    printf("Volume name: %s\n", ext2.volumeName);
    printf("Last Checked: %s\n", lastCheckStr);
    printf("Last Mounted: %s\n", lastMountStr);
    printf("Last Written: %s\n", lastWriteStr);
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
    (void) fp;
}
