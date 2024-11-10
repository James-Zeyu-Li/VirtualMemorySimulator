#include <iostream>
#include <unordered_map>
#include <ctime>

using namespace std;

class TLBEntry {
public:
    uint32_t vpn;  // Virtual Page Number
    uint32_t pfn;  // Physical Frame Number
    bool valid;
    bool read;
    bool write;
    bool execute;
    long lastAccessTime;

    // Constructor
    TLBEntry() : vpn(-1), pfn(-1), valid(false), read(false), write(false), execute(false), lastAccessTime(0) {}

    TLBEntry(uint32_t vpn, uint32_t pfn, bool valid, bool read, bool write, bool execute, long lastAccessTime)
        : vpn(vpn), pfn(pfn), valid(valid), read(read), write(write), execute(execute), lastAccessTime(lastAccessTime) {}
};

class TLB {
public:
    uint32_t size;  // TLB size
    unordered_map<uint32_t, TLBEntry> entries;  // Unordered map to simulate the TLB

    TLB(uint32_t size) : size(size) {}

    // Lookup function to check if a VPN is in TLB
    int lookupTLB(uint32_t vpn) {
        auto expectEntry = entries.find(vpn);
        // checks if the VPN was found in the map and if the entry is valid.
        if (expectEntry != entries.end() && expectEntry->second.valid) {
            // Update access time
            expectEntry->second.lastAccessTime = time(0);
            return expectEntry->second.pfn;
        }
        return -1;  // TLB miss
    }

    // Update TLB with a new entry or modify an existing one
    void updateTLB(uint32_t vpn, uint32_t pfn, bool read, bool write, bool execute) {
        long currentTime = time(0);
        entries[vpn] = TLBEntry(vpn, pfn, true, read, write, execute, currentTime);
    }

    // Flush the entire TLB
    void flush() {
        entries.clear();
    }

    // Evict an entry if TLB is full
    void evictIfNeeded() {
    // If the TLB is full
        if (entries.size() >= size) {
            long oldestAccessTime = LONG_MAX;  // Start with a very large value to ensure we find the least recent
            int vpnToEvict = -1;  // This will hold the VPN of the entry to evict
            
            // Iterate through all entries to find the one with the oldest access time (LRU)
            for (const auto& entry : entries) {
                // Compare the access time of each entry with the current oldest one
                if (entry.second.lastAccessTime < oldestAccessTime) {
                    vpnToEvict = entry.first;  // Update the VPN of the entry to evict
                    oldestAccessTime = entry.second.lastAccessTime;  // Update the oldest access time
                }
            }
            
            // Evict the least recently used entry (with the smallest lastAccessTime)
            if (vpnToEvict != -1) {
                entries.erase(vpnToEvict);  // Remove the entry from the TLB
            }
        }
    }
};