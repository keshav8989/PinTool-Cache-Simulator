#include "TraceAnalyzer.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// =============================================
//  HELPERS
// =============================================
std::string TraceAnalyzer::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

unsigned long TraceAnalyzer::parseHex(const std::string& s) {
    std::string t = trim(s);
    if (t.size() > 2 && t[0]=='0' && (t[1]=='x'||t[1]=='X'))
        t = t.substr(2);
    unsigned long val = 0;
    for (size_t i = 0; i < t.size(); i++) {
        char c = t[i];
        int d = 0;
        if (c>='0' && c<='9') d = c-'0';
        else if (c>='a' && c<='f') d = 10+c-'a';
        else if (c>='A' && c<='F') d = 10+c-'A';
        val = val*16 + d;
    }
    return val;
}

// =============================================
//  LOAD TRACE FILE
//  Format per line: PC, Read/Write, Addr, Distance, Reuse
// =============================================
bool TraceAnalyzer::load(const std::string& filename) {
    std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
        std::cerr << "  [ERROR] Cannot open: " << filename << "\n";
        return false;
    }

    entries.clear();
    maxReuse.clear();
    totalDistance  = 0;
    validDistances = 0;

    std::string line;
    int lineNum = 0;

    while (std::getline(fin, line)) {
        lineNum++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Split by comma
        std::vector<std::string> fields;
        std::istringstream iss(line);
        std::string tok;
        while (std::getline(iss, tok, ','))
            fields.push_back(trim(tok));

        if (fields.size() < 5) continue;   // skip malformed lines

        TraceEntry e;
        e.pc       = parseHex(fields[0]);
        e.isWrite  = (fields[1] == "Write" || fields[1] == "write" || fields[1] == "W");
        e.addr     = parseHex(fields[2]);
        e.distance = std::atoll(fields[3].c_str());
        e.reuse    = std::atoll(fields[4].c_str());

        entries.push_back(e);

        // Track max reuse per address
        if (maxReuse.find(e.addr) == maxReuse.end() || e.reuse > maxReuse[e.addr])
            maxReuse[e.addr] = e.reuse;

        // Accumulate distance stats (skip first-use: distance == -1)
        if (e.distance >= 0) {
            totalDistance += e.distance;
            validDistances++;
        }
    }
    fin.close();

    std::cout << "  Trace loaded: " << entries.size()
              << " entries from \"" << filename << "\"\n";
    return true;
}

// =============================================
//  AVERAGE DISTANCE
// =============================================
double TraceAnalyzer::avgDistance() const {
    if (validDistances == 0) return 0.0;
    return (double)totalDistance / (double)validDistances;
}

// =============================================
//  PRINT SUMMARY (matches Pin tool stdout output)
// =============================================
void TraceAnalyzer::printSummary(int topN) const {
    if (entries.empty()) {
        std::cout << "  [!] No trace loaded.\n";
        return;
    }

    long long reads = 0, writes = 0, firstUse = 0;
    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].isWrite) writes++;
        else                    reads++;
        if (entries[i].distance == -1) firstUse++;
    }

    std::string line(50, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  TRACE ANALYSIS SUMMARY\n";
    std::cout << line << "\n";
    std::cout << "  Total accesses     : " << entries.size()   << "\n";
    std::cout << "  Reads              : " << reads             << "\n";
    std::cout << "  Writes             : " << writes            << "\n";
    std::cout << "  First-use (dist=-1): " << firstUse          << "\n";
    std::cout << "  Reuse accesses     : " << validDistances    << "\n";
    std::cout << "  Unique addresses   : " << maxReuse.size()   << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Avg Distance       : ";
    if (validDistances > 0)
        std::cout << avgDistance() << " instructions\n";
    else
        std::cout << "N/A (no reuse)\n";

    // Top-N hottest addresses by reuse count
    std::vector<std::pair<unsigned long, long long>> vec(maxReuse.begin(), maxReuse.end());
    std::sort(vec.begin(), vec.end(),
        [](const std::pair<unsigned long,long long>& a,
           const std::pair<unsigned long,long long>& b){
            return a.second > b.second;
        });

    std::cout << "\n  Top " << topN << " addresses with maximum reuse:\n";
    std::cout << "  " << std::string(36,'-') << "\n";
    std::cout << "  " << std::left << std::setw(16) << "Address"
              << std::setw(10) << "Reuse" << "Type\n";
    std::cout << "  " << std::string(36,'-') << "\n";

    for (int i = 0; i < topN && i < (int)vec.size(); i++) {
        // Determine last access type for this address
        std::string accType = "Read";
        for (int j = (int)entries.size()-1; j >= 0; j--) {
            if (entries[j].addr == vec[i].first) {
                accType = entries[j].isWrite ? "Write" : "Read";
                break;
            }
        }
        std::cout << "  " << std::left << std::setw(16)
                  << ("0x" + toHexStr(vec[i].first))
                  << std::setw(10) << vec[i].second
                  << accType << "\n";
    }
    std::cout << line << "\n";
}

