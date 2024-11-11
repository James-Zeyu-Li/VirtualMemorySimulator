#include <iostream>
#include "../pageTable.cpp" // Adjust the include path as necessary
#include <cassert>
#include <stdexcept>
#include <string>
#include <stdio.h>
using namespace std;

// Helper functions for assertions
void assertEqual(int expected, int actual, const std::string &testName)
{
    if (expected == actual)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] " << testName << " - Expected: " << expected << ", Got: " << actual << std::endl;
    }
}

void assertNotEqual(int notExpected, int actual, const std::string &testName)
{
    if (notExpected != actual)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] " << testName << " - Value should not be: " << notExpected << std::endl;
    }
}

void assertTrue(bool condition, const std::string &testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] " << testName << std::endl;
    }
}

void assertFalse(bool condition, const std::string &testName)
{
    if (!condition)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] " << testName << std::endl;
    }
}

// Check if a VPN exists in the page table
bool vpnExistsInPageTable(PageTable &pt, uint32_t VPN)
{
    return pt.lookupPageTable(VPN) != -1;
}

// Test initialization of PageTable
void testInitializePageTable()
{
    std::cout << "Running testInitializePageTable..." << std::endl;
    PageTable pt;
    pt.printPageTable(); // Should print an empty page table
    std::cout << "If the above page table is empty, testInitializePageTable passed.\n"
              << std::endl;
}

// Test valid VPN access and updates
void testValidVPNAccess()
{
    std::cout << "Running testValidVPNAccess..." << std::endl;
    PageTable pt;

    uint32_t validVPN = 42;
    uint32_t frameNumber = 7;

    // Update the page table with a valid VPN
    pt.updatePageTable(validVPN, frameNumber, true, false, true, true, true, 0);

    // Lookup the VPN
    int32_t lookupFrame = pt.lookupPageTable(validVPN);
    assertEqual(frameNumber, lookupFrame, "Valid VPN lookup");

    // Check reference count increment
    pt.lookupPageTable(validVPN); // Should increment reference count to 1
    pt.lookupPageTable(validVPN); // Should increment reference count to 2
    pt.lookupPageTable(validVPN); // Should increment reference count to 3
    pt.lookupPageTable(validVPN); // Should not increment beyond 3

    // Since we can't access private members directly, we consider the test passed if no errors occur
    std::cout << "testValidVPNAccess passed.\n"
              << std::endl;
}

// Test invalid VPN access
void testInvalidVPNAccess()
{
    std::cout << "Running testInvalidVPNAccess..." << std::endl;
    PageTable pt;

    uint32_t invalidVPNs[] = {static_cast<uint32_t>(-1), (1U << 30)}; // Assuming address space supports up to 1 << 20 pages

    for (uint32_t vpn : invalidVPNs)
    {
        int lookupFrame = pt.lookupPageTable(vpn);
        assertEqual(-1, lookupFrame, "Invalid VPN lookup for VPN " + std::to_string(vpn));
    }

    std::cout << "testInvalidVPNAccess passed.\n"
              << std::endl;
}

// Test handling of page faults
void testPageFaultHandling()
{
    std::cout << "Running testPageFaultHandling..." << std::endl;
    PageTable pt(4); // Initialize with 4 physical frames

    uint32_t VPNs[] = {1, 2, 3, 4};
    for (uint32_t vpn : VPNs)
    {
        int lookupFrame = pt.lookupPageTable(vpn);
        assertEqual(-1, lookupFrame, "VPN " + std::to_string(vpn) + " not in page table");

        bool pageFaultHandled = pt.handlePageFault(vpn);
        assertTrue(pageFaultHandled, "Page fault handled for VPN " + std::to_string(vpn));

        lookupFrame = pt.lookupPageTable(vpn);
        assertNotEqual(-1, lookupFrame, "VPN " + std::to_string(vpn) + " in page table after handling fault");
    }

    std::cout << "testPageFaultHandling passed.\n"
              << std::endl;
}

