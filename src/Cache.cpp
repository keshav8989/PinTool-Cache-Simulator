#include "Cache.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

void Cache::configure(const CacheConfig& cfg) {
    config = cfg;
    config.compute();
    timer = 0;
    stats.reset();
    int w = (config.ways == 0) ? config.numBlocks : config.ways;
    sets.assign(config.numSets, std::vector<CacheBlock>(w));
}

void Cache::decode(unsigned long addr,
                   unsigned long& tag,
                   unsigned long& setIdx,
                   unsigned long& offset) const {
    unsigned long offMask = (1UL << config.offsetBits) - 1;
    unsigned long idxMask = (1UL << config.indexBits)  - 1;
    offset = addr & offMask;
    setIdx = (addr >> config.offsetBits) & idxMask;
    tag    =  addr >> (config.offsetBits + config.indexBits);
}

int Cache::findBlock(int s, unsigned long tag) const {
    const std::vector<CacheBlock>& set = sets[s];
    for (int w = 0; w < (int)set.size(); w++)
        if (set[w].valid && set[w].tag == tag) return w;
    return -1;
}

int Cache::findVictim(int s) {
    std::vector<CacheBlock>& set = sets[s];
    // Empty slot first
    for (int w = 0; w < (int)set.size(); w++)
        if (!set[w].valid) return w;

    int victim = 0;
    if (config.policy == LRU) {
        for (int w = 1; w < (int)set.size(); w++)
            if (set[w].lastUsed < set[victim].lastUsed) victim = w;
    } else if (config.policy == FIFO) {
        for (int w = 1; w < (int)set.size(); w++)
            if (set[w].insertOrder < set[victim].insertOrder) victim = w;
    } else if (config.policy == LFU) {
        for (int w = 1; w < (int)set.size(); w++) {
            if (set[w].frequency < set[victim].frequency) victim = w;
            else if (set[w].frequency == set[victim].frequency &&
                     set[w].lastUsed  <  set[victim].lastUsed)  victim = w;
        }
    } else {
        victim = rand() % (int)set.size();
    }
    return victim;
}

void Cache::install(int s, int w, unsigned long tag, bool dirty) {
    CacheBlock& b = sets[s][w];
    if (b.valid && b.dirty) stats.writebacks++;
    b.valid       = true;
    b.dirty       = dirty;
    b.tag         = tag;
    b.lastUsed    = ++timer;
    b.insertOrder = timer;
    b.frequency   = 1;
}

bool Cache::access(unsigned long address, bool isWrite, bool verbose) {
    unsigned long tag, setIdx, offset;
    decode(address, tag, setIdx, offset);

    stats.total++;
    if (isWrite) stats.writes++;
    else         stats.reads++;

    int way = findBlock((int)setIdx, tag);
    if (way >= 0) {
        // HIT
        stats.hits++;
        CacheBlock& b = sets[setIdx][way];
        b.lastUsed = ++timer;
        b.frequency++;
        if (isWrite) b.dirty = (config.writePolicy == WRITE_BACK);

        if (verbose) {
            std::cout << "  0x" << std::hex << std::uppercase
                      << std::setw(8) << std::setfill('0') << address
                      << std::dec << std::setfill(' ')
                      << "  Set=" << setIdx << "  Way=" << way
                      << "  ->  HIT\n";
        }
        return true;
    }

    // MISS
    stats.misses++;
    int victim = findVictim((int)setIdx);
    bool dirty = isWrite && (config.writePolicy == WRITE_BACK);
    install((int)setIdx, victim, tag, dirty);

    if (verbose) {
        std::cout << "  0x" << std::hex << std::uppercase
                  << std::setw(8) << std::setfill('0') << address
                  << std::dec << std::setfill(' ')
                  << "  Set=" << setIdx << "  Way=" << victim
                  << "  ->  MISS\n";
    }
    return false;
}

void Cache::showCache() const {
    int ways = (config.ways == 0) ? config.numBlocks : config.ways;
    std::string line(52, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  CACHE  [" << config.typeName() << " | "
              << config.policyName() << " | " << config.writeName() << "]\n";
    std::cout << line << "\n";
    for (int s = 0; s < config.numSets; s++) {
        std::cout << "\n  Set " << s << "\n";
        for (int w = 0; w < ways; w++) {
            const CacheBlock& b = sets[s][w];
            std::cout << "    Block " << w << "  | ";
            if (b.valid)
                std::cout << "Tag:" << toBin(b.tag, config.tagBits)
                          << "  Valid:1  Dirty:" << b.dirty
                          << "  Freq:" << b.frequency << "\n";
            else
                std::cout << "Tag:----  Valid:0\n";
        }
    }
    std::cout << "\n" << line << "\n";
}

void Cache::showStats() const {
    std::string line(42, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  CACHE STATISTICS\n";
    std::cout << line << "\n";
    std::cout << "  Total Accesses  : " << stats.total      << "\n";
    std::cout << "  Hits            : " << stats.hits       << "\n";
    std::cout << "  Misses          : " << stats.misses     << "\n";
    std::cout << "  Reads           : " << stats.reads      << "\n";
    std::cout << "  Writes          : " << stats.writes     << "\n";
    std::cout << "  Writebacks      : " << stats.writebacks << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Hit Rate        : " << stats.hitRate()  << "%\n";
    std::cout << "  Miss Rate       : " << stats.missRate() << "%\n";
    std::cout << line << "\n";
    std::cout << "  AMAT = " << stats.hitTime << " + "
              << std::setprecision(3) << (stats.missRate()/100.0)
              << " x " << stats.missPenalty << " = "
              << std::setprecision(2) << stats.amat() << " ns\n";
    std::cout << line << "\n";
}

void Cache::analyzeAddress(unsigned long addr) const {
    unsigned long tag, setIdx, offset;
    decode(addr, tag, setIdx, offset);
    std::string line(44, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  ADDRESS: 0x" << std::hex << std::uppercase
              << std::setw(8) << std::setfill('0') << addr
              << std::dec << std::setfill(' ') << "\n";
    std::cout << line << "\n";
    std::cout << "  Binary  : " << toBin(addr, 32) << "\n";
    std::cout << "  Tag     : " << toBin(tag,    config.tagBits)
              << "  (" << config.tagBits    << " bits)\n";
    std::cout << "  Index   : " << toBin(setIdx, config.indexBits)
              << "  (" << config.indexBits  << " bits)\n";
    std::cout << "  Offset  : " << toBin(offset, config.offsetBits)
              << "  (" << config.offsetBits << " bits)\n";
    std::cout << "  Set     : " << setIdx  << "\n";
    std::cout << line << "\n";
}

std::string Cache::toBin(unsigned long val, int bits) const {
    if (bits <= 0) bits = 8;
    std::string r(bits, '0');
    for (int i = bits-1; i >= 0; i--) {
        r[i] = (val & 1) ? '1' : '0';
        val >>= 1;
    }
    return r;
}
