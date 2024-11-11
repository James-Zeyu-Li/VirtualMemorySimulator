#ifndef TLB_H
#define TLB_H

#include <unordered_map>
#include <cstdint>
#include "TLBEntry.h"

class TLB {
public:
    uint32_t size;  // TLB size
    std::unordered_map<uint32_t, TLBEntry> entries;  // Unordered map to simulate the TLB

    TLB(uint32_t size);

    // Lookup function to check if a VPN is in TLB
    int lookupTLB(uint32_t vpn);

    // Update TLB with a new entry or modify an existing one
    void updateTLB(uint32_t vpn, uint32_t pfn, bool read, bool write, bool execute);

    // Delete one entry from the TLB by VPN
    void deleteTLB(uint32_t vpn);

    // Flush the entire TLB
    void flush();

    // Evict an entry if TLB is full
    void evictIfNeeded();
};

#endif // TLB_H
