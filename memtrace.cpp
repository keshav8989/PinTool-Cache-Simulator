/*
Your Name, RollNo
*/

#include "pin.H"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

ofstream outfile;

// Global instruction counter (counts ALL instructions, not just memory ones)
UINT64 insCount = 0;

// Map: effective memory address -> instruction number of last access
unordered_map<ADDRINT, UINT64> lastAccess;

// Map: effective memory address -> total reuse count (cumulative)
unordered_map<ADDRINT, UINT64> reuseCount;

// For average distance calculation (excludes first-use)
UINT64 totalDistance  = 0;
UINT64 validDistances = 0;  // count of non-first-use accesses

// -------------------------------------------------------
//  Called BEFORE every memory instruction
// -------------------------------------------------------
VOID RecordMemAccess(ADDRINT pc, ADDRINT addr, BOOL isWrite)
{
    // Distance = instructions since last access to THIS address
    INT64 distance = -1;   // -1 means first use

    if (lastAccess.count(addr))
    {
        // Not first use: compute distance
        distance = (INT64)(insCount - lastAccess[addr]);
        totalDistance += distance;
        validDistances++;

        // Increment cumulative reuse count
        reuseCount[addr]++;
    }
    else
    {
        // First use: initialize reuse count to 0
        reuseCount[addr] = 0;
    }

    // Update last-access instruction number AFTER computing distance
    lastAccess[addr] = insCount;

    // Write trace line: PC, R/W, addr, distance, reuse
    outfile << "0x" << hex << pc      << ", "
            << (isWrite ? "Write" : "Read") << ", "
            << "0x" << addr           << ", "
            << dec << distance        << ", "
            << reuseCount[addr]       << "\n";
}

// -------------------------------------------------------
//  Counts ALL instructions (not just memory) for distance
// -------------------------------------------------------
VOID CountInstruction()
{
    insCount++;
}

// -------------------------------------------------------
//  Instrumentation callback: fires for every instruction
// -------------------------------------------------------
VOID Instruction(INS ins, VOID *v)
{
    // Count every instruction for accurate distance
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInstruction, IARG_END);

    // Record memory reads
    if (INS_IsMemoryRead(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMemAccess,
            IARG_INST_PTR,
            IARG_MEMORYREAD_EA,
            IARG_BOOL, FALSE,
            IARG_END);
    }

    // Record memory writes
    if (INS_IsMemoryWrite(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMemAccess,
            IARG_INST_PTR,
            IARG_MEMORYWRITE_EA,
            IARG_BOOL, TRUE,
            IARG_END);
    }
}

// -------------------------------------------------------
//  Fini: print summary to stdout
// -------------------------------------------------------
VOID Fini(INT32 code, VOID *v)
{
    outfile.close();

    cout << "\n===== TRACE SUMMARY =====\n";
    cout << "Total instructions     : " << dec << insCount      << "\n";
    cout << "Total memory accesses  : " << (validDistances + reuseCount.size()) << "\n";
    cout << "Unique addresses       : " << reuseCount.size()    << "\n";

    // Average distance (excluding first-use)
    if (validDistances > 0)
        cout << "Average Distance       : "
             << (double)totalDistance / validDistances << "\n";
    else
        cout << "Average Distance       : N/A (no reuse)\n";

    // Top 5 most reused addresses
    vector<pair<ADDRINT, UINT64>> vec(reuseCount.begin(), reuseCount.end());
    sort(vec.begin(), vec.end(),
         [](const pair<ADDRINT,UINT64>& a, const pair<ADDRINT,UINT64>& b){
             return a.second > b.second;
         });

    cout << "\nTop 5 addresses with maximum reuse:\n";
    for (int i = 0; i < 5 && i < (int)vec.size(); i++)
        cout << "  0x" << hex << vec[i].first
             << "  ->  reuse=" << dec << vec[i].second << "\n";

    cout << "=========================\n";
}

// -------------------------------------------------------
//  Main
// -------------------------------------------------------
int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
    {
        cerr << "PIN Init Failed\n";
        return 1;
    }

    outfile.open("mem.trace");
    if (!outfile.is_open())
    {
        cerr << "Cannot open mem.trace\n";
        return 1;
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
