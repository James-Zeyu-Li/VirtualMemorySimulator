#ifndef TLBENTRY_H
#define TLBENTRY_H

#include <cstdint>

class TLBEntry {
public:
    uint32_t vpn;  // Virtual Page Number
    uint32_t pfn;  // Physical Frame Number
    bool valid;
    bool read;
    bool write;
    bool execute;
    long lastAccessTime;

    // Constructors
    TLBEntry();
    TLBEntry(uint32_t vpn, uint32_t pfn, bool valid, bool read, bool write, bool execute, long lastAccessTime);
};

#endif // TLBENTRY_H