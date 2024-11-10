#include <iostream>
#include <cassert>
#include "../MemoryManager.h"

void testMemoryManager() {
    // Initialize MemoryManager with 256 page table frames, 16 TLB entries, 256 physical frames
    MemoryManager memoryManager(256, 16, 256);

    // 1. Test case for TLB miss and page table hit
    uint32_t testVirtualAddress1 = 0x12345000;  // Sample virtual address
    memoryManager.translateVirtualAddress(testVirtualAddress1); // Expecting a page fault first

    uint32_t physicalAddress1 = memoryManager.translateVirtualAddress(testVirtualAddress1); // Expecting a TLB hit after page fault
    assert(physicalAddress1 != UINT32_MAX);
    std::cout << "Test 1 (TLB miss, page table hit): Passed" << std::endl;

    // 2. Test case for TLB hit
    uint32_t physicalAddress2 = memoryManager.translateVirtualAddress(testVirtualAddress1); // Expecting TLB hit
    assert(physicalAddress2 == physicalAddress1);
    std::cout << "Test 2 (TLB hit): Passed" << std::endl;

    // 3. Test case for different virtual address causing another page fault
    uint32_t testVirtualAddress2 = 0x12346000;
    uint32_t physicalAddress3 = memoryManager.translateVirtualAddress(testVirtualAddress2); // Expecting a new page fault
    assert(physicalAddress3 != UINT32_MAX);
    assert(physicalAddress3 != physicalAddress1);
    std::cout << "Test 3 (New page fault for different address): Passed" << std::endl;

    // 4. Test case for invalid virtual address (simulating beyond address space)
    uint32_t invalidVirtualAddress = 0xFFFFFFFF; // Out of 32-bit address range for this setup
    uint32_t physicalAddressInvalid = memoryManager.translateVirtualAddress(invalidVirtualAddress);
    assert(physicalAddressInvalid == UINT32_MAX);
    std::cout << "Test 4 (Invalid address): Passed" << std::endl;
}

int main() {
    std::cout << "Starting MemoryManager tests..." << std::endl;
    testMemoryManager();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}