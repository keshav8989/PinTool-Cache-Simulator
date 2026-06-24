#include "Simulator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

// =============================================
//  CONSTRUCTOR
// =============================================
Simulator::Simulator() : traceLoaded(false) {
    CacheConfig cfg;
    cfg.cacheSize   = 1024;
    cfg.blockSize   = 64;
    cfg.ways        = 2;
    cfg.policy      = LRU;
    cfg.writePolicy = WRITE_BACK;
    cfg.compute();
    cache.configure(cfg);
}

// =============================================
//  HELPERS
// =============================================
std::string Simulator::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

std::string Simulator::toUpper(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        r[i] = (char)std::toupper((unsigned char)r[i]);
    return r;
}

unsigned long Simulator::parseAddr(const std::string& s) {
    std::string t = trim(s);
    if (t.size()>2 && t[0]=='0' && (t[1]=='x'||t[1]=='X')) t = t.substr(2);
    unsigned long v = 0;
    for (size_t i = 0; i < t.size(); i++) {
        char c = t[i];
        int d = 0;
        if (c>='0'&&c<='9') d=c-'0';
        else if (c>='a'&&c<='f') d=10+c-'a';
        else if (c>='A'&&c<='F') d=10+c-'A';
        v = v*16+d;
    }
    return v;
}

// =============================================
//  LOAD / STORE
// =============================================
void Simulator::doLoad(const std::string& a) {
    cache.access(parseAddr(a), false, true);
}
void Simulator::doStore(const std::string& a) {
    cache.access(parseAddr(a), true, true);
}

// =============================================
//  LOAD TRACE from file
// =============================================
void Simulator::doLoadTrace(const std::string& filename) {
    traceLoaded = tracer.load(filename);
    if (traceLoaded)
        std::cout << "  Use RUN_TRACE to simulate all accesses through cache.\n"
                  << "  Use TRACE_SUMMARY to see Pin tool analysis.\n"
                  << "  Use SHOW_TRACE [N] to print first N trace lines.\n";
}

// =============================================
//  RUN TRACE through cache
// =============================================
void Simulator::doRunTrace() {
    if (!traceLoaded) {
        std::cout << "  [!] No trace loaded. Use LOAD_TRACE <file> first.\n";
        return;
    }

    cache.resetStats();
    const std::vector<TraceEntry>& entries = tracer.getEntries();

    std::cout << "\n  Running " << entries.size()
              << " trace entries through cache...\n";
    std::cout << "  (verbose off for speed — use LOAD/STORE for single access)\n\n";

    long long hitStreak = 0, maxHitStreak = 0;
    long long missStreak = 0, maxMissStreak = 0;

    for (size_t i = 0; i < entries.size(); i++) {
        bool hit = cache.access(entries[i].addr, entries[i].isWrite, false);
        if (hit) {
            hitStreak++;
            missStreak = 0;
            if (hitStreak > maxHitStreak) maxHitStreak = hitStreak;
        } else {
            missStreak++;
            hitStreak = 0;
            if (missStreak > maxMissStreak) maxMissStreak = missStreak;
        }
    }

    cache.showStats();

    // Extra insight
    std::string line(42,'-');
    std::cout << line << "\n";
    std::cout << "  TRACE INSIGHTS\n";
    std::cout << line << "\n";
    std::cout << "  Max consecutive hits   : " << maxHitStreak  << "\n";
    std::cout << "  Max consecutive misses : " << maxMissStreak << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Avg reuse (from trace) : " << tracer.avgDistance()
              << " instructions\n";
    std::cout << line << "\n";
}

