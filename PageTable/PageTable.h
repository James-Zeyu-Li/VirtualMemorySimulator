#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <unordered_map>
#include <cstdint>
#include <list>
#include <unordered_set>
#include "PageTableEntry.h"
#include "PhysicalFrameManager.h"
#include "helperFiles/ClockAlgorithm.h"

class PageTable
{
private:
    // Two-level page table structure
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, PageTableEntry> > pageTable;

    // Frame manager to allocate and manage physical frames
    PhysicalFrameManager pfManager;

    // Clock algorithm manager
    ClockAlgorithm clockAlgo;

    // Helper functions to extract level-1 and level-2 indices from a VPN
    uint32_t getL1Index(uint32_t VPN);
    uint32_t getL2Index(uint32_t VPN);

    // Helper function to check and create a second-level map if necessary
    std::unordered_map<uint32_t, PageTableEntry> &checkL2(uint32_t l1Index);


public:
    // Constants
    static const uint64_t addressSpaceSize = 4ULL * 1024 * 1024 * 1024; // 32-bit address space
    static const int pageSize = 4096;                                   // 4KB
    static const int addressBits = 32;
    static const int pageOffsetBits = 12; // log2(pageSize)
    static const int vpnBits = addressBits - pageOffsetBits;
    static const int l1Bits = vpnBits / 2;
    static const int l2Bits = vpnBits - l1Bits;

    // Constructors
    PageTable();                     // Default constructor with 256 frames
    PageTable(uint32_t totalFrames); // Constructor with custom frame count

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

    // For testing purposes only
    // void printPageTable() const;

    bool isValidRange(uint32_t VPN);
};

#endif // PAGETABLE_H
