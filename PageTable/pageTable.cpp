// This is the implementation of the page table
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include "PageTableEntry.h"
#include "PhysicalFrameManager.h"
#include <list>
#include <unordered_set>
using namespace std;

class PageTable
{
private:
    // 2 layer map to simulate 2 layered page table
    unordered_map<uint32_t, unordered_map<uint32_t, PageTableEntry> > pageTable;
    static const uint64_t addressSpaceSize = 4ULL * 1024 * 1024 * 1024; // 32-bit address space
    static const int pageSize = 4096;                                   // 4KB
    static const int addressBits = 32;
    static const int pageOffsetBits = 12; // log2(pageSize)
    static const int vpnBits = addressBits - pageOffsetBits;

    static const int l1Bits = vpnBits / 2;
    static const int l2Bits = vpnBits - l1Bits;

    int getL1Index(uint32_t VPN) { return VPN >> l2Bits; }             // shift right by l2Bits
    int getL2Index(uint32_t VPN) { return VPN & ((1 << l2Bits) - 1); } // get the last l2Bits

    // Physical frame manager to manage the physical frames
    PhysicalFrameManager pfManager;
    // clock hand pair to keep track of the current position in the clock algorithm
    list<uint32_t>::iterator clockHand; // Iterator for clock algorithm

    // Check if the second level map exists, if not create one
    unordered_map<uint32_t, PageTableEntry> &checkL2(uint32_t l1Index)
    {
        if (pageTable.find(l1Index) == pageTable.end()) // if the first level map does not exist
        {
            pageTable[l1Index] = unordered_map<uint32_t, PageTableEntry>(); // create a new second level map
        }
        return pageTable[l1Index]; // return the second level map
    }

    list<uint32_t> activePages;         // Use list for efficient insertion/removal
    unordered_set<uint32_t> activeVPNs; // To avoid duplicate VPNs

    bool isValidVPN(uint32_t VPN)
    {
        return VPN >= 0 && VPN < addressSpaceSize / pageSize;
    }

    // Public methods
public:
    PageTable() : pfManager(256) // Initiate page table with 256 frames
    {
        clockHand = activePages.begin();
    };

    // Initiate page table with a given size
    PageTable(uint32_t totalFrames) : pfManager(totalFrames)
    {
        clockHand = activePages.begin();
    };

    // Lookup the page table for a given VPN and PFN
    int32_t lookupPageTable(uint32_t VPN)
    {
        if (!isValidVPN(VPN))
        {
            cerr << "Invalid VPN." << VPN << "Out of range" << endl;
            return -1;
        }

        uint32_t l1Index = getL1Index(VPN);
        uint32_t l2Index = getL2Index(VPN);

        if (pageTable.find(l1Index) != pageTable.end() &&
            pageTable[l1Index].find(l2Index) != pageTable[l1Index].end() &&
            pageTable[l1Index][l2Index].valid)
        {
            if (pageTable[l1Index][l2Index].reference < 3)
            {
                pageTable[l1Index][l2Index].referenceInc();
                cout << "After incrementing reference, VPN " << VPN
                     << " has reference " << (int)pageTable[l1Index][l2Index].reference << endl;
            }

            // if the page is valid, update the active pages
            if (activeVPNs.insert(VPN).second) // Insert VPN to activeVPNs
            {
                activePages.push_back(VPN);
                if (activePages.size() == 1) // set clockHand to the first element
                {
                    clockHand = activePages.begin();
                }
            }

            return pageTable[l1Index][l2Index].frameNumber;
        }
        else
        {
            return -1;
        }
    }

    // Update the page table with the given VPN and PFN
    void updatePageTable(uint32_t VPN, uint32_t frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference)
    {
        if (!isValidVPN(VPN))
        {
            cerr << "Invalid VPN." << VPN << "Out of range" << endl;
            return;
        }
        uint32_t l1Index = getL1Index(VPN);
        uint32_t l2Index = getL2Index(VPN);

        // Get the second level map
        auto &entry = checkL2(l1Index)[l2Index];

        // Update the page table entry
        entry.frameNumber = frameNumber;
        entry.valid = valid;
        entry.dirty = dirty;
        entry.read = read;
        entry.write = write;
        entry.execute = execute;
        entry.reference = reference;

        // if the page is valid, update the active pages
        if (valid && activeVPNs.insert(VPN).second)
        {
            activePages.push_back(VPN);
            if (activePages.size() == 1) // set clockHand to the first element
            {
                clockHand = activePages.begin();
            }
        }
    }

    // Handle page fault by replacing a page in memory, using the clock algorithm
    bool handlePageFault(uint32_t VPN)
    {
        // check if the VPN is valid
        if (!isValidVPN(VPN))
        {
            cerr << "Invalid VPN." << VPN << "Out of range" << endl;
            return false;
        }

        int newFrame = pfManager.allocateFrame();
        if (newFrame != -1)
        {
            // Add new VPN to activePages and activeVPNs
            updatePageTable(VPN, newFrame, true, false, true, true, true, 0);
            return true;
        }
        else
        {
            if (!replacePageUsingClockAlgo(VPN))
            {
                cerr << "Failed to replace page for VPN: " << VPN << endl;
                return false;
            }
            return true;
        }
    }

    // Reset the page table
    void resetPageTable()
    {
        pageTable.clear();
        activePages.clear();
        activeVPNs.clear();
        clockHand = activePages.begin();
    }

    // Write the page back to disk
    void writeBackToDisk(uint32_t frameNumber)
    {
        cout << "Writing frame " << frameNumber << " back to disk." << endl;
    }