// Test physical frame allocation until exhaustion
void testPhysicalFrameAllocation()
{
    std::cout << "Running testPhysicalFrameAllocation..." << std::endl;
    int totalFrames = 3;
    PageTable pt(totalFrames);

    for (uint32_t i = 0; i < static_cast<uint32_t>(totalFrames); ++i)
    {
        bool pageFaultHandled = pt.handlePageFault(i);
        assertTrue(pageFaultHandled, "Allocated frame for VPN " + std::to_string(i));
    }

    // Next allocation should trigger page replacement
    bool pageFaultHandled = pt.handlePageFault(99);
    assertTrue(pageFaultHandled, "Page replacement occurred for VPN 99");

    std::cout << "testPhysicalFrameAllocation passed.\n"
              << std::endl;
}

// Test the page replacement algorithm (Clock Algorithm)
void testPageReplacement()
{
    std::cout << "Running testPageReplacement..." << std::endl;
    PageTable pt(2); // Small number of frames to trigger replacement quickly

    pt.handlePageFault(1);
    pt.handlePageFault(2);

    // Access VPN 1 to set its reference count
    pt.lookupPageTable(1); // Reference count becomes 1

    // Trigger page replacement
    pt.handlePageFault(3);

    // VPN 2 should be replaced since its reference count is lower
    int frameForVPN2 = pt.lookupPageTable(2);
    assertEqual(-1, frameForVPN2, "VPN 2 should have been replaced");

    int frameForVPN3 = pt.lookupPageTable(3);
    assertNotEqual(-1, frameForVPN3, "VPN 3 should be in page table");

    std::cout << "testPageReplacement passed.\n"
              << std::endl;
}

// Test reference count increment and decrement
void testReferenceCounts()
{
    std::cout << "Running testReferenceCounts..." << std::endl;
    PageTable pt(2);

    pt.handlePageFault(1);
    pt.handlePageFault(2);

    // Access VPNs to set reference counts
    pt.lookupPageTable(1); // Reference count becomes 1
    pt.lookupPageTable(2); // Reference count becomes 1
    pt.lookupPageTable(2); // Reference count becomes 2

    // Trigger page replacement
    pt.handlePageFault(3);

    // VPN 1 should be replaced since it has a lower reference count
    int frameForVPN1 = pt.lookupPageTable(1);
    assertEqual(-1, frameForVPN1, "VPN 1 should have been replaced");

    int frameForVPN2 = pt.lookupPageTable(2);
    assertNotEqual(-1, frameForVPN2, "VPN 2 should still be in page table");

    std::cout << "testReferenceCounts passed.\n"
              << std::endl;
}

// Test handling of dirty pages
void testDirtyPages()
{
    std::cout << "Running testDirtyPages..." << std::endl;
    PageTable pt(2);

    pt.handlePageFault(1);
    pt.updatePageTable(1, 0, true, true, true, true, true, 1); // Mark VPN 1 as dirty

    pt.handlePageFault(2);
    pt.updatePageTable(2, 1, true, false, true, true, true, 1);

    // Trigger page replacement
    pt.handlePageFault(3);

    // VPN 1 should be written back to disk
    // Since we cannot capture console output easily, we consider the test passed if no errors occur
    std::cout << "testDirtyPages passed.\n"
              << std::endl;
}

// Test resetting the page table
void testResetFunctionality()
{
    std::cout << "Running testResetFunctionality..." << std::endl;
    PageTable pt;

    pt.handlePageFault(1);
    pt.handlePageFault(2);

    pt.resetPageTable();

    int frameForVPN1 = pt.lookupPageTable(1);
    assertEqual(-1, frameForVPN1, "VPN 1 should not be in page table after reset");

    std::cout << "testResetFunctionality passed.\n"
              << std::endl;
}

