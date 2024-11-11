#ifndef CLOCKALGORITHM_H
#define CLOCKALGORITHM_H

#include <list>
#include <unordered_set>
#include <cstdint>
#include "PageTableEntry.h"

// Forward declaration to avoid circular dependency
class PageTable;

class ClockAlgorithm
{
private:
    std::list<uint32_t> activePages;         // Tracks active VPNs in a circular list
    std::list<uint32_t>::iterator clockHand; // Iterator pointing to current clock position
    std::unordered_set<uint32_t> activeVPNs; // To avoid duplicate VPNs

public:
    ClockAlgorithm();

    // Add a new VPN to active pages
    void addPage(uint32_t VPN);

    // Remove a VPN from active pages
    void removePage(uint32_t VPN);

    // Select a VPN to replace using the clock algorithm
    // Returns true and sets targetVPN if a target is found
    // Returns false if no target is found
    bool selectPageToReplace(uint32_t &targetVPN, PageTable &pageTable);

    // Reset the clock algorithm
    void reset();

private:
    // Move the clock hand to the next position
    void moveClockHandNext();

    // Get the page entry at the current clock hand position
    PageTableEntry *getPageEntryAtClockHand();
};

#endif // CLOCKALGORITHM_H
