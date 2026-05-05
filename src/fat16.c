#include "fat16.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

static uint16_t next_cluster(FILE *fp, FAT16Tree fat16, uint16_t cluster);
static void build_name(const unsigned char entry[32], char *name);

static int readTwoBytes(FILE *fp, long offset, uint16_t *value) {
    unsigned char bytes[2];

    if ((fseek(fp, offset, SEEK_SET) != 0) || (fread(bytes, sizeof(unsigned char), 2, fp) != 2)) {
        printf("Error reading file\n");
        return 0;
    }

    *value = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);
    return 1;
}

static void trim_trailing_spaces(char *text) {
    size_t len = strlen(text);

    while (len > 0 && text[len - 1] == ' ') {
        text[len - 1] = '\0';
        len--;
    }
}

static int sameName(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char) *left) != tolower((unsigned char) *right)) {
            return 0;
        }

        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static int loadFat16Tree(FILE *fp, FAT16Tree *fat16) {
    if(!readTwoBytes(fp, 11, &fat16->sectorSize)) {
        return 0;
    }

    if((fseek(fp, 13, SEEK_SET) != 0) || (fread(&fat16->sectorsPerCluster, sizeof(unsigned char), 1, fp) != 1)) {
        printf("Error reading file\n");
        return 0;
    }

    if(!readTwoBytes(fp, 14, &fat16->reservedSectors)) {
        return 0;
    }

    if((fseek(fp, 16, SEEK_SET) != 0) || (fread(&fat16->numFATs, sizeof(unsigned char), 1, fp) != 1)) {
        printf("Error reading file\n");
        return 0;
    }

    if(!readTwoBytes(fp, 17, &fat16->maxRootEntries)) {
        return 0;
    }

    if(!readTwoBytes(fp, 22, &fat16->sectorsPerFAT)) {
        return 0;
    }

    fat16->rootDirSectors = (fat16->maxRootEntries * 32 + fat16->sectorSize - 1) / fat16->sectorSize;
    fat16->clusterSize = fat16->sectorSize * fat16->sectorsPerCluster;
    fat16->fatStart = fat16->reservedSectors * fat16->sectorSize;
    fat16->rootStart = (fat16->reservedSectors + fat16->numFATs * fat16->sectorsPerFAT) * fat16->sectorSize;
    fat16->dataStart = fat16->rootStart + fat16->rootDirSectors * fat16->sectorSize;

    return 1;
}

static int findFileInDirectory(FILE *fp, FAT16Tree fat16, uint16_t cluster, const char *fileName, uint16_t *firstCluster, uint32_t *fileSize) {
    unsigned char entry[32];
    int i;
    int entries;
    long offset;

    if (cluster == 0) {
        entries = fat16.maxRootEntries;

        for (i = 0; i < entries; i++) {
            char name[13];
            uint16_t entryCluster;

            offset = fat16.rootStart + i * 32;
            if((fseek(fp, offset, SEEK_SET) != 0) || (fread(entry, sizeof(unsigned char), 32, fp) != 32)) {
                printf("Error reading file\n");
                return 0;
            }

            if (entry[0] == 0x00) {
                break;
            }

            if (entry[0] == 0xE5 || entry[11] == 0x0F || (entry[11] & 0x08)) {
                continue;
            }

            build_name(entry, name);

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            entryCluster = (uint16_t) entry[26] | ((uint16_t) entry[27] << 8);

            if ((entry[11] & 0x10) != 0) {
                if (entryCluster >= 2 && findFileInDirectory(fp, fat16, entryCluster, fileName, firstCluster, fileSize)) {
                    return 1;
                }
            } else if (sameName(name, fileName)) {
                *firstCluster = entryCluster;
                *fileSize = (uint32_t) entry[28] |
                            ((uint32_t) entry[29] << 8) |
                            ((uint32_t) entry[30] << 16) |
                            ((uint32_t) entry[31] << 24);
                return 1;
            }
        }

        return 0;
    }

    while (cluster < 0xFFF8) {
        entries = fat16.clusterSize / 32;
        offset = fat16.dataStart + (long) (cluster - 2) * fat16.clusterSize;

        for (i = 0; i < entries; i++) {
            char name[13];
            uint16_t entryCluster;

            if((fseek(fp, offset + i * 32, SEEK_SET) != 0) || (fread(entry, sizeof(unsigned char), 32, fp) != 32)) {
                printf("Error reading file\n");
                return 0;
            }

            if (entry[0] == 0x00) {
                return 0;
            }

            if (entry[0] == 0xE5 || entry[11] == 0x0F || (entry[11] & 0x08)) {
                continue;
            }

            build_name(entry, name);

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            entryCluster = (uint16_t) entry[26] | ((uint16_t) entry[27] << 8);

            if ((entry[11] & 0x10) != 0) {
                if (entryCluster >= 2 && findFileInDirectory(fp, fat16, entryCluster, fileName, firstCluster, fileSize)) {
                    return 1;
                }
            } else if (sameName(name, fileName)) {
                *firstCluster = entryCluster;
                *fileSize = (uint32_t) entry[28] |
                            ((uint32_t) entry[29] << 8) |
                            ((uint32_t) entry[30] << 16) |
                            ((uint32_t) entry[31] << 24);
                return 1;
            }
        }

        cluster = next_cluster(fp, fat16, cluster);
    }

    return 0;
}