// =============================================
//  COMPARE all 4 policies on loaded trace
// =============================================
void Simulator::doCompare() {
    std::string line(54,'-');
    std::cout << "\n" << line << "\n";
    std::cout << "  POLICY COMPARISON";
    if (traceLoaded)
        std::cout << " (on loaded trace, " << tracer.size() << " accesses)";
    std::cout << "\n" << line << "\n";

    Policy policies[] = { LRU, FIFO, LFU, RANDOM_P };
    const char* names[] = { "LRU", "FIFO", "LFU", "Random" };

    CacheConfig base = cache.getConfig();

    std::cout << std::left
              << "  " << std::setw(8)  << "Policy"
              << std::setw(10) << "Accesses"
              << std::setw(8)  << "Hits"
              << std::setw(8)  << "Misses"
              << std::setw(10) << "Hit%"
              << "AMAT(ns)\n";
    std::cout << "  " << std::string(50,'-') << "\n";

    for (int p = 0; p < 4; p++) {
        CacheConfig cfg = base;
        cfg.policy = policies[p];
        cfg.compute();

        Cache tc;
        tc.configure(cfg);

        if (traceLoaded) {
            const std::vector<TraceEntry>& ent = tracer.getEntries();
            for (size_t i = 0; i < ent.size(); i++)
                tc.access(ent[i].addr, ent[i].isWrite, false);
        } else {
            // Synthetic pattern
            unsigned long addrs[] = {
                0x1000,0x1004,0x1008,0x2000,0x1000,0x3000,
                0x1004,0x4000,0x1000,0x2000,0x5000,0x1008
            };
            for (int i = 0; i < 12; i++)
                tc.access(addrs[i], false, false);
        }

        const Stats& s = tc.getStats();
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  " << std::left << std::setw(8) << names[p]
                  << std::setw(10) << s.total
                  << std::setw(8)  << s.hits
                  << std::setw(8)  << s.misses
                  << std::setw(9)  << s.hitRate() << "%  "
                  << std::setprecision(2) << s.amat() << "\n";
    }
    std::cout << line << "\n";
}

// =============================================
//  SET CONFIG
// =============================================
void Simulator::setConfig(const std::string& args) {
    CacheConfig cfg = cache.getConfig();
    std::istringstream iss(args);
    std::string tok;
    while (iss >> tok) {
        size_t eq = tok.find('=');
        if (eq == std::string::npos) continue;
        std::string key = toUpper(tok.substr(0, eq));
        std::string val = toUpper(tok.substr(eq+1));
        if (key=="SIZE") {
            int sz = std::atoi(val.c_str());
            if (val.find("KB")!=std::string::npos) sz*=1024;
            cfg.cacheSize = sz;
        }
        else if (key=="BLOCK") cfg.blockSize = std::atoi(val.c_str());
        else if (key=="WAYS") {
            cfg.ways = std::atoi(val.c_str());
        }
        else if (key=="POLICY") {
            if (val=="LRU")         cfg.policy = LRU;
            else if (val=="FIFO")   cfg.policy = FIFO;
            else if (val=="LFU")    cfg.policy = LFU;
            else if (val=="RANDOM") cfg.policy = RANDOM_P;
        }
        else if (key=="WRITE") {
            cfg.writePolicy = (val=="WT"||val=="WRITETHROUGH")
                              ? WRITE_THROUGH : WRITE_BACK;
        }
    }
    cfg.compute();
    cache.configure(cfg);
    std::cout << "  Config: " << cfg.typeName() << " | "
              << cfg.policyName() << " | " << cfg.writeName()
              << " | " << cfg.cacheSize << "B | Block=" << cfg.blockSize << "B\n";
}

void Simulator::printConfig() const {
    const CacheConfig& c = cache.getConfig();
    std::string line(42,'-');
    std::cout << "\n" << line << "\n";
    std::cout << "  CONFIGURATION\n" << line << "\n";
    std::cout << "  Type        : " << c.typeName()   << "\n";
    std::cout << "  Cache Size  : " << c.cacheSize << " B\n";
    std::cout << "  Block Size  : " << c.blockSize << " B\n";
    std::cout << "  Ways        : " << ((c.ways==0)?"Fully Assoc":std::to_string(c.ways)) << "\n";
    std::cout << "  Sets        : " << c.numSets   << "\n";
    std::cout << "  Policy      : " << c.policyName()  << "\n";
    std::cout << "  Write       : " << c.writeName()   << "\n";
    std::cout << "  Tag bits    : " << c.tagBits    << "\n";
    std::cout << "  Index bits  : " << c.indexBits  << "\n";
    std::cout << "  Offset bits : " << c.offsetBits << "\n";
    std::cout << line << "\n";
}

// =============================================
//  BANNER
// =============================================
void Simulator::printBanner() const {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════════╗\n";
    std::cout << "  ║   CPU Cache Simulator + Pin Trace Analyzer ║\n";
    std::cout << "  ║   DSA: Hash Map | LRU/FIFO/LFU | Heap     ║\n";
    std::cout << "  ║   Integrates with Intel Pin mem.trace      ║\n";
    std::cout << "  ╚═══════════════════════════════════════════╝\n";
}

