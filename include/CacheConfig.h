#pragma once
#include <string>

enum Policy      { LRU, FIFO, LFU, RANDOM_P };
enum WritePolicy { WRITE_BACK, WRITE_THROUGH };

struct CacheConfig {
    int cacheSize;   // bytes
    int blockSize;   // bytes
    int ways;        // 1=direct, 0=fully-assoc, N=N-way
    Policy      policy;
    WritePolicy writePolicy;

    // Derived (call compute())
    int numBlocks, numSets;
    int offsetBits, indexBits, tagBits;

    CacheConfig()
        : cacheSize(1024), blockSize(64), ways(2),
          policy(LRU), writePolicy(WRITE_BACK),
          numBlocks(0), numSets(0),
          offsetBits(0), indexBits(0), tagBits(0) {}

    void compute() {
        numBlocks  = cacheSize / blockSize;
        numSets    = (ways == 0) ? 1 : numBlocks / ways;
        if (numSets < 1) numSets = 1;
        offsetBits = log2i(blockSize);
        indexBits  = log2i(numSets);
        tagBits    = 32 - offsetBits - indexBits;
        if (tagBits < 0) tagBits = 0;
    }

    static int log2i(int n) {
        int c = 0; while (n > 1) { n >>= 1; c++; } return c;
    }

    std::string policyName() const {
        if (policy == LRU)      return "LRU";
        if (policy == FIFO)     return "FIFO";
        if (policy == LFU)      return "LFU";
        return "Random";
    }
    std::string writeName() const {
        return (writePolicy == WRITE_BACK) ? "Write-Back" : "Write-Through";
    }
    std::string typeName() const {
        if (ways == 1) return "Direct Mapped";
        if (ways == 0) return "Fully Associative";
        return std::to_string(ways) + "-Way Set Associative";
    }
};
