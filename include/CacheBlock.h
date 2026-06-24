#pragma once

struct CacheBlock {
    bool          valid;
    bool          dirty;
    unsigned long tag;
    int           frequency;   // LFU
    int           insertOrder; // FIFO
    int           lastUsed;    // LRU

    CacheBlock() : valid(false), dirty(false), tag(0),
                   frequency(0), insertOrder(0), lastUsed(0) {}

    void reset() {
        valid = dirty = false;
        tag = 0;
        frequency = insertOrder = lastUsed = 0;
    }
};
