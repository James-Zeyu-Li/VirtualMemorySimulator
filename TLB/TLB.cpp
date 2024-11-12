#include "TLB.h"
#include "TLBEntry.h"
#include <climits>
#include <iostream>
#include <chrono>

// Constructor for TLB, initializing with the given size
TLB::TLB(uint32_t size) : size(size) {}

// Lookup function to check if a VPN is in TLB
int TLB::lookupTLB(uint32_t vpn) {
    auto expectEntry = entries.find(vpn);
    // Checks if the VPN was found in the map and if the entry is valid.
    if (expectEntry != entries.end() && expectEntry->second.valid) {
        // Update access time
        expectEntry->second.lastAccessTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return expectEntry->second.pfn;
    }
    return -1;  // Return -1 if the entry is not found or invalid
}

// Update TLB with a new entry or modify an existing one
void TLB::updateTLB(uint32_t vpn, uint32_t pfn, bool read, bool write, bool execute) {
    // Check if TLB needs to evict an entry
    if (entries.size() >= size){
        evictIfNeeded();
    }

    // Update the entry in the TLB with current time in milliseconds
    entries[vpn] = TLBEntry(vpn, pfn, true, read, write, execute, 
                            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}

// Delete one entry from the TLB by VPN
void TLB::deleteTLB(uint32_t vpn) {
    entries.erase(vpn);
}

// Flush the entire TLB
void TLB::flush() {
    entries.clear();
}


// Eviction logic if needed based on most recent access (LRU)
void TLB::evictIfNeeded() {
    long oldestAccessTime = LONG_MIN;  // Start with the smallest value to find the least recent
    uint32_t vpnToEvict = UINT32_MAX;  // This will hold the VPN of the entry to evict

    // Iterate through all entries to find the one with the oldest access time (LRU)
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        const TLBEntry& entry = it->second;
        // If the current entry has an older access time, update the oldest access time and VPN to evict
        if (entry.lastAccessTime < oldestAccessTime) {
            vpnToEvict = it->first;
            oldestAccessTime = entry.lastAccessTime;
        }
    }

    // Evict the least recently used entry (with the smallest lastAccessTime)
    if (vpnToEvict != UINT32_MAX) {
        entries.erase(vpnToEvict);  // Remove the entry from the TLB
    }
}
