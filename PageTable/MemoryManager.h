#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstdint>
#include <iostream>
#include "pageTable.cpp"          // Include the page table header
#include "tlb.cpp"                // Include the TLB header
#include "physicalFrameManger.h"  // Include the physical frame manager header

class MemoryManager {
private:
    PageTable pageTable;
    TLB tlb;
    PhysicalFrameManager frameManager;

public:
    MemoryManager(int pageTableFrames, int tlbSize, int physicalFrames);

    // Function to translate virtual address to physical address
    uint32_t translateVirtualAddress(uint32_t virtualAddress);
};

#endif // MEMORY_MANAGER_H