// This is the implementation of the page table
#include <unordered_map>
#include <cstdint>
#include "pageTableEntry.cpp"
using namespace std;

class PageTable
{
private:
    // 2 layer map to simulate 2 layered page table
    unordered_map<int, unordered_map<int, PageTableEntry>> table;
    static const int pageSize = 4096; // 4KB
    int pageFaults = 0;
    int pageHits = 0;
    int getL1Index(int VPN) { return VPN >> 10; }
    int getL2Index(int VPN) { return VPN & 0x3FF; }

public:
    PageTable() = default;

    // Initiate page table with a given size 4KB
    void addPageTableEntry(int VPN, int PFN, int frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference)
    {
        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        table[l1Index][l2Index] = PageTableEntry(frameNumber, valid, dirty, read, write, execute, reference);
    }

    // Lookup the page table for a given VPN and PFN
    int lookupPageTable(int VPN)
    {

        int l1Index = getL1Index(VPN);
        int l2Index = getL2Index(VPN);

        if (table.find(l1Index) != table.end() &&
            table[l1Index].find(l2Index) != table[l1Index].end() &&
            table[l1Index][l2Index].valid)
        {
            incrementPageHits();
            return table[l1Index][l2Index].frameNumber;
        }
        return -1;
    }

    // Void updatePageTable(int vpn, int pfn, bool valid, bool read, bool write, bool execute){

    // }

    // Void handlePageFault(int vpn)
    // {

    // }

    // PageTableEntry* getOrCreateSecondLevel (int l1Index)
    // {

    // }

private:
    void incrementPageFaults() { pageFaults++; }
    void incrementPageHits() { pageHits++; }
    void resetPageFaults() { pageFaults = 0; }
    void resetPageHits() { pageHits = 0; }
    unordered_map<int, PageTableEntry> getL1Entry(int l1Index)
    {
        if (table.find(l1Index) == table.end())
        {
            table[l1Index] = unordered_map<int, PageTableEntry>();
        }
    }
};