    // Remove the address for one entry
    // delete the VPN from the pageTable and activePages
    void removeAddressForOneEntry(uint32_t VPN)
    {
        uint32_t l1Index = getL1Index(VPN);
        uint32_t l2Index = getL2Index(VPN);

        // delete the VPN from the pageTable
        pageTable[l1Index].erase(l2Index);
        if (pageTable[l1Index].empty())
        {
            pageTable.erase(l1Index);
        }

        // delete the VPN from the activeVPNs
        activeVPNs.erase(VPN);
        removeFromActivePages(VPN);
    }

    // private methods
private:
    // Replace a page in memory using the clock algorithm
    bool replacePageUsingClockAlgo(uint32_t newVPN)
    {
        if (activePages.empty())
        {
            cerr << "Error: No active pages available for replacement." << endl;
            return false;
        }

        const int maxScans = pfManager.getTotalFrames();

        while (true) // loop until a page is replaced
        {
            int scanCount = 0;
            while (scanCount < maxScans)
            {
                PageTableEntry *pageEntry = getPageEntryAtClockHand();
                if (pageEntry && pageEntry->valid)
                {
                    cout << "Checking VPN " << *clockHand << " - Reference: " << pageEntry->reference << endl;

                    if (pageEntry->reference == 0)
                    {
                        int oldVPN = *clockHand;
                        cout << "Replacing VPN " << oldVPN << " with new VPN " << newVPN << endl;
                        handlePageReplacement(newVPN, oldVPN, *pageEntry);
                        return true;
                    }
                }
                else
                {
                    cout << "Skipping invalid or empty page entry at clock hand position." << endl;
                }
                moveClockHandNext();
                scanCount++;
            }

            // if no page is replaced after maxScans, decrement the reference counter for all pages
            decrementAllReferenceCounters();
        }

        cerr << "Error: No replaceable frame found after maximum scans." << endl;
        return false;
    }

    void decrementAllReferenceCounters()
    {
        for (int VPN : activePages)
        {
            int l1Index = getL1Index(VPN);
            int l2Index = getL2Index(VPN);

            auto it1 = pageTable.find(l1Index);
            if (it1 == pageTable.end())
            {
                cerr << "Error: First level map not found." << endl;
                return;
            }

            auto &secondLevel = it1->second;
            auto it2 = secondLevel.find(l2Index);
            if (it2 == secondLevel.end())
            {
                cerr << "Error: Second level map not found." << endl;
                return;
            }

            it2->second.referenceDec();
        }
    }

    // Handle page replacement by replacing the old page with the new page
    void handlePageReplacement(uint32_t newVPN, uint32_t oldVPN, PageTableEntry &oldEntry)
    {

        uint32_t oldFrame = oldEntry.frameNumber;

        // Handle dirty pages
        if (oldEntry.dirty)
        {
            writeBackToDisk(oldFrame);
        }

        oldEntry.valid = false;
        oldEntry.reset();

        // Clean up old page table entry
        uint32_t l1Index = getL1Index(oldVPN);
        uint32_t l2Index = getL2Index(oldVPN);
        pageTable[l1Index].erase(l2Index);
        if (pageTable[l1Index].empty())
        {
            pageTable.erase(l1Index);
        }

        // Remove old VPN from tracking structures
        activeVPNs.erase(oldVPN);
        clockHand = activePages.erase(clockHand);
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }

        // Add new VPN to tracking structures
        activePages.insert(clockHand, newVPN);
        activeVPNs.insert(newVPN);

        // Update page table with new VPN
        updatePageTable(newVPN, oldFrame, true, false, true, true, true, 0);
    }

    // Get the page entry at the current clock hand position
    PageTableEntry *getPageEntryAtClockHand()
    {
        if (activePages.empty())
        {
            cerr << "Error: No active pages found." << endl;
            return nullptr;
        }

        uint32_t currentVPN = *clockHand;
        uint32_t l1Index = getL1Index(currentVPN);
        uint32_t l2Index = getL2Index(currentVPN);

        auto it1 = pageTable.find(l1Index);
        if (it1 == pageTable.end())
        {
            cerr << "Error: First level map not found." << endl;
            return nullptr;
        }

        auto &secondLevel = it1->second;
        auto it2 = secondLevel.find(l2Index);
        if (it2 == secondLevel.end())
        {
            cerr << "Error: Second level map not found." << endl;
            return nullptr;
        }

        return &(it2->second);
    }

    // Move the clock hand to the next position
    void moveClockHandNext()
    {
        if (activePages.empty())
        {
            cerr << "The activePages is empty, clock hand can't be moved" << endl;
            return;
        }

        ++clockHand;
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }
    }

    // Get the page entry at the current clock hand position
    PageTableEntry *handleClockHand()
    {
        PageTableEntry *pageEntry = getPageEntryAtClockHand();
        moveClockHandNext();
        return pageEntry;
    }

    // Remove the address for all entries
    void removeFromActivePages(uint32_t VPN)
    {
        auto it = find(activePages.begin(), activePages.end(), VPN);
        if (it != activePages.end())
        {
            // delete the VPN from the activePages
            if (it == clockHand)
            {
                clockHand = activePages.erase(it);
                if (clockHand == activePages.end())
                {
                    clockHand = activePages.begin();
                }
            }
            else
            {
                activePages.erase(it);
            }
        }
    }

public:
    // for testing purposes only
    void printPageTable() const
    {
        for (const auto &l1Entry : pageTable)
        {
            for (const auto &l2Entry : l1Entry.second)
            {
                const auto &entry = l2Entry.second;
                std::cout << "VPN: " << (l1Entry.first << l2Bits | l2Entry.first)
                          << " -> Frame Number: " << entry.frameNumber
                          << ", Valid: " << entry.valid
                          << ", Dirty: " << entry.dirty
                          << ", Reference: " << (int)entry.reference
                          << std::endl;
            }
        }
    }
};
