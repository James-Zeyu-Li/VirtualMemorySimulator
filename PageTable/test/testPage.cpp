#include <iostream>
// #include "../pageTable.cpp" // Adjust the include path as necessary
#include <cassert>
#include <stdexcept>
#include <string>
#include <stdio.h>

#include "../PageTable.h"
#include "../helperFiles/ClockAlgorithm.h"

using namespace std;

// Basic test to add and retrieve pages
void testBasicPageInsertAndLookup(PageTable &pageTable)
{
    std::cout << "=== Basic Page Insert and Lookup Test ===" << std::endl;

    pageTable.updatePageTable(1, 100, true, false, true, true, false, 1);
    pageTable.updatePageTable(2, 101, true, false, true, true, false, 1);

    auto entry1 = pageTable.getPageTableEntry(1);
    auto entry2 = pageTable.getPageTableEntry(2);

    std::cout << "Looking up VPN 1: " << (entry1 ? "Found" : "Not Found") << std::endl;
    std::cout << "Looking up VPN 2: " << (entry2 ? "Found" : "Not Found") << std::endl;

    std::cout << std::endl;
}

// Test repeated page replacements to ensure replacement logic is correct
void testRepeatedReplacement(PageTable &pageTable)
{
    std::cout << "=== Repeated Replacement Test ===" << std::endl;

    for (int i = 0; i < 5; ++i)
    {
        pageTable.updatePageTable(i, 200 + i, true, false, true, true, false, 1);
    }

    // Trigger replacements
    for (int i = 5; i < 10; ++i)
    {
        bool replaced = pageTable.replacePageUsingClockAlgo(i);
        std::cout << "Replacing VPN " << i << ": " << (replaced ? "Success" : "Failure") << std::endl;
    }

    std::cout << std::endl;
}

// Test for multiple page faults
void testPageFaults(PageTable &pageTable)
{
    std::cout << "=== Page Fault Test ===" << std::endl;

    int result1 = pageTable.lookupPageTable(100);
    int result2 = pageTable.lookupPageTable(101);
    std::cout << "Page Fault for VPN 100: " << (result1 == -1 ? "Occurred" : "Not Occurred") << std::endl;
    std::cout << "Page Fault for VPN 101: " << (result2 == -1 ? "Occurred" : "Not Occurred") << std::endl;

    std::cout << std::endl;
}

// Edge cases: invalid pages, max reference count, and out-of-range VPN
void testEdgeCases(PageTable &pageTable)
{
    std::cout << "=== Edge Case Tests ===" << std::endl;

    // Invalid page entry
    pageTable.updatePageTable(3, 102, false, false, true, true, false, 1);
    auto invalidEntry = pageTable.getPageTableEntry(3);
    std::cout << "Invalid VPN 3 Lookup: " << (invalidEntry ? "Exists" : "Not Exists") << std::endl;

    // Max reference count
    pageTable.updatePageTable(4, 103, true, false, true, true, false, 3);
    auto maxRefEntry = pageTable.getPageTableEntry(4);
    if (maxRefEntry)
        maxRefEntry->referenceInc();
    std::cout << "Max Reference Count for VPN 4: " << static_cast<int>(maxRefEntry->reference) << std::endl;

    // Out-of-range VPN
    int outOfRangeResult = pageTable.lookupPageTable(5000); // Assuming 5000 is out of range
    std::cout << "Out-of-Range VPN 5000 Lookup: " << (outOfRangeResult == -1 ? "Not Found" : "Found") << std::endl;

    std::cout << std::endl;
}

// Test to remove invalid pages from ClockAlgorithm and verify cleanup
void testInvalidPageRemoval(PageTable &pageTable)
{
    std::cout << "=== Invalid Page Removal from ClockAlgorithm ===" << std::endl;

    pageTable.updatePageTable(5, 104, false, false, true, true, false, 1);
    pageTable.replacePageUsingClockAlgo(6); // Should remove VPN 5 if it's in active list

    int result = pageTable.lookupPageTable(5);
    std::cout << "Lookup after Replacement for Invalid VPN 5: " << (result == -1 ? "Not Found" : "Found") << std::endl;

    std::cout << std::endl;
}

