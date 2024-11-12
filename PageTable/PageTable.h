#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <unordered_map>
#include <cstdint>
#include <list>
#include <unordered_set>
#include "PageTableEntry.h"
#include "helperFiles/ClockAlgorithm.h"
#include <cmath>
using namespace std;

class PageTable
{
private:
    // Two-level page table structure
    unordered_map<uint32_t, unordered_map<uint32_t, PageTableEntry>> pageTable;

    uint32_t addressBits; // 32bit
    uint64_t addressSpaceSize; // length of the address space
    uint32_t pageSize;         // 4096

    // Counters to track allocated entries
    uint32_t level1EntriesAllocated = 0; // Count of L1 entries allocated
    uint32_t level2EntriesAllocated = 0; // Count of L2 entries allocated

    const int pageOffsetBits = static_cast<int>(log2(pageSize));
    const int vpnBits = addressBits - pageOffsetBits;
    const int l1Bits = vpnBits / 2;
    const int l2Bits = vpnBits - l1Bits;

    // Clock algorithm manager
    ClockAlgorithm clockAlgo;

    // Helper functions to extract level-1 and level-2 indices from a VPN
    uint32_t getL1Index(uint32_t VPN);
    uint32_t getL2Index(uint32_t VPN);

    // Helper function to check and create a second-level map if necessary
    unordered_map<uint32_t, PageTableEntry> &checkL2(uint32_t l1Index);

public:
    // Constructors
    PageTable(uint32_t addressBits, uint32_t pageSize);

    // Lookup the page table for a given VPN, returning the frame number or -1 if not found
    int32_t lookupPageTable(uint32_t VPN);

    // Update the page table with a new or existing entry
    void updatePageTable(uint32_t VPN, uint32_t frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference);

    // Replace a page in memory using ClockAlgorithm
    bool replacePageUsingClockAlgo(uint32_t VPN);

    // Write a page frame back to disk
    void writeBackToDisk(uint32_t frameNumber);

    // Remove the address for one entry
    int removeAddressForOneEntry(uint32_t VPN);

    // Get the PageTableEntry for a given VPN
    PageTableEntry *getPageTableEntry(uint32_t VPN);

    void resetPageTable();

    bool isValidRange(uint32_t VPN);

    // Functions to calculate memory usage
    uint32_t getAllocatedEntries() const;
    uint32_t getTotalMemoryUsage() const;
    uint32_t getAvailableSpaceSingleLevel(uint64_t addressSpaceSize, uint32_t pageSize) const;
    void displayStatistics() const;
};

#endif // PAGETABLE_H