// =============================================
//  PRINT FIRST N LINES OF TRACE
// =============================================
void TraceAnalyzer::printTrace(int n) const {
    if (entries.empty()) {
        std::cout << "  [!] No trace loaded.\n";
        return;
    }

    int show = (n < (int)entries.size()) ? n : (int)entries.size();
    std::string line(62, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  TRACE (first " << show << " of " << entries.size() << " entries)\n";
    std::cout << line << "\n";
    std::cout << "  " << std::left
              << std::setw(12) << "PC"
              << std::setw(8)  << "R/W"
              << std::setw(14) << "Address"
              << std::setw(10) << "Distance"
              << "Reuse\n";
    std::cout << "  " << std::string(58,'-') << "\n";

    for (int i = 0; i < show; i++) {
        const TraceEntry& e = entries[i];
        std::cout << "  "
                  << std::left  << std::setw(12) << ("0x"+toHexStr(e.pc))
                  << std::setw(8)  << (e.isWrite ? "Write" : "Read")
                  << std::setw(14) << ("0x"+toHexStr(e.addr))
                  << std::setw(10) << e.distance
                  << e.reuse << "\n";
    }
    std::cout << line << "\n";
}

// =============================================
//  REUSE HISTOGRAM  (bucket by reuse count)
// =============================================
void TraceAnalyzer::printReuseHistogram() const {
    if (entries.empty()) {
        std::cout << "  [!] No trace loaded.\n";
        return;
    }

    // Count addresses by reuse bucket: 0, 1-5, 6-20, 21+
    int b0=0, b1=0, b2=0, b3=0;
    for (std::unordered_map<unsigned long,long long>::const_iterator it
             = maxReuse.begin(); it != maxReuse.end(); ++it) {
        long long r = it->second;
        if (r == 0)       b0++;
        else if (r <= 5)  b1++;
        else if (r <= 20) b2++;
        else              b3++;
    }

    int total = b0 + b1 + b2 + b3;
    std::string line(44, '-');
    std::cout << "\n" << line << "\n";
    std::cout << "  REUSE HISTOGRAM\n";
    std::cout << line << "\n";
    std::cout << "  Bucket     Count   Bar\n";
    std::cout << "  " << std::string(40,'-') << "\n";

    auto bar = [](int n, int tot) -> std::string {
        int w = tot ? (n * 20 / tot) : 0;
        return std::string(w, '#');
    };

    std::cout << "  Reuse=0    " << std::setw(6) << b0 << "  " << bar(b0,total) << "\n";
    std::cout << "  Reuse 1-5  " << std::setw(6) << b1 << "  " << bar(b1,total) << "\n";
    std::cout << "  Reuse 6-20 " << std::setw(6) << b2 << "  " << bar(b2,total) << "\n";
    std::cout << "  Reuse 21+  " << std::setw(6) << b3 << "  " << bar(b3,total) << "\n";
    std::cout << line << "\n";
}

// helper: unsigned long to hex string
std::string TraceAnalyzer::toHexStr(unsigned long val) const {
    if (val == 0) return "0";
    std::string r;
    const char* h = "0123456789abcdef";
    while (val) { r = h[val&0xf] + r; val >>= 4; }
    return r;
}
