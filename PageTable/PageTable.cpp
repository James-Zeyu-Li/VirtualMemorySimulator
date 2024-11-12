#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_set>
#include "helperFiles/ClockAlgorithm.h"
#include "PageTableEntry.h"
#include "PageTable.h"

using namespace std;

// Helper functions to extract level-1 and level-2 indices from a VPN
uint32_t PageTable::getL1Index(uint32_t VPN)
{
    return VPN >> l2Bits; // Shift right by l2Bits
}

uint32_t PageTable::getL2Index(uint32_t VPN)
{
    return VPN & ((1 << l2Bits) - 1); // Get the last l2Bits
}

// Helper function to check and create a second-level map if necessary
unordered_map<uint32_t, PageTableEntry> &PageTable::checkL2(uint32_t l1Index)
{
    if (pageTable.find(l1Index) == pageTable.end()) // If the first level map does not exist
    {
        pageTable[l1Index] = unordered_map<uint32_t, PageTableEntry>(); // Create a new second-level map
        level1EntriesAllocated++; // Increment L1 counter when a new L1 entry is allocated
    }
    return pageTable[l1Index]; // Return the second-level map
}

// Check if a VPN is within a valid range
bool PageTable::isValidRange(uint32_t VPN)
{
    return VPN < addressSpaceSize / pageSize;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// constructor
PageTable::PageTable(uint32_t addressBits, uint32_t pageSize)
    :addressBits(addressBits),
    addressSpaceSize(1ULL << addressBits),
      pageSize(pageSize),
      clockAlgo()
{
    if (addressSpaceSize % pageSize != 0)
    {
        cerr << "Error: Address space size must be a multiple of page size" << endl;
        return;
    }
}
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Lookup the page table for a given VPN and return the frame number or -1 if not found
int32_t PageTable::lookupPageTable(uint32_t VPN)
{
    if (!isValidRange(VPN))
    {
        cerr << "Invalid VPN: " << VPN << " Out of range" << endl;
        return -1;
    }

    uint32_t l1Index = getL1Index(VPN); // Get the first-level index
    uint32_t l2Index = getL2Index(VPN); // Get the second-level index

    auto it1 = pageTable.find(l1Index); // Find the first-level map
    if (it1 != pageTable.end())         // If the first-level map exists
    {
        auto it2 = it1->second.find(l2Index);              // Find the second-level map
        if (it2 != it1->second.end() && it2->second.valid) // If the second-level map exists and the page is valid
        {
            // Increment reference count if less than 3
            if (it2->second.reference < 3)
            {
                it2->second.referenceInc(); // Increment the reference count
            }

            // Update active pages via ClockAlgorithm
            clockAlgo.addPage(VPN);

            return it2->second.frameNumber; // Return the frame number
        }
    }

    return -1; // Page fault
}

// Update the page table with the given VPN and PFN
void PageTable::updatePageTable(uint32_t VPN, uint32_t frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference)
{
    if (!isValidRange(VPN))
    {
        cerr << "Invalid VPN: " << VPN << " Out of range" << endl;
        return;
    }

    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    // Check if we're adding a new first-level entry
    bool isNewL1Entry = (pageTable.find(l1Index) == pageTable.end());
    auto &l2Table = checkL2(l1Index); // This will create a new second-level map if necessary

    // If a new L1 entry was created, increment the L1 counter
    if (isNewL1Entry) {
        level1EntriesAllocated++;
    }

    // Check if we're adding a new second-level entry
    bool isNewL2Entry = (l2Table.find(l2Index) == l2Table.end());
    if (isNewL2Entry) {
        level2EntriesAllocated++;
    }

    // Get the second-level map entry
    auto &entry = l2Table[l2Index];

    // Update the page table entry
    entry.frameNumber = frameNumber;
    entry.valid = valid;
    entry.dirty = dirty;
    entry.read = read;
    entry.write = write;
    entry.execute = execute;
    entry.reference = reference;

    // If the page is valid, update the active pages via ClockAlgorithm
    if (valid) {
        clockAlgo.addPage(VPN);
    } else {
        // If the page is invalid, remove it from the active pages
        clockAlgo.removePage(VPN);
    }
}

// handle page fault with ClockAlgorithm, page replacement
bool PageTable::replacePageUsingClockAlgo(uint32_t VPN)
{
    if (!isValidRange(VPN))
    {
        cerr << "Invalid VPN: " << VPN << " Out of range" << endl;
        return false;
    }

    uint32_t targetVPN; // claim a target VPN

    // select a page to replace using the clock algorithm
    while (clockAlgo.selectPageToReplace(targetVPN, *this)) // call the selectPageToReplace function from ClockAlgorithm, if a target is found
    {
        PageTableEntry *targetEntry = getPageTableEntry(targetVPN);

        // if page is valid, replace it
        if (targetEntry && targetEntry->valid)
        {
            uint32_t oldFrame = targetEntry->frameNumber;

            // if the target page is dirty, write it back to disk
            if (targetEntry->dirty)
            {
                writeBackToDisk(oldFrame);
            }

            // remove the old page from the page table before replacing it
            // FIXME: removed and reallocated memory within pagetable, no interaction with main -------
            int removedFrame = removeAddressForOneEntry(targetVPN);
            if (removedFrame == -1)
            {
                cerr << "Error: Failed to remove victim VPN: " << targetVPN << endl;
                return false;
            }

            // ---- need method from main to allocate a new frame for the new page
            int newFrame = oldFrame;
            updatePageTable(VPN, oldFrame, true, false, true, true, true, 0);
            // ---------------

            PageTableEntry *newEntry = getPageTableEntry(VPN);
            if (!newEntry || !newEntry->valid)
            {
                cerr << "Error: Failed to update page table with VPN: " << VPN << " and Frame: " << oldFrame << endl;
                return false;
            }

            //--------------------------------------------
            return true;
        }
        else
        {
            // if the target page is invalid, remove it from the page table and try again
            cerr << "Warning: Invalid or non-existent page selected by ClockAlgorithm: " << targetVPN << endl;
            clockAlgo.removePage(targetVPN); // remove the target page from the active pages
        }
    }

    cerr << "Failed to replace page for VPN: " << VPN << endl;
    return false; // fail to replace page
}

// Write the page back to disk
void PageTable::writeBackToDisk(uint32_t frameNumber)
{
    cout << "Writing frame " << frameNumber << " back to disk." << endl;
}

// Remove the address for one entry
int PageTable::removeAddressForOneEntry(uint32_t VPN)
{
    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    // check if VPN is in the page table
    // fist check if the first level index exists
    auto it1 = pageTable.find(l1Index);
    if (it1 == pageTable.end())
    {
        cerr << "Error: L1 index " << l1Index << " not found in the page table for VPN: " << VPN << endl;
        return -1;
    }

    // then check if the second level index exists
    auto it2 = it1->second.find(l2Index);
    if (it2 == it1->second.end())
    {
        cerr << "Error: L2 index " << l2Index << " not found in the page table for VPN: " << VPN << endl;
        return -1;
    }

    // get the frame number for the VPN
    int pfn = it2->second.frameNumber;


    // delete the entry from the page table
    it1->second.erase(it2);
    if (it1->second.empty())
    {
        pageTable.erase(it1);
    }

    // remove the VPN from the clock algorithm
    clockAlgo.removePage(VPN);

    return pfn; // return the frame number
}

// Get the PageTableEntry for a given VPN
PageTableEntry *PageTable::getPageTableEntry(uint32_t VPN)
{
    if (!isValidRange(VPN))
    {
        return nullptr;
    }

    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    // check if the VPN is in the page table
    auto it1 = pageTable.find(l1Index);
    if (it1 == pageTable.end())
    {
        return nullptr;
    }

    // check if the second level index exists
    auto it2 = it1->second.find(l2Index);
    if (it2 == it1->second.end())
    {
        return nullptr;
    }

    // return the page table entry if it is valid
    if (it2->second.valid)
    {
        return &(it2->second);
    }
    return nullptr;
}

// reset the page table
void PageTable::resetPageTable()
{
    pageTable.clear();
    clockAlgo.reset();
}

// Returns the number of allocated entries in total
uint32_t PageTable::getAllocatedEntries() const {
    return level1EntriesAllocated + level2EntriesAllocated;
}

// Returns the memory usage for the two-level page table
uint32_t PageTable::getTotalMemoryUsage() const {
    uint32_t sizeL1Entry = sizeof(unordered_map<uint32_t, PageTableEntry>);
    uint32_t sizeL2Entry = sizeof(PageTableEntry);

    return (level1EntriesAllocated * sizeL1Entry) + (level2EntriesAllocated * sizeL2Entry);
}

// Returns the memory usage for a hypothetical single-level page table
uint32_t PageTable::getAvailableSpaceSingleLevel(uint64_t addressSpaceSize, uint32_t pageSize) const {
    uint32_t numPages = addressSpaceSize / pageSize;
    uint32_t sizeSingleLevelEntry = sizeof(PageTableEntry);
    return (numPages * sizeSingleLevelEntry);
}

void PageTable::displayStatistics() const {
    cout << "Two-Level Page Table Statistics:" << endl;
    cout << "  Total L1 Entries Allocated: " << level1EntriesAllocated << endl;
    cout << "  Total L2 Entries Allocated: " << level2EntriesAllocated << endl;
    cout << "  Total Allocated Entries: " << getAllocatedEntries() << endl;
    cout << "  Total Memory Usage (Two-Level): " << getTotalMemoryUsage() << " bytes" << endl;
    cout << "  For comparison, a single-level page table requires " << addressSpaceSize / pageSize
         << " entries and " << getAvailableSpaceSingleLevel(addressSpaceSize, pageSize) << " bytes" << endl;
    cout << endl;
}
