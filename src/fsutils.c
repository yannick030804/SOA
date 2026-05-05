#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ext2.h"
#include "fat16.h"

int checkParams (int argc, char *argv[]) {

    if (argc == 4 && strcmp(argv[1], "--cat") == 0) {
        return 3;
    }

    if (argc != 3) {
        printf("Error: Try one of the following options:\n");
        printf("Use: ./fsutils --info <filename>\n");
        printf("Use: ./fsutils --tree <filename>\n");
        printf("Use: ./fsutils --cat <filename> <file>\n");
        return 1;
    }

    if (strcmp(argv[1], "--info") == 0) {
        return 0;
    } else if (strcmp(argv[1], "--tree") == 0) {
        return 2;
    } else {
        printf("Error: Try one of the following options:\n");
        printf("Use: ./fsutils --info <filename>\n");
        printf("Use: ./fsutils --tree <filename>\n");
        printf("Use: ./fsutils --cat <filename> <file>\n");
        return 1;
    }
}

FILE *openFile (char *filename) {
    FILE *fp = NULL;
    char path[256];
    const char *searchPaths[] = {
        "data/ext2",
        "data/fat16",
        "../data/ext2",
        "../data/fat16"
    };
    size_t i;

    for (i = 0; i < sizeof(searchPaths) / sizeof(searchPaths[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", searchPaths[i], filename);
        fp = fopen(path, "rb");

        if (fp != NULL) {
            return fp;
        }
    }

    return NULL;
}

int isEXT2 (FILE *fp) {
    unsigned char bytes[2];
    uint16_t firm;

    if((fseek(fp, 1080, SEEK_SET) != 0) || (fread(bytes, sizeof(unsigned char), 2, fp) != 2)) {
        return 0;
    }

    firm = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);

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

    int option = checkParams(argc, argv);

    if (option == 1) {
        return 1;
    }

    FILE *fp = openFile(argv[2]);

    if (fp == NULL) {
        printf("Error: cannot open file %s\n", argv[2]);
        return 1;
    }

    if (isEXT2(fp)) {
        if (option == 0) {
            ext2_info(fp);
        } else if (option == 2) {
            ext2_tree(fp);
        } else if (option == 3) {
            printf("Error: --cat is only available for FAT16.\n");
        }
    } else if (isFAT16(fp)) {
        if (option == 0) {
            fat16_info(fp);
        } else if (option == 2) {
            fat16_tree(fp);
        } else if (option == 3) {
            fat16_cat(fp, argv[3]);
        }
    } else {
        printf("Error: unknown filesystem.\n");
    }

    fclose(fp);

    return 0;
}
