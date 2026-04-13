#include "fat16.h"

void showInfoFAT16 (FAT16 fat16) {
    printf("------ Filesystem Information ------\n\n");
    printf("Filesystem: FAT16\n\n");

    printf("System name: %s\n", fat16.systemName);
    printf("Sector size: %u\n", fat16.sectorSize);
    printf("Sectors per cluster: %u\n", fat16.sectorsPerCluster);
    printf("Reserved sectors: %u\n", fat16.reservedSectors);
    printf("# of FATs: %u\n", fat16.numFATs);
    printf("Max root entries: %u\n", fat16.maxRootEntries);
    printf("Sectors per FAT: %u\n", fat16.sectorsPerFAT);
    printf("Label: %s\n", fat16.label);
}

void fat16_info (FILE *fp) {
    FAT16 fat16;

    //System name
    fseek(fp, 3, SEEK_SET);
    fread(fat16.systemName, sizeof(char), 8, fp);
    fat16.systemName[8] = '\0';

    //Sector size
    fseek(fp, 11, SEEK_SET);
    fread(&fat16.sectorSize, sizeof(unsigned short), 1, fp);

    // Sectors per cluster
    fseek(fp, 13, SEEK_SET);
    fread(&fat16.sectorsPerCluster, sizeof(unsigned char), 1, fp);

    // Reserved sectors
    fseek(fp, 14, SEEK_SET);
    fread(&fat16.reservedSectors, sizeof(unsigned short), 1, fp);

    // Number of FATs
    fseek(fp, 16, SEEK_SET);
    fread(&fat16.numFATs, sizeof(unsigned char), 1, fp);

    // Max root entries
    fseek(fp, 17, SEEK_SET);
    fread(&fat16.maxRootEntries, sizeof(unsigned short), 1, fp);

    // Sectors per FAT
    fseek(fp, 22, SEEK_SET);
    fread(&fat16.sectorsPerFAT, sizeof(unsigned short), 1, fp);

    // Label
    fseek(fp, 43, SEEK_SET);
    fread(fat16.label, sizeof(char), 11, fp);
    fat16.label[11] = '\0';

    showInfoFAT16(fat16);
}