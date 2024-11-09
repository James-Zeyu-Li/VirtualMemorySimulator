// This is the implementation of the page table
#include <unordered_map>
#include <cstdint>
#include "pageTableEntry.cpp"
#include <queue>
#include "physicalFrameManager.cpp"
#include <list>
#include <unordered_set>
// #include <cmath>
// #define LOG2(x) ((x) <= 1 ? 0 : 1 + LOG2((x) / 2))
using namespace std;

class PageTable
{
private:
    // 2 layer map to simulate 2 layered page table
    unordered_map<int, unordered_map<int, PageTableEntry>> pageTable;
    static const uint64_t addressSpaceSize = 4ULL * 1024 * 1024 * 1024; // 32-bit address space
    static const int pageSize = 4096;                                   // 4KB
    static const int addressBits = 32;
    static const int pageOffsetBits = 12; // log2(pageSize)
    static const int vpnBits = addressBits - pageOffsetBits;

    static const int l1Bits = vpnBits / 2;
    static const int l2Bits = vpnBits - l1Bits;

    int getL1Index(int VPN) { return VPN >> l2Bits; }
    int getL2Index(int VPN) { return VPN & ((1 << l2Bits) - 1); }

    // Physical frame manager to manage the physical frames
    PhysicalFrameManager pfManager;

    // clock hand pair to keep track of the current position in the clock algorithm
    list<int>::iterator clockHand; // Iterator for clock algorithm

    // Check if the second level map exists, if not create one
    unordered_map<int, PageTableEntry> &checkL2(int l1Index)
    {
        if (pageTable.find(l1Index) == pageTable.end()) // if the first level map does not exist
        {
            pageTable[l1Index] = unordered_map<int, PageTableEntry>();
        }
        return pageTable[l1Index]; // return the second level map
    }

    list<int> activePages;         // Use list for efficient insertion/removal
    unordered_set<int> activeVPNs; // To avoid duplicate VPNs

    bool isValidVPN(int VPN)
    {
        return VPN >= 0 && VPN < addressSpaceSize / pageSize;
    }

    // Public methods
public:
    PageTable() : pfManager(0)
    {
        clockHand = activePages.begin();
    };

    // Initiate page table with a given size
    PageTable(int totalFrames) : pfManager(totalFrames)
    {
        clockHand = activePages.begin();
    };

    // Update the page table with the given VPN and PFN
    void updatePageTable(int VPN, int frameNumber, bool valid, bool read, bool write, bool execute, bool dirty, uint8_t reference)
    {
        if (!isValidVPN(VPN))
        {
            cerr << "Invalid VPN." << VPN << "Out of range" << endl;
            return;
        }
        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        // Get the second level map
        auto &entry = checkL2(l1Index)[l2Index];

        // Update the page table entry
        entry.frameNumber = frameNumber;
        entry.valid = valid;
        entry.read = read;
        entry.write = write;
        entry.execute = execute;
        entry.dirty = dirty;
        entry.reference = reference;

        // if the page is valid, update the active pages
        if (activeVPNs.insert(VPN).second) // Insert VPN to activeVPNs
        {
            activePages.push_back(VPN);
            if (activePages.size() == 1) // set clockHand to the first element
            {
                clockHand = activePages.begin();
            }
        }
    }

    // Lookup the page table for a given VPN and PFN
    int lookupPageTable(int VPN)
    {
        if (!isValidVPN(VPN))
        {
            cerr << "Invalid VPN." << VPN << "Out of range" << endl;
            return -1;
        }

        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        if (pageTable.find(l1Index) != pageTable.end() &&
            pageTable[l1Index].find(l2Index) != pageTable[l1Index].end() &&
            pageTable[l1Index][l2Index].valid)
        {
            pageTable[l1Index][l2Index].reference = 3; // set to 3

            for (auto &l1Entry : pageTable)
            {
                for (auto &l2Entry : l1Entry.second)
                {
                    // Skip the current page
                    if (l1Entry.first == l1Index && l2Entry.first == l2Index)
                        continue;

                    // decrement reference for all other pages if reference > 0
                    if (l2Entry.second.reference > 0)
                        l2Entry.second.reference--;
                }
            }
            return pageTable[l1Index][l2Index].frameNumber;
        }
        else
        {
            return -1;
        }
    }

    // Handle page fault by replacing a page in memory, using the clock algorithm
    void handlePageFault(int VPN)
    {
        int newFrame = pfManager.allocateFrame();
        if (newFrame != -1)
        {
            // Add new VPN to activePages and activeVPNs
            updatePageTable(VPN, newFrame, true, false, false, false, false, 0);
        }
        else
        {
            replacePageUsingClockAlgo(VPN);
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

    // private methods
private:
    // Replace a page in memory using the clock algorithm
    void replacePageUsingClockAlgo(int VPN)
    {
        const int maxReferenceLevel = 3; // set to 3

        // start from 0 to maxReferenceLevel
        for (int targetReference = 0; targetReference <= maxReferenceLevel; ++targetReference)
        {
            int scanCount = 0;
            const int maxScans = pfManager.getTotalFrames(); // one full scan

            // start scanning the active pages
            while (scanCount < maxScans)
            {
                PageTableEntry *pageEntry = getPageEntryAtClockHand();
                if (!pageEntry)
                {
                    moveClockHandNext();
                    scanCount++;
                    continue;
                }

                // If the page is valid and has the target reference, replace it
                if (pageEntry->valid && pageEntry->reference == targetReference)
                {
                    handlePageReplacement(VPN, *pageEntry);
                    return; // page replaced
                }

                moveClockHandNext();
                scanCount++;
            }
        }

        // If no page is replaced, throw an exception
        cerr << "Exceeded max scans. No frames can be replaced." << endl;
        return;
    }

    // Get the page entry at the current clock hand position
    PageTableEntry *getPageEntryAtClockHand()
    {
        if (activePages.empty())
            return nullptr;

        int currentVPN = *clockHand;
        int l1Index = getL1Index(currentVPN);
        int l2Index = getL2Index(currentVPN);

        auto it1 = pageTable.find(l1Index);
        if (it1 == pageTable.end())
            return nullptr;

        auto &secondLevel = it1->second;
        auto it2 = secondLevel.find(l2Index);
        if (it2 == secondLevel.end())
            return nullptr;

        return &(it2->second);
    }

    // Move the clock hand to the next position
    void moveClockHandNext()
    {
        if (activePages.empty())
            return;
        ++clockHand;
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }
    }

    // Handle page replacement by replacing the old page with the new page
    void handlePageReplacement(int VPN, PageTableEntry &oldEntry)
    {
        int oldFrame = oldEntry.frameNumber;
        int oldVPN = *clockHand;

        if (oldEntry.dirty)
        {
            writeBackToDisk(oldFrame);
        }
        oldEntry.reset();

        // Remove old VPN from activePages and activeVPNs
        clockHand = activePages.erase(clockHand);
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }
        activeVPNs.erase(oldVPN);

        // Add new VPN to activePages and activeVPNs
        activePages.insert(clockHand, VPN);
        activeVPNs.insert(VPN);

        // Update page table with new VPN
        updatePageTable(VPN, oldFrame, true, false, false, false, false, 0);
    }

    // Write the page back to disk
    void writeBackToDisk(int frameNumber)
    {
        cout << "Writing frame " << frameNumber << " back to disk." << endl;
    }
};
