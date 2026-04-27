#include "ext2.h"

/*
 * Format a raw timestamp into a human-readable string
 */
static void format_timestamp(unsigned int rawTime, char *buffer, size_t bufferSize) {
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
void showInfoEXT2 (EXT2Info ext2, unsigned int blockSize, const char *lastMountStr, const char *lastWriteStr, const char *lastCheckStr) {
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
    fseek(fp, 1024 + 0, SEEK_SET);
    fread(&ext2.numInodes, sizeof(unsigned int), 1, fp);

    // Número de bloques
    fseek(fp, 1024 + 4, SEEK_SET);
    fread(&ext2.numBlocks, sizeof(unsigned int), 1, fp);

    // Bloques reservados
    fseek(fp, 1024 + 8, SEEK_SET);
    fread(&ext2.reservedBlocks, sizeof(unsigned int), 1, fp);

    // Bloques libres
    fseek(fp, 1024 + 12, SEEK_SET);
    fread(&ext2.freeBlocks, sizeof(unsigned int), 1, fp);

    // Inodos libres
    fseek(fp, 1024 + 16, SEEK_SET);
    fread(&ext2.freeInodes, sizeof(unsigned int), 1, fp);

    // Primer bloque
    fseek(fp, 1024 + 20, SEEK_SET);
    fread(&ext2.firstBlock, sizeof(unsigned int), 1, fp);

    // log block size
    fseek(fp, 1024 + 24, SEEK_SET);
    fread(&ext2.logBlockSize, sizeof(unsigned int), 1, fp);

    // bloques por grupo
    fseek(fp, 1024 + 32, SEEK_SET);
    fread(&ext2.blocksPerGroup, sizeof(unsigned int), 1, fp);

    // fragmentos por grupo
    fseek(fp, 1024 + 36, SEEK_SET);
    fread(&ext2.fragsPerGroup, sizeof(unsigned int), 1, fp);

    // inodos por grupo
    fseek(fp, 1024 + 40, SEEK_SET);
    fread(&ext2.inodesPerGroup, sizeof(unsigned int), 1, fp);

    // última montura
    fseek(fp, 1024 + 44, SEEK_SET);
    fread(&ext2.lastMount, sizeof(unsigned int), 1, fp);

    // última escritura
    fseek(fp, 1024 + 48, SEEK_SET);
    fread(&ext2.lastWrite, sizeof(unsigned int), 1, fp);

    // última comprobación
    fseek(fp, 1024 + 64, SEEK_SET);
    fread(&ext2.lastCheck, sizeof(unsigned int), 1, fp);

    // primer inode
    fseek(fp, 1024 + 84, SEEK_SET);
    fread(&ext2.firstInode, sizeof(unsigned int), 1, fp);

    // tamaño inode
    fseek(fp, 1024 + 88, SEEK_SET);
    fread(&ext2.inodeSize, sizeof(unsigned short), 1, fp);

    // nombre volumen
    fseek(fp, 1024 + 120, SEEK_SET);
    fread(ext2.volumeName, sizeof(char), 16, fp);
    ext2.volumeName[16] = '\0';

    // calcular block size real
    unsigned int blockSize = 1024 << ext2.logBlockSize;

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