// =============================================
//  HELP
// =============================================
void Simulator::printHelp() const {
    std::cout << "\n";
    std::cout << "  =====================================================\n";
    std::cout << "   COMMANDS\n";
    std::cout << "  =====================================================\n";
    std::cout << "   --- Manual Access ---\n";
    std::cout << "   LOAD  <addr>            Read from address\n";
    std::cout << "   STORE <addr>            Write to address\n";
    std::cout << "\n";
    std::cout << "   --- Pin Trace Integration ---\n";
    std::cout << "   LOAD_TRACE <file>       Load mem.trace from Pin tool\n";
    std::cout << "   TRACE_SUMMARY           Avg distance + top reused addrs\n";
    std::cout << "   SHOW_TRACE [N]          Print first N trace lines\n";
    std::cout << "   REUSE_HISTOGRAM         Show reuse distribution\n";
    std::cout << "   RUN_TRACE               Run trace through cache\n";
    std::cout << "\n";
    std::cout << "   --- Cache Analysis ---\n";
    std::cout << "   SHOW_CACHE              Display cache contents\n";
    std::cout << "   STATS                   Hit/miss + AMAT\n";
    std::cout << "   ANALYZE <addr>          Tag/Index/Offset breakdown\n";
    std::cout << "   COMPARE                 LRU vs FIFO vs LFU vs Random\n";
    std::cout << "\n";
    std::cout << "   --- Config ---\n";
    std::cout << "   CONFIG                  Show current config\n";
    std::cout << "   SET ways=4 policy=LRU write=WB size=2048 block=64\n";
    std::cout << "       policy: LRU FIFO LFU RANDOM\n";
    std::cout << "       write:  WB (write-back)  WT (write-through)\n";
    std::cout << "   RESET                   Reset stats\n";
    std::cout << "   HELP                    This help\n";
    std::cout << "   EXIT                    Quit\n";
    std::cout << "  =====================================================\n";
}

// =============================================
//  HANDLE COMMAND
// =============================================
void Simulator::handleCommand(const std::string& raw) {
    if (raw.empty()) return;
    std::string up = toUpper(raw);

    if (up.size()>5  && up.substr(0,5)  == "LOAD "  && up.substr(0,10) != "LOAD_TRACE") {
        doLoad(trim(raw.substr(5))); return;
    }
    if (up.size()>6  && up.substr(0,6)  == "STORE ") { doStore(trim(raw.substr(6))); return; }
    if (up.size()>11 && up.substr(0,11) == "LOAD_TRACE ") { doLoadTrace(trim(raw.substr(11))); return; }
    if (up == "TRACE_SUMMARY")      { tracer.printSummary(5); return; }
    if (up == "REUSE_HISTOGRAM")    { tracer.printReuseHistogram(); return; }
    if (up == "RUN_TRACE")          { doRunTrace(); return; }
    if (up.size()>11 && up.substr(0,11) == "SHOW_TRACE ") {
        int n = std::atoi(trim(raw.substr(11)).c_str());
        tracer.printTrace(n > 0 ? n : 20); return;
    }
    if (up == "SHOW_TRACE")         { tracer.printTrace(20); return; }
    if (up == "SHOW_CACHE")         { cache.showCache(); return; }
    if (up == "STATS")              { cache.showStats(); return; }
    if (up.size()>8 && up.substr(0,8) == "ANALYZE ") {
        cache.analyzeAddress(parseAddr(trim(raw.substr(8)))); return;
    }
    if (up == "COMPARE")            { doCompare(); return; }
    if (up == "CONFIG")             { printConfig(); return; }
    if (up.size()>4 && up.substr(0,4) == "SET ") { setConfig(trim(raw.substr(4))); return; }
    if (up == "RESET")              { cache.resetStats(); std::cout<<"  Stats reset.\n"; return; }
    if (up == "HELP")               { printHelp(); return; }
    if (up == "EXIT" || up == "QUIT") { std::cout<<"\n  Goodbye!\n\n"; exit(0); }

    std::cout << "  [!] Unknown command. Type HELP.\n";
}

// =============================================
//  MAIN REPL
// =============================================
void Simulator::run() {
    printBanner();
    printHelp();
    std::string line;
    while (true) {
        std::cout << "\n  > ";
        if (!std::getline(std::cin, line)) break;
        handleCommand(trim(line));
    }
}