// Test replacing a dirty page to ensure it writes back to disk
void testDirtyPageReplacement(PageTable &pageTable)
{
    std::cout << "=== Dirty Page Replacement Test ===" << std::endl;

    // Insert a dirty page
    pageTable.updatePageTable(11, 111, true, true, true, true, true, 1);
    pageTable.replacePageUsingClockAlgo(12);

    auto entry = pageTable.getPageTableEntry(11);
    std::cout << "Lookup VPN 11 after replacement: " << (entry ? "Found" : "Not Found") << std::endl;

    std::cout << std::endl;
}

// Test to remove an invalid page to ensure correct handling
void testInvalidVPNRemoval(PageTable &pageTable)
{
    std::cout << "=== Invalid VPN Removal Test ===" << std::endl;

    // Add an invalid page and attempt to remove it
    pageTable.updatePageTable(13, 113, false, false, true, true, true, 1);
    int removedFrame = pageTable.removeAddressForOneEntry(13);

    std::cout << "Attempted to remove invalid VPN 13: " << (removedFrame == -1 ? "Removal Failed" : "Removal Succeeded") << std::endl;

    std::cout << std::endl;
}

// Test memory full replacement logic by filling the memory and triggering replacement
void testMemoryFullReplacement(PageTable &pageTable)
{
    std::cout << "=== Memory Full Replacement Test ===" << std::endl;

    // Fill the memory with 256 frames (default)
    for (int i = 0; i < 256; ++i)
    {
        pageTable.updatePageTable(i, 200 + i, true, false, true, true, true, 1);
    }

    // Trigger replacement when memory is full
    for (int i = 256; i < 260; ++i)
    {
        bool replaced = pageTable.replacePageUsingClockAlgo(i);
        std::cout << "Replacing VPN " << i << " when memory is full: " << (replaced ? "Success" : "Failure") << std::endl;
    }

    std::cout << std::endl;
}

// Test re-inserting an invalid page to verify it can be made valid again
void testInvalidPageReInsert(PageTable &pageTable)
{
    std::cout << "=== Invalid Page Re-Insert Test ===" << std::endl;

    // Insert an invalid page
    pageTable.updatePageTable(14, 114, false, false, true, true, true, 1);

    // Re-insert as valid page
    pageTable.updatePageTable(14, 114, true, false, true, true, true, 1);

    auto entry = pageTable.getPageTableEntry(14);
    std::cout << "Re-inserted VPN 14 as valid: " << (entry && entry->valid ? "Success" : "Failure") << std::endl;

    std::cout << std::endl;
}
void testRepeatedAccess(PageTable &pageTable)
{
    std::cout << "=== Repeated Access Test ===" << std::endl;

    // 初始化 VPN 10 的页表项，并设置初始引用计数
    pageTable.updatePageTable(10, 110, true, false, true, true, true, 1);

    // 多次访问该页面以增加引用计数
    for (int i = 0; i < 5; ++i)
    {
        pageTable.lookupPageTable(10);
    }

    // 获取页表项并输出状态信息
    auto entry = pageTable.getPageTableEntry(10);
    if (entry)
    {
        std::cout << "State of VPN 10 after repeated access: " << entry->toString() << std::endl;
    }
    else
    {
        std::cout << "Failed to retrieve VPN 10" << std::endl;
    }

    std::cout << std::endl;
}
int main()
{
    PageTable pageTable; // Default with 256 frames

    // Run each test with spacing for clarity
    testBasicPageInsertAndLookup(pageTable);
    testRepeatedReplacement(pageTable);
    testPageFaults(pageTable);
    testEdgeCases(pageTable);
    testInvalidPageRemoval(pageTable);
    testRepeatedAccess(pageTable);
    testDirtyPageReplacement(pageTable);
    testInvalidVPNRemoval(pageTable);
    testMemoryFullReplacement(pageTable);
    testInvalidPageReInsert(pageTable);
    return 0;
}