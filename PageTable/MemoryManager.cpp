#include "MemoryManager.h"

MemoryManager::MemoryManager(int pageTableFrames, int tlbSize, int physicalFrames)
    : pageTable(pageTableFrames), tlb(tlbSize), frameManager(physicalFrames) {}

// Function to translate virtual address to physical address
uint32_t MemoryManager::translateVirtualAddress(uint32_t virtualAddress) {
    // Constants for address components based on page size
    const int pageOffsetBits = 12; // 4KB page size
    const uint32_t pageSize = 1 << pageOffsetBits;
    const uint32_t pageOffsetMask = pageSize - 1;

    // Calculate VPN (Virtual Page Number) and offset within the page
    uint32_t vpn = virtualAddress >> pageOffsetBits;
    uint32_t offset = virtualAddress & pageOffsetMask;

    // 1. Check the TLB first for the VPN
    int pfn = tlb.lookupTLB(vpn);
    if (pfn != -1) {
        // TLB hit - construct the physical address
        std::cout << "TLB hit for VPN " << vpn << ", PFN: " << pfn << std::endl;
        return (pfn << pageOffsetBits) | offset;
    }

    // 2. TLB miss - check the page table
    pfn = pageTable.lookupPageTable(vpn);
    if (pfn != -1) {
        // Page table hit - update TLB and return physical address
        std::cout << "Page table hit for VPN " << vpn << ", PFN: " << pfn << std::endl;
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB with read/write/execute permissions as needed
        return (pfn << pageOffsetBits) | offset;
    }

    // 3. Page fault - Handle page fault
    std::cout << "Page fault for VPN " << vpn << std::endl;
    if (!pageTable.handlePageFault(vpn)) {
        std::cerr << "Error: Unable to handle page fault for VPN " << vpn << std::endl;
        return UINT32_MAX; // Return an error if page fault handling fails
    }

    // Retry after handling page fault
    pfn = pageTable.lookupPageTable(vpn);
    if (pfn != -1) {
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB after page fault resolution
        return (pfn << pageOffsetBits) | offset;
    }

    // If we still can't resolve the address, return an error
    std::cerr << "Error: Failed to translate virtual address " << virtualAddress << std::endl;
    return UINT32_MAX;
}