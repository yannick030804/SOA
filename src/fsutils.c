#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "fat16.h"

int checkParams (int argc, char *argv[]) {

    if (argc != 3) {
        printf("Use: ./fsutils --info <filename>\n");
        return 1;
    }

    if (strcmp(argv[1], "--info") != 0) {
        printf("Error: Invalid option.\n");
        printf("Use: ./fsutils --info <filename>\n");
        return 1;
    }

    return 0;
}

FILE *openFile (char *filename) {
    FILE *fp;
    char path[256];

    snprintf(path, sizeof(path), "../data/ext2/%s", filename);
    fp = fopen(path, "rb");

    if (fp != NULL) {
        return fp;
    }

    snprintf(path, sizeof(path), "../data/fat16/%s", filename);
    fp = fopen(path, "rb");

    if (fp != NULL) {
        return fp;
    }

    return NULL;
}

int isEXT2 (FILE *fp) {
    unsigned short firm;

    fseek(fp, 1080, SEEK_SET);
    fread(&firm, sizeof(unsigned short), 1, fp);

    return (firm == 0xEF53);
}

int isFAT16 (FILE *fp) {
    char fsType[6];

    fseek(fp, 54, SEEK_SET);
    fread(fsType, sizeof(char), 5, fp);
    fsType[5] = '\0';

    if (strcmp(fsType, "FAT16") == 0) {
        return 1;
    }

    return 0;
}

int main (int argc, char *argv[]) {

    if (checkParams(argc, argv)) {
        return 1;
    }

    FILE *fp = openFile(argv[2]);
    
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", argv[2]);
        return 1;
    }

    if (isEXT2(fp)) {
        ext2_info(fp);
    } else if (isFAT16(fp)) {
        fat16_info(fp);
    } else {
        printf("Error: unknown filesystem.\n");
    }

    fclose(fp);

    return 0;
}