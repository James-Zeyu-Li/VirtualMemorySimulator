// This is the implementation of the page table
#include <unordered_map>
#include <cstdint>
#include "pageTableEntry.cpp"
#include <queue>
using namespace std;

class PageTable
{
private:
    // 2 layer map to simulate 2 layered page table
    unordered_map<int, unordered_map<int, PageTableEntry>> pageTable;
    static const int pageSize = 4096; // 4KB
    int pageFaults = 0;
    int pageHits = 0;
    int getL1Index(int VPN) { return VPN >> 10; }
    int getL2Index(int VPN) { return VPN & 0x3FF; }
    queue<int> freeFrames; // queue to keep track of free frames in physical memory
    int clockHand = 0;     // used for the clock algorithm

    // Check if the second level map exists, if not create one
    unordered_map<int, PageTableEntry> &checkL2(int l1Index)
    {
        if (pageTable.find(l1Index) == pageTable.end())
        {
            pageTable[l1Index] = unordered_map<int, PageTableEntry>();
        }
        return pageTable[l1Index]; // return the second level map
    }

public:
    PageTable() = default;

    // Initiate page table with a given size
    PageTable(int totalFrames)
    {
        for (int i = 0; i < totalFrames; i++)
        {
            freeFrames.push(i);
        }
    };

    // Initiate page table with a given size 4KB
    void addPageTableEntry(int VPN, int PFN, int frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference)
    {
        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        pageTable[l1Index][l2Index] = PageTableEntry(frameNumber, valid, dirty, read, write, execute, reference);
    }

    void updatePageTable(int vpn, int pfn, bool valid, bool read, bool write, bool execute, bool dirty, uint8_t reference)
    {
        int l1Index = getL1Index(vpn);
        int l2Index = getL2Index(vpn);

        auto &entry = checkL2(l1Index)[l2Index];
        entry.frameNumber = pfn;
        entry.valid = valid;
        entry.read = read;
        entry.write = write;
        entry.execute = execute;
        entry.dirty = dirty;
        entry.reference = reference;
    }

    // Lookup the page table for a given VPN and PFN
    int lookupPageTable(int VPN)
    {

        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        if (pageTable.find(l1Index) != pageTable.end() &&
            pageTable[l1Index].find(l2Index) != pageTable[l1Index].end() &&
            pageTable[l1Index][l2Index].valid)
        {
            incrementPageHits();
            pageTable[l1Index][l2Index].referenceInc(); // increment the reference counter when used
            return pageTable[l1Index][l2Index].frameNumber;
        }
        else
        {
            // Page fault
            // could call page fault handler here to handle the page fault
            incrementPageFaults();
            return -1;
        }
    }

    // Handle page fault by replacing a page in memory, using the clock algorithm
    void handlePageFault(int vpn)
    {
        if (!freeFrames.empty())
        {
            // Allocate new frame directly to the empty frame
            int newFrame = freeFrames.front();
            freeFrames.pop();
            updatePageTable(vpn, newFrame, true, false, false, false, false, 0);
            return;
        }
        else
        {
            // Clock algorithm
            while (true)
            {
                int l1Index = getL1Index(clockHand);
                int l2Index = getL2Index(clockHand);

                if (pageTable.find(l1Index) != pageTable.end() &&
                    pageTable[l1Index].find(l2Index) != pageTable[l1Index].end())
                {

                    if (pageTable[l1Index][l2Index].reference == 0)
                    {
                        // Store old frame number to replace
                        int oldFrame = pageTable[l1Index][l2Index].frameNumber;

                        // Remove old mapping from page table
                        pageTable[l1Index][l2Index] = PageTableEntry();

                        // Create new mapping
                        updatePageTable(vpn, oldFrame, true, false, false, false, false, 0);
                        break;
                    }
                    else
                    {
                        // Reset reference bit
                        pageTable[l1Index][l2Index].reference = 0;
                    }
                }

                // Update clock hand once per iteration
                clockHand = (clockHand + 1) % pageSize;
            }
        }
    }

    // int allocateNewFrame(int VPN) {

    // }

    // PageTableEntry* getOrCreateSecondLevel (int l1Index)
    // {

    // }

    void resetPageTable()
    {
        pageTable.clear();
        resetPageFaults();
        resetPageHits();
    }

    int getPageFaults() { return pageFaults; }
    int getPageHits() { return pageHits; }

private:
    void incrementPageFaults() { pageFaults++; }
    void incrementPageHits() { pageHits++; }
    void resetPageFaults() { pageFaults = 0; }
    void resetPageHits() { pageHits = 0; }
};
