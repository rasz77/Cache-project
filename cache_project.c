// cachelab.c
// Cache Simulator -> Implemented LRU and both FIFO and Optimal

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "cachelab.h"

typedef struct {
    int valid;
    uint64_t tag;
    int lastUsed;   // for LRU: timestamp of last use
    int inserted;   // for FIFO: insertion timestamp
} CacheLine;

typedef struct {
    CacheLine *lines; // array of lines per set
} CacheSet;

typedef struct {
    int setCount;    // S = 2^s
    int lineCount;   // E = associativity
    int blockSize;   // B = 2^b
    CacheSet *sets;
} Cache;

// Initialize cache
Cache initCache(int S, int E, int B) {
    Cache cache;
    cache.setCount = S;
    cache.lineCount = E;
    cache.blockSize = B;
    cache.sets = (CacheSet *)malloc(S * sizeof(CacheSet));
    if (!cache.sets) {
        fprintf(stderr, "Memory allocation failed for cache sets\n");
        exit(1);
    }

    for (int i = 0; i < S; i++) {
        cache.sets[i].lines = (CacheLine *)malloc(E * sizeof(CacheLine));
        if (!cache.sets[i].lines) {
            fprintf(stderr, "Memory allocation failed for cache lines\n");
            exit(1);
        }
        for (int j = 0; j < E; j++) {
            cache.sets[i].lines[j].valid = 0;
            cache.sets[i].lines[j].tag = 0;
            cache.sets[i].lines[j].lastUsed = 0;
            cache.sets[i].lines[j].inserted = 0;
        }
    }
    return cache;
}

void freeCache(Cache *cache) {
    if (!cache) return;
    for (int i = 0; i < cache->setCount; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
}

// Extract tag and set index from an address
void getAddressComponents(uint64_t addr, int blockBits, int setBits, uint64_t *tag, int *setIndex) {
    if (setBits == 0) {
        *setIndex = 0;
    } else {
        uint64_t mask = ((uint64_t)1 << setBits) - 1ULL;
        *setIndex = (int)((addr >> blockBits) & mask);
    }
    *tag = addr >> (blockBits + setBits);
}

// --- LRU Replacement ---
char accessLRU(Cache *cache, uint64_t addr, int time, int blockBits, int setBits) {
    uint64_t tag;
    int setIndex;
    getAddressComponents(addr, blockBits, setBits, &tag, &setIndex);

    CacheSet *set = &cache->sets[setIndex];

    // Check for hit
    for (int i = 0; i < cache->lineCount; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].lastUsed = time;
            return 'H';
        }
    }

    // Miss: find empty line or least recently used
    int replace = -1;
    for (int i = 0; i < cache->lineCount; i++) {
        if (!set->lines[i].valid) {
            replace = i;
            break;
        }
    }
    if (replace == -1) {
        int oldest = INT_MAX;
        for (int i = 0; i < cache->lineCount; i++) {
            if (set->lines[i].lastUsed < oldest) {
                oldest = set->lines[i].lastUsed;
                replace = i;
            }
        }
    }

    set->lines[replace].valid = 1;
    set->lines[replace].tag = tag;
    set->lines[replace].lastUsed = time;
    return 'M';
}

// --- FIFO Replacement ---
char accessFIFO(Cache *cache, uint64_t addr, int time, int blockBits, int setBits) {
    uint64_t tag;
    int setIndex;
    getAddressComponents(addr, blockBits, setBits, &tag, &setIndex);

    CacheSet *set = &cache->sets[setIndex];

    // Check for hit
    for (int i = 0; i < cache->lineCount; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            return 'H';
        }
    }

    // Miss: find empty line or oldest insertion
    int replace = -1;
    for (int i = 0; i < cache->lineCount; i++) {
        if (!set->lines[i].valid) {
            replace = i;
            break;
        }
    }
    if (replace == -1) {
        int oldest = INT_MAX;
        for (int i = 0; i < cache->lineCount; i++) {
            if (set->lines[i].inserted < oldest) {
                oldest = set->lines[i].inserted;
                replace = i;
            }
        }
    }

    set->lines[replace].valid = 1;
    set->lines[replace].tag = tag;
    set->lines[replace].inserted = time;
    return 'M';
}

