#include "TLBEntry.h"
#include <ctime>  // For time()

// Default constructor
TLBEntry::TLBEntry() 
    : vpn(0), pfn(0), valid(false), read(false), write(false), execute(false), lastAccessTime(0) {}

// Parameterized constructor
TLBEntry::TLBEntry(uint32_t vpn, uint32_t pfn, bool valid, bool read, bool write, bool execute, long lastAccessTime)
    : vpn(vpn), pfn(pfn), valid(valid), read(read), write(write), execute(execute), lastAccessTime(lastAccessTime) {}