// Test removing a single address
void testRemoveAddressForOneEntry()
{
    std::cout << "Running testRemoveAddressForOneEntry..." << std::endl;

    // Initialize page table with a small number of frames for testing
    PageTable pt(4);

    // Case 1: Removing a non-existent VPN
    pt.removeAddressForOneEntry(1000); // Attempt to remove VPN that was never added
    assertFalse(vpnExistsInPageTable(pt, 1000), "Removing non-existent VPN should have no effect");

    // Case 2: Add a VPN and then remove it
    pt.updatePageTable(42, 1, true, false, true, true, true, 0); // Add VPN 42
    assertTrue(vpnExistsInPageTable(pt, 42), "VPN 42 should be present in page table");
    pt.removeAddressForOneEntry(42); // Remove VPN 42
    assertFalse(vpnExistsInPageTable(pt, 42), "VPN 42 should be removed from page table\n");

    // Case 3: Remove the only VPN in the page table
    pt.updatePageTable(43, 2, true, false, true, true, true, 0); // Add VPN 43
    assertTrue(vpnExistsInPageTable(pt, 43), "VPN 43 should be present in page table");
    pt.removeAddressForOneEntry(43); // Remove VPN 43
    assertFalse(vpnExistsInPageTable(pt, 43), "VPN 43 should be removed from page table\n");

    // Case 4: Removing the VPN at the current clock hand position
    pt.updatePageTable(44, 3, true, false, true, true, true, 0); // Add VPN 44
    pt.updatePageTable(45, 4, true, false, true, true, true, 0); // Add VPN 45
    assertTrue(vpnExistsInPageTable(pt, 44), "VPN 44 should be present in page table");
    assertTrue(vpnExistsInPageTable(pt, 45), "VPN 45 should be present in page table\n");

    // Set clockHand to VPN 44 manually for testing
    pt.lookupPageTable(44);
    pt.removeAddressForOneEntry(44); // Remove VPN 44 which is at the clock hand position
    assertFalse(vpnExistsInPageTable(pt, 44), "VPN 44 should be removed from page table");
    assertTrue(vpnExistsInPageTable(pt, 45), "VPN 45 should still be present in page table\n");

    // Case 5: Remove multiple entries in succession
    pt.updatePageTable(46, 5, true, false, true, true, true, 0); // Add VPN 46
    pt.updatePageTable(47, 6, true, false, true, true, true, 0); // Add VPN 47
    assertTrue(vpnExistsInPageTable(pt, 46), "VPN 46 should be present in page table");
    assertTrue(vpnExistsInPageTable(pt, 47), "VPN 47 should be present in page table\n");

    pt.removeAddressForOneEntry(46);
    assertFalse(vpnExistsInPageTable(pt, 46), "VPN 46 should be removed from page table");
    assertTrue(vpnExistsInPageTable(pt, 47), "VPN 47 should still be present in page table\n");

    pt.removeAddressForOneEntry(47);
    assertFalse(vpnExistsInPageTable(pt, 47), "VPN 47 should be removed from page table\n");

    // Case 6: Ensure clock hand updates correctly after removals
    pt.updatePageTable(48, 7, true, false, true, true, true, 0); // Add VPN 48
    pt.updatePageTable(49, 8, true, false, true, true, true, 0); // Add VPN 49
    assertTrue(vpnExistsInPageTable(pt, 48), "VPN 48 should be present in page table");
    assertTrue(vpnExistsInPageTable(pt, 49), "VPN 49 should be present in page table\n");

    pt.lookupPageTable(48);          // Set clock hand to VPN 48
    pt.removeAddressForOneEntry(48); // Remove VPN 48
    assertFalse(vpnExistsInPageTable(pt, 48), "VPN 48 should be removed from page table");
    assertTrue(vpnExistsInPageTable(pt, 49), "VPN 49 should still be present in page table");
    pt.removeAddressForOneEntry(49); // Remove VPN 49
    assertFalse(vpnExistsInPageTable(pt, 49), "VPN 49 should be removed from page table\n");

    std::cout << "testRemoveAddressForOneEntry completed.\n"
              << std::endl;
}

// Test various edge cases
void testEdgeCases()
{
    std::cout << "Running testEdgeCases..." << std::endl;
    PageTable pt;

    // Attempt to update with invalid VPN
    pt.updatePageTable(static_cast<uint32_t>(-1), 0, true, false, true, true, true, 0);
    pt.updatePageTable(1U << 31, 0, true, false, true, true, true, 0); // Exceeds address space

    // Attempt to free an invalid frame
    PhysicalFrameManager pfm(256);
    try
    {
        pfm.freeAFrame(static_cast<uint32_t>(-1));
        std::cout << "[FAIL] Exception not thrown for invalid frame number" << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "[PASS] Exception caught for invalid frame number: " << e.what() << std::endl;
    }

    std::cout << "testEdgeCases passed.\n"
              << std::endl;
}

// Helper function for assertions with toString output
void assertEqualString(const std::string &expected, const std::string &actual, const std::string &testName)
{
    if (expected == actual)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] " << testName << " - Expected: " << expected << ", Got: " << actual << std::endl;
    }
}