static void printFileContent(FILE *fp, FAT16Tree fat16, uint16_t firstCluster, uint32_t fileSize) {
    unsigned char buffer[4096];
    uint32_t remaining = fileSize;
    uint16_t cluster = firstCluster;

    while (cluster >= 2 && cluster < 0xFFF8 && remaining > 0) {
        uint32_t clusterRemaining = fat16.clusterSize;
        long offset = fat16.dataStart + (long) (cluster - 2) * fat16.clusterSize;

        if (remaining < clusterRemaining) {
            clusterRemaining = remaining;
        }

        while (clusterRemaining > 0) {
            uint32_t chunk = sizeof(buffer);

            if (clusterRemaining < chunk) {
                chunk = clusterRemaining;
            }

            if((fseek(fp, offset, SEEK_SET) != 0) || (fread(buffer, sizeof(unsigned char), chunk, fp) != chunk)) {
                printf("Error reading file\n");
                return;
            }

            fwrite(buffer, sizeof(unsigned char), chunk, stdout);
            offset += chunk;
            clusterRemaining -= chunk;
            remaining -= chunk;
        }

        if (remaining == 0) {
            break;
        }

        cluster = next_cluster(fp, fat16, cluster);
    }
}

uint16_t next_cluster(FILE *fp, FAT16Tree fat16, uint16_t cluster) {
    uint16_t next;

    if(!readTwoBytes(fp, fat16.fatStart + cluster * 2, &next)) {
        return 0xFFFF;
    }

    return next;
}

void build_name(const unsigned char entry[32], char *name) {
    int i;
    int pos = 0;

    for (i = 0; i < 8 && entry[i] != ' '; i++) {
        name[pos++] = (char) entry[i];
    }

    if (entry[8] != ' ') {
        name[pos++] = '.';

        for (i = 8; i < 11 && entry[i] != ' '; i++) {
            name[pos++] = (char) entry[i];
        }
    }

    name[pos] = '\0';
}

