#pragma once
#include "CacheBlock.h"
#include "CacheConfig.h"
#include <vector>
#include <string>

struct Stats {
    long long hits, misses, reads, writes, writebacks, total;
    double    hitTime, missPenalty;

    Stats() : hits(0), misses(0), reads(0), writes(0),
              writebacks(0), total(0),
              hitTime(2.0), missPenalty(50.0) {}

    void reset() { hits=misses=reads=writes=writebacks=total=0; }

    double hitRate()  const { return total?100.0*hits/total:0.0; }
    double missRate() const { return total?100.0*misses/total:0.0; }
    double amat()     const { return hitTime + (missRate()/100.0)*missPenalty; }
};

class Cache {
public:
    Cache() : timer(0) {}

    void configure(const CacheConfig& cfg);

    // Returns true on hit
    bool access(unsigned long address, bool isWrite, bool verbose);

    void showCache()  const;
    void showStats()  const;
    void resetStats()       { stats.reset(); }
    void analyzeAddress(unsigned long addr) const;

    const Stats&       getStats()  const { return stats; }
    const CacheConfig& getConfig() const { return config; }

private:
    CacheConfig config;
    Stats       stats;
    std::vector<std::vector<CacheBlock>> sets;
    int timer;

    void          decode(unsigned long addr,
                         unsigned long& tag,
                         unsigned long& setIdx,
                         unsigned long& offset) const;
    int           findBlock(int setIdx, unsigned long tag) const;
    int           findVictim(int setIdx);
    void          install(int setIdx, int way,
                          unsigned long tag, bool dirty);
    std::string   toBin(unsigned long val, int bits) const;
};
