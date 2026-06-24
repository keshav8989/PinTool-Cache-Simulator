#pragma once
#include <string>

// =============================================
//  ONE LINE FROM mem.trace
//  Format: PC, Read/Write, Addr, Distance, Reuse
// =============================================
struct TraceEntry {
    unsigned long pc;
    bool          isWrite;
    unsigned long addr;
    long long     distance;   // -1 = first use
    long long     reuse;      // cumulative reuse count

    TraceEntry() : pc(0), isWrite(false), addr(0), distance(-1), reuse(0) {}
};