void append_child(Node *parent, Node *child) {
    Node *current;

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

void readDirectory(FILE *fp, FAT16Tree fat16, Node *parent, uint16_t cluster) {
    unsigned char entry[32];
    int i;
    int entries;
    long offset;

    if (cluster == 0) {
        entries = fat16.maxRootEntries;

        for (i = 0; i < entries; i++) {
            offset = fat16.rootStart + i * 32;
            if((fseek(fp, offset, SEEK_SET) != 0) || (fread(entry, sizeof(unsigned char), 32, fp) != 32)) {
                printf("Error reading file\n");
                return;
            }

            if (entry[0] == 0x00) {
                break;
            }

            if (entry[0] == 0xE5 || entry[11] == 0x0F || (entry[11] & 0x08)) {
                continue;
            }

            Node *child = malloc(sizeof(Node));
            uint16_t firstCluster;

            if (child == NULL) {
                return;
            }

            build_name(entry, child->name);

            if (strcmp(child->name, ".") == 0 || strcmp(child->name, "..") == 0) {
                free(child);
                continue;
            }

            child->isDirectory = (entry[11] & 0x10) != 0;
            child->child = NULL;
            child->next = NULL;

            append_child(parent, child);

            firstCluster = (uint16_t) entry[26] | ((uint16_t) entry[27] << 8);

            if (child->isDirectory && firstCluster >= 2) {
                readDirectory(fp, fat16, child, firstCluster);
            }
        }

        return;
    }

    while (cluster < 0xFFF8) {
        entries = fat16.clusterSize / 32;
        offset = fat16.dataStart + (long) (cluster - 2) * fat16.clusterSize;

        for (i = 0; i < entries; i++) {
            if((fseek(fp, offset + i * 32, SEEK_SET) != 0) || (fread(entry, sizeof(unsigned char), 32, fp) != 32)) {
                printf("Error reading file\n");
                return;
            }

            if (entry[0] == 0x00) {
                return;
            }

            if (entry[0] == 0xE5 || entry[11] == 0x0F || (entry[11] & 0x08)) {
                continue;
            }

            Node *child = malloc(sizeof(Node));
            uint16_t firstCluster;

            if (child == NULL) {
                return;
            }

            build_name(entry, child->name);

            if (strcmp(child->name, ".") == 0 || strcmp(child->name, "..") == 0) {
                free(child);
                continue;
            }

            child->isDirectory = (entry[11] & 0x10) != 0;
            child->child = NULL;
            child->next = NULL;

            append_child(parent, child);

            firstCluster = (uint16_t) entry[26] | ((uint16_t) entry[27] << 8);

            if (child->isDirectory && firstCluster >= 2) {
                readDirectory(fp, fat16, child, firstCluster);
            }
        }

        cluster = next_cluster(fp, fat16, cluster);
    }
}

static void printTree(Node *node, const char *prefix, int isLast) {
    char nextPrefix[256];

    if (node == NULL) {
        return;
    }

    printf("%s", prefix);
    printf("%s", isLast ? "└── " : "├── ");
    printf("%s\n", node->name);

    snprintf(nextPrefix, sizeof(nextPrefix), "%s%s", prefix, isLast ? "    " : "│   ");

    if (node->child != NULL) {
        Node *child = node->child;

        while (child != NULL) {
            printTree(child, nextPrefix, child->next == NULL);
            child = child->next;
        }
    }
}

static void freeTree(Node *node) {
    if (node == NULL) {
        return;
    }

    freeTree(node->child);
    freeTree(node->next);
    free(node);
}

/*
 * Show the information of an FAT16 file system
 */
void showInfoFAT16 (FAT16Info fat16) {
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

/*
 * Get and find information about an FAT16 filesystem
 */
void fat16_info (FILE *fp) {
    FAT16Info fat16;

    //System name
    if((fseek(fp, 3, SEEK_SET) != 0) || (fread(fat16.systemName, sizeof(char), 8, fp) != 8)) {
        printf("Error reading file\n");
        return;
    }
    fat16.systemName[8] = '\0';

    //Sector size
    if(!readTwoBytes(fp, 11, &fat16.sectorSize)) {
        return;
    }

    // Sectors per cluster
    if((fseek(fp, 13, SEEK_SET) != 0) || (fread(&fat16.sectorsPerCluster, sizeof(unsigned char), 1, fp) != 1)) {
        printf("Error reading file\n");
        return;
    }

    // Reserved sectors
    if(!readTwoBytes(fp, 14, &fat16.reservedSectors)) {
        return;
    }

    // Number of FATs
    if((fseek(fp, 16, SEEK_SET) != 0) || (fread(&fat16.numFATs, sizeof(unsigned char), 1, fp) != 1)) {
        printf("Error reading file\n");
        return;
    }

    // Max root entries
    if(!readTwoBytes(fp, 17, &fat16.maxRootEntries)) {
        return;
    }

    // Sectors per FAT
    if(!readTwoBytes(fp, 22, &fat16.sectorsPerFAT)) {
        return;
    }

    // Label
    if((fseek(fp, 43, SEEK_SET) != 0) || (fread(fat16.label, sizeof(char), 11, fp) != 11)) {
        printf("Error reading file\n");
        return;
    }
    fat16.label[11] = '\0';
    trim_trailing_spaces(fat16.systemName);
    trim_trailing_spaces(fat16.label);

    showInfoFAT16(fat16);
}

/*
 * Show the directory tree of an FAT16 file system
 */

void fat16_tree(FILE *fp) {
    FAT16Tree fat16;

    if(!loadFat16Tree(fp, &fat16)) {
        return;
    }

    Node *root = malloc(sizeof(Node));
    if (root == NULL) {
        return;
    }

    strcpy(root->name, ".");
    root->isDirectory = 1;
    root->child = NULL;
    root->next = NULL;

    readDirectory(fp, fat16, root, 0);

    printf("%s\n", root->name);

    if (root->child != NULL) {
        Node *child = root->child;

        while (child != NULL) {
            printTree(child, "", child->next == NULL);
            child = child->next;
        }
    }

    freeTree(root);
}

/*
 * Show the directory tree of an FAT16 file system
 */

void fat16_cat(FILE* fp, char* file) {
    FAT16Tree fat16;
    uint16_t firstCluster;
    uint32_t fileSize;

    if(!loadFat16Tree(fp, &fat16)) {
        return;
    }

    if(!findFileInDirectory(fp, fat16, 0, file, &firstCluster, &fileSize)) {
        printf("Error: file %s not found\n", file);
        return;
    }

    printFileContent(fp, fat16, firstCluster, fileSize);
}
