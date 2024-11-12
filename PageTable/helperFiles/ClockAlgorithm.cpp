#include "ClockAlgorithm.h"
#include "../PageTable.h"
#include <algorithm>
#include <iostream>

using namespace std;

ClockAlgorithm::ClockAlgorithm()
{
    clockHand = activePages.begin(); // initialize clockHand to the beginning of the list
}

// According to the Clock Algorithm, add a new page to the activePages list.
void ClockAlgorithm::addPage(uint32_t VPN)
{
    if (activeVPNs.find(VPN) == activeVPNs.end())
    {
        activePages.push_back(VPN);
        activeVPNs.insert(VPN);
        if (activePages.size() == 1)
        {
            clockHand = activePages.begin();
        }
    }
}

// According to the Clock Algorithm, remove a page from the activePages list.
void ClockAlgorithm::removePage(uint32_t VPN)
{
    // find the VPN in the activePages list, if it exists, return an iterator to it
    auto it = std::find(activePages.begin(), activePages.end(), VPN);

    if (it != activePages.end()) // if the VPN is found in the activePages list
    {
        // if the VPN is the current clockHand, move the clockHand to the next element
        if (it == clockHand)
        {
            moveClockHandNext();
            if (clockHand == activePages.end()) clockHand = activePages.begin();
        }
        // remove the VPN from the activeVPNs set
        activePages.erase(it);
        activeVPNs.erase(VPN);
        
        if (activePages.empty()) // if the activePages list is empty, reset the clockHand
        {
            clockHand = activePages.end(); // prevent dereferencing an empty iterator
        }
    }
}

// scan the activePages list to find a page to replace, based on the reference bit of each page
bool ClockAlgorithm::selectPageToReplace(uint32_t &targetVPN, PageTable &pageTable)
{
    if (activePages.empty())
    {
        std::cerr << "Error: No active pages available for replacement." << std::endl;
        return false;
    }

    int maxScans = activePages.size(); // maximum number of scans before resetting reference bits
    int scans = 0;
    cout << "Selecting page to replace. Total active pages: " << activePages.size() << endl;

    while (true) // until a page to replace is found
    {
        if (scans >= maxScans)
        {
            cout << "Completed one full scan, resetting reference bits" << endl;

            // after one complete scan, reset the reference bits for all active pages, and reset the scan count
            for (uint32_t vpn : activePages)
            {
                PageTableEntry *entry = pageTable.getPageTableEntry(vpn);
                if (entry && entry->reference > 0)
                {
                    entry->referenceDec(); // decrement the reference bit
                }
            }
            scans = 0; // reset the scan count
        }

        // place the clock hand back at the beginning if it reaches the end of the list
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }

        // get the VPN at the current clock hand position
        uint32_t currentVPN = *clockHand;
        PageTableEntry *entry = pageTable.getPageTableEntry(currentVPN); // get the page table entry for the current VPN

        //  --------------------------------------------
        if (!entry || !pageTable.isValidRange(currentVPN))
        {
            cerr << "Error: Invalid or out-of-range PageTableEntry for VPN: " << currentVPN << ". Removing from active pages." << std::endl;
            removePage(currentVPN);
            moveClockHandNext();
            scans++;
            continue;
        }
        cout << "Scanning VPN: " << currentVPN << ", Reference: " << entry->reference << endl;

        if (entry->reference == 0)
        {
            targetVPN = currentVPN;
            cout << "Selected VPN to replace: " << targetVPN << endl;
            moveClockHandNext(); // 将时钟指针移至下一个页面
            return true;
        }

        moveClockHandNext();
        scans++;
    }

    return false; // should never reach here
}

// reset the clock algorithm by clearing the activePages list and activeVPNs set
void ClockAlgorithm::reset()
{
    activePages.clear();
    activeVPNs.clear();
    clockHand = activePages.begin();
}

// move the clock hand to the next position in the activePages list
void ClockAlgorithm::moveClockHandNext()
{
    if (activePages.empty())
    {
        return;
    }
    ++clockHand;
    if (clockHand == activePages.end())
    {
        clockHand = activePages.begin();
    }
}
