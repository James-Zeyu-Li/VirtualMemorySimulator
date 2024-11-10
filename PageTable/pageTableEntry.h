#ifndef PAGETABLEENTRY_H
#define PAGETABLEENTRY_H

#include <cstdint>

class PageTableEntry
{
public:
    uint32_t frameNumber; // The frame number associated with this page entry
    bool valid;           // Valid bit
    bool dirty;           // Dirty bit
    bool read;            // Read permission
    bool write;           // Write permission
    bool execute;         // Execute permission
    bool reference;       // Reference bit for clock algorithm (0 or 1)

    // Constructor with default parameters
    PageTableEntry(uint32_t frameNumber = static_cast<uint32_t>(-1),
                   bool valid = false,
                   bool dirty = false,
                   bool read = false,
                   bool write = false,
                   bool execute = false,
                   bool reference = false);

    // Reset the page entry to default values
    void reset();

    // Set reference bit to true
    void referenceInc();

    // Set reference bit to false
    void referenceDec();
};

#endif // PAGETABLEENTRY_H
