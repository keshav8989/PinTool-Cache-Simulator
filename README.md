# CPU Cache Simulator + Pin Tool Trace Analyzer

## Files
- memtrace.cpp       — Fixed Pin tool (correct cumulative reuse count)
- src/Cache.cpp      — Cache simulation (LRU/FIFO/LFU/Random, Write-Back/Through)
- src/TraceAnalyzer.cpp — Parses mem.trace, computes avg distance + top reused addrs
- src/Simulator.cpp  — REPL tying everything together
- src/main.cpp       — Entry point
- tests/test1.c      — Static array test program
- tests/test2.c      — Dynamic malloc test program
- traces/mem.trace   — Sample generated trace

## Build Cache Simulator (Windows, no Pin needed)
  g++ -std=c++14 -O2 -Iinclude src/TraceAnalyzer.cpp src/Cache.cpp src/Simulator.cpp src/main.cpp -o cachesim

## Run
  .\cachesim.exe

## Key Commands
  LOAD_TRACE traces/mem.trace   <- load Pin tool output
  TRACE_SUMMARY                 <- avg distance + top 5 reused addresses
  SHOW_TRACE 20                 <- print first 20 trace lines
  REUSE_HISTOGRAM               <- reuse distribution
  RUN_TRACE                     <- simulate trace through cache
  COMPARE                       <- LRU vs FIFO vs LFU vs Random

## Pin Tool (Linux with Pin installed)
  make
  $PIN_ROOT/pin -t memtrace.so -- ./test1
  $PIN_ROOT/pin -t memtrace.so -- ./test2
  # Generates mem.trace + prints avg distance + top 5 reused addresses

## Bug Fixed in original memtrace.cpp
  Original: reuse was always 0 or 1 (not cumulative)
  Fixed:    reuseCount[addr]++ gives true cumulative reuse count
  Fixed:    insCount increments on ALL instructions (not just memory)
            for accurate distance calculation
