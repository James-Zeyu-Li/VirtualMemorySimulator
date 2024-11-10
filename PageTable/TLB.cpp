#include "TLB.h"
#include <ctime>
#include <climits>
// how nany entries are in the TLB
TLB::TLB(uint32_t size) : size(size) {}

// Lookup function to check if a VPN is in TLB
int TLB::lookupTLB(uint32_t vpn) {
    auto expectEntry = entries.find(vpn);
    // Checks if the VPN was found in the map and if the entry is valid.
    if (expectEntry != entries.end() && expectEntry->second.valid) {
        // Update access time
        expectEntry->second.lastAccessTime = time(0);
        return expectEntry->second.pfn;
    }
    return -1;  // TLB miss
}

// Update TLB with a new entry or modify an existing one
void TLB::updateTLB(uint32_t vpn, uint32_t pfn, bool read, bool write, bool execute) {
    long currentTime = time(0);
    if (entries.size() >= size) {
        evictIfNeeded();
    }
    entries[vpn] = TLBEntry(vpn, pfn, true, read, write, execute, currentTime);
}

// Flush the entire TLB
void TLB::flush() {
    entries.clear();
}

// Evict an entry if TLB is full
void TLB::evictIfNeeded() {
    long oldestAccessTime = LONG_MAX;  // Start with a very large value to ensure we find the least recent
    uint32_t vpnToEvict = UINT32_MAX;  // This will hold the VPN of the entry to evict

    // Iterate through all entries to find the one with the oldest access time (LRU)
    for (const auto& entry : entries) {
        // Compare the access time of each entry with the current oldest one
        if (entry.second.lastAccessTime < oldestAccessTime) {
            vpnToEvict = entry.first;  // Update the VPN of the entry to evict
            oldestAccessTime = entry.second.lastAccessTime;  // Update the oldest access time
        }
    }

    // Evict the least recently used entry (with the smallest lastAccessTime)
    if (vpnToEvict != UINT32_MAX) {
        entries.erase(vpnToEvict);  // Remove the entry from the TLB
    }
}
