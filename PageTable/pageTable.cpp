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

    // Helper function for lookupPageTable, returns frame number if valid, -1 if not.
    int getFrameNumber(int VPN, int PFN)
    {
        if (table.find(VPN) != table.end() &&
            table[VPN].find(PFN) != table[VPN].end() &&
            table[VPN][PFN].valid)
        {
            return table[VPN][PFN].frameNumber;
        }
        return -1;
    }

public:
    PageTable() = default;

    int lookupPageTable(int VPN, int PFN)
    {
        return getFrameNumber(VPN, PFN);
    }


    void setPageTableEntry(int vpn, int pfn, int frameNumber, bool valid)
    {
        table[vpn][pfn] = PageTableEntry(frameNumber, valid);
    }
};
