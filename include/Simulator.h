#pragma once
#include "Cache.h"
#include "TraceAnalyzer.h"
#include <string>

class Simulator {
public:
    Simulator();
    void run();

private:
    Cache         cache;
    TraceAnalyzer tracer;
    bool          traceLoaded;

    void handleCommand(const std::string& line);
    void doLoad (const std::string& addrStr);
    void doStore(const std::string& addrStr);
    void doLoadTrace(const std::string& filename);
    void doRunTrace();
    void doCompare();
    void setConfig(const std::string& args);
    void printConfig() const;
    void printBanner() const;
    void printHelp()   const;

    static unsigned long parseAddr(const std::string& s);
    static std::string   trim(const std::string& s);
    static std::string   toUpper(const std::string& s);
};
