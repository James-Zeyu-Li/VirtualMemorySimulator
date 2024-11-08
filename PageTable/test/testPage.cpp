#include <cassert>
#include <iostream>
#include "../pageTable.cpp"

void testLookupPageTable()
{
    PageTable pt;

    // add a valid page table entry
    int vpn = 1, pfn = 2;
    pt.setPageTableEntry(vpn, pfn, 100, true); // add pfn 100 to vpn 1

    // test valid page table entry 
    int result = pt.lookupPageTable(vpn, pfn);
    assert(result == 100); // expected result is 100
    std::cout << "Test 1 passed: Found valid entry with frame number 100." << std::endl;

    // test invalid page table entry
    int invalidVpn = 3, invalidPfn = 4;
    result = pt.lookupPageTable(invalidVpn, invalidPfn);
    assert(result == -1); // expected result is -1
    std::cout << "Test 2 passed: No entry found, as expected." << std::endl;
}

int main()
{
    testLookupPageTable();
    return 0;
}