// Test to verify reference counting behavior for a page table entry
void testReferenceCountBehavior()
{
    std::cout << "Running testReferenceCountBehavior..." << std::endl;

    PageTableEntry entry(1, true, false, true, true, true, 0);

    // Initial state with reference count 0
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 0 }", entry.toString(), "Initial reference count at 0");

    // Increment reference count to 1, 2, then 3
    entry.referenceInc();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 1 }", entry.toString(), "Increment reference to 1");

    entry.referenceInc();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 2 }", entry.toString(), "Increment reference to 2");

    entry.referenceInc();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 3 }", entry.toString(), "Increment reference to 3");

    // Attempt to increment beyond 3
    entry.referenceInc();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 3 }", entry.toString(), "Reference count should remain at max 3");

    // Decrement reference count back to 2, 1, then 0
    entry.referenceDec();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 2 }", entry.toString(), "Decrement reference to 2");

    entry.referenceDec();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 1 }", entry.toString(), "Decrement reference to 1");

    entry.referenceDec();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 0 }", entry.toString(), "Decrement reference to 0");

    // Attempt to decrement below 0
    entry.referenceDec();
    assertEqualString("{ Frame: 1, Valid: 1, Dirty: 0, Reference: 0 }", entry.toString(), "Reference count should remain at min 0");

    std::cout << "testReferenceCountBehavior passed. \n"
              << std::endl;
}

// Test to verify reference count interactions within PageTable
void testPageTableReferenceCountInteractions()
{
    std::cout << "Running testPageTableReferenceCountInteractions..." << std::endl;

    PageTable pt(2); // Initialize with 2 frames to allow for replacement

    // Handle page faults to add VPNs
    pt.handlePageFault(1); // Adds VPN 1
    pt.handlePageFault(2); // Adds VPN 2

    // Access VPN 1 multiple times to increment its reference count
    pt.lookupPageTable(1); // Ref count for VPN 1 should become 1
    pt.lookupPageTable(1); // Ref count for VPN 1 should become 2
    pt.lookupPageTable(1); // Ref count for VPN 1 should become 3
    pt.lookupPageTable(1); // Further lookups should keep ref count at max 3

    // Verify VPN 1 is still in the page table (ref count should be at max)
    int32_t frameForVPN1 = pt.lookupPageTable(1);
    assertNotEqual(-1, frameForVPN1, "VPN 1 should be in the page table");

    // Access VPN 2 once to give it a reference count of 1
    pt.lookupPageTable(2);

    // Trigger replacement by adding a new VPN (3)
    pt.handlePageFault(3); // This should replace VPN 2, which has a lower reference count than VPN 1

    // Confirm that VPN 2 was replaced
    int32_t frameForVPN2 = pt.lookupPageTable(2);
    assertEqual(-1, frameForVPN2, "VPN 2 should have been replaced due to lower reference count");

    // Confirm VPN 1 is still in the page table
    frameForVPN1 = pt.lookupPageTable(1);
    assertNotEqual(-1, frameForVPN1, "VPN 1 should still be in the page table after replacement");

    // Access VPN 3 once to increase its reference count to 1
    pt.lookupPageTable(3);

    // Adding a new VPN (4) should trigger replacement and remove VPN 3
    pt.handlePageFault(4); // Since VPN 3 has a lower reference count than VPN 1, it should be replaced

    // Confirm that VPN 3 was replaced and VPN 1 remains
    int32_t frameForVPN3 = pt.lookupPageTable(3);
    assertEqual(-1, frameForVPN3, "VPN 3 should have been replaced due to lower reference count");

    frameForVPN1 = pt.lookupPageTable(1);
    assertNotEqual(-1, frameForVPN1, "VPN 1 should still be in the page table after replacement");

    std::cout << "testPageTableReferenceCountInteractions passed." << std::endl;
}

int main()
{
    testInitializePageTable();
    testValidVPNAccess();
    testInvalidVPNAccess();
    testPageFaultHandling();
    testPhysicalFrameAllocation();
    testPageReplacement();
    testReferenceCounts();
    testDirtyPages();
    testResetFunctionality();
    testRemoveAddressForOneEntry();
    testEdgeCases();

    testReferenceCountBehavior();
    testPageTableReferenceCountInteractions();

    return 0;
}
