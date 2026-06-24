#pragma once
#include "TraceEntry.h"
#include <string>
#include <vector>
#include <unordered_map>

// =============================================
//  TRACE ANALYZER
//  - Parses mem.trace file
//  - Computes average distance
//  - Finds top-N hottest addresses (by reuse)
//  - Feeds accesses into cache simulator
// =============================================
class TraceAnalyzer {
public:
    TraceAnalyzer() : totalDistance(0), validDistances(0) {}

    // Load and parse the trace file
    bool load(const std::string& filename);

    // Print summary: avg distance + top-N reused addresses
    void printSummary(int topN = 5) const;

    // Print first N lines of trace
    void printTrace(int n = 20) const;

    // Print reuse histogram
    void printReuseHistogram() const;

    // Getters
    const std::vector<TraceEntry>& getEntries() const { return entries; }
    size_t size() const { return entries.size(); }
    double avgDistance() const;

private:
    std::vector<TraceEntry> entries;

    // addr -> max reuse seen (from trace itself)
    std::unordered_map<unsigned long, long long> maxReuse;

    long long totalDistance;
    long long validDistances;

    // Parse one hex string like "0x1000" or "1000"
    static unsigned long parseHex(const std::string& s);
    static std::string   trim(const std::string& s);
    std::string toHexStr(unsigned long val) const;
};