// Optimal
char accessOptimal(Cache *cache, uint64_t *addresses, int addrCount, int currentIndex, int blockBits, int setBits) {
    uint64_t addr = addresses[currentIndex];
    uint64_t tag;
    int setIndex;
    getAddressComponents(addr, blockBits, setBits, &tag, &setIndex);

    CacheSet *set = &cache->sets[setIndex];

    // Check for hit
    for (int i = 0; i < cache->lineCount; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            return 'H';
        }
    }

    // Miss: find empty line or optimal replacement
    int replace = -1;
    for (int i = 0; i < cache->lineCount; i++) {
        if (!set->lines[i].valid) {
            replace = i;
            break;
        }
    }

    if (replace == -1) {
        int farthestIndex = -1;
        int farthestPos = -1;
        for (int i = 0; i < cache->lineCount; i++) {
            uint64_t curTag = set->lines[i].tag;
            int nextPos = -1;
            for (int j = currentIndex + 1; j < addrCount; j++) {
                uint64_t ftag;
                int fset;
                getAddressComponents(addresses[j], blockBits, setBits, &ftag, &fset);
                if (fset == setIndex && ftag == curTag) {
                    nextPos = j;
                    break;
                }
            }
            if (nextPos == -1) {
                farthestIndex = i;
                farthestPos = INT_MAX;
                break;
            } else if (nextPos > farthestPos) {
                farthestPos = nextPos;
                farthestIndex = i;
            }
        }
        replace = (farthestIndex == -1) ? 0 : farthestIndex;
    }

    set->lines[replace].valid = 1;
    set->lines[replace].tag = tag;
    return 'M';
}

int main(int argc, char *argv[]) {
    int m = -1, s = -1, e = -1, b = -1;
    char *inputFile = NULL;
    char *policy = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) m = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) s = atoi(argv[++i]);
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) e = atoi(argv[++i]);
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) b = atoi(argv[++i]);
        else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) inputFile = argv[++i];
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) policy = argv[++i];
    }

    if (m == -1 || s == -1 || e == -1 || b == -1 || !inputFile || !policy) {
        fprintf(stderr, "Error: Missing required argument.\n");
        fprintf(stderr, "Usage: ./cachelab -m <m> -s <s> -e <e> -b <b> -i <file> -r <policy>\n");
        return 1;
    }

    int S = 1 << s;
    int E = (e == 0) ? 1 : (1 << e);
    int B = 1 << b;

    FILE *fp = fopen(inputFile, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", inputFile);
        return 1;
    }

    // Read addresses
    size_t cap = 1024;
    size_t addrCount = 0;
    uint64_t *addresses = (uint64_t *)malloc(cap * sizeof(uint64_t));
    if (!addresses) { fprintf(stderr, "Memory allocation failed\n"); fclose(fp); return 1; }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') continue;
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        uint64_t val = strtoull(p, NULL, 16);
        if (addrCount >= cap) {
            cap *= 2;
            addresses = (uint64_t *)realloc(addresses, cap * sizeof(uint64_t));
            if (!addresses) { fprintf(stderr, "Memory allocation failed\n"); fclose(fp); return 1; }
        }
        addresses[addrCount++] = val;
    }
    fclose(fp);

    Cache cache = initCache(S, E, B);
    int hits = 0, misses = 0;
    int time = 0;

    for (size_t i = 0; i < addrCount; i++) {
        uint64_t addr = addresses[i];
        time++;
        char result = 'M';

        if (strcmp(policy, "lru") == 0) {
            result = accessLRU(&cache, addr, time, b, s);
        } else if (strcmp(policy, "fifo") == 0) {
            result = accessFIFO(&cache, addr, time, b, s);
        } else if (strcmp(policy, "optimal") == 0) {
            result = accessOptimal(&cache, addresses, (int)addrCount, (int)i, b, s);
        } else {
            fprintf(stderr, "Unknown policy '%s'\n", policy);
            freeCache(&cache);
            free(addresses);
            return 1;
        }

        printf("%llX %c\n", (unsigned long long)addr, result);
        if (result == 'H') hits++;
        else misses++;
    }

    void printResult(int hits, int misses, int missRatePercent, int totalCycle) {
        printf("[result] hits: %d misses: %d miss rate: %d%% total running time: %d cycle\n",
               hits, misses, missRatePercent, totalCycle);
    }


    int total = hits + misses;
    double missRate = (total > 0) ? (double)misses / total : 0.0;
    int missRatePercent = (int)(missRate * 100);
    int avgAccessTime = (int)(HIT_TIME + missRate * MISS_PENALTY);
    int totalCycle = (int)(total * avgAccessTime);

    printResult(hits, misses, missRatePercent, totalCycle);

    freeCache(&cache);
    free(addresses);
    return 0;
}