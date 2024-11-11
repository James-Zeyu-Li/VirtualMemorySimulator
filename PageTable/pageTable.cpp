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
    }
    return pageTable[l1Index]; // Return the second-level map
}

// Check if a VPN is within a valid range
bool PageTable::isValidRange(uint32_t VPN)
{
    return VPN < addressSpaceSize / pageSize;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Default constructor with 256 frames
PageTable::PageTable(uint64_t addressSpaceSize, uint32_t pageSize,
                     uint32_t &maxFrames)
    : addressSpaceSize(addressSpaceSize),
      pageSize(pageSize),
      allocatedFrames(allocatedFrames),
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

    // Get the second-level map
    auto &entry = checkL2(l1Index)[l2Index]; // Get the page table entry, create if it does not exist, call the above function

    // Update the page table entry
    entry.frameNumber = frameNumber;
    entry.valid = valid;
    entry.dirty = dirty;
    entry.read = read;
    entry.write = write;
    entry.execute = execute;
    entry.reference = reference;

    // If the page is valid, update the active pages via ClockAlgorithm
    if (valid)
    {
        clockAlgo.addPage(VPN);
    }
    else
    {
        // If the page is invalid, remove it from the active pages
        clockAlgo.removePage(VPN);
    }
}

// handle page fault with ClockAlgorithm, page replacement
bool PageTable::replacePageUsingClockAlgo(uint32_t VPN)
{
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
            int removedFrame = removeAddressForOneEntry(targetVPN);
            if (removedFrame == -1)
            {
                cerr << "Error: Failed to remove victim VPN: " << targetVPN << endl;
                return false;
            }

// ---- need method from main to allocate a new frame for the new page
            // allocate the old frame for the new page
            updatePageTable(VPN, oldFrame, true, false, true, true, true, 0);
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
        std::cerr << "Error: L1 index " << l1Index << " not found in the page table for VPN: " << VPN << std::endl;
        return -1;
    }

    // then check if the second level index exists
    auto it2 = it1->second.find(l2Index);
    if (it2 == it1->second.end())
    {
        std::cerr << "Error: L2 index " << l2Index << " not found in the page table for VPN: " << VPN << std::endl;
        return -1;
    }

    // get the frame number for the VPN
    PageTableEntry &entry = it2->second;
    int pfn = entry.frameNumber; // get the frame number

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