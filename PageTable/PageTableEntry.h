#ifndef PAGETABLEENTRY_H
#define PAGETABLEENTRY_H

#include <cstdint>
#include <sstream>

class PageTableEntry
{
public:
    uint32_t frameNumber; // The frame number associated with this page entry
    bool valid;           // Valid bit
    bool dirty;           // Dirty bit
    bool read;            // Read permission
    bool write;           // Write permission
    bool execute;         // Execute permission
    uint8_t reference;    // Reference level for clock algorithm (0 to 3)

    // Constructor with default parameters in header file only
    PageTableEntry(uint32_t frameNumber = static_cast<uint32_t>(-1),
                   bool valid = false,
                   bool dirty = false,
                   bool read = false,
                   bool write = false,
                   bool execute = false,
                   uint8_t reference = 0);

    // Reset the page entry to default values
    void reset();

    // Set reference bit to true
    void referenceInc();

    // Set reference bit to false
    void referenceDec();

    // Return a string representation of the page entry
    std::string toString() const
    {
        std::ostringstream oss;
        oss << "{ Frame: " << frameNumber
            << ", Valid: " << valid
            << ", Dirty: " << dirty
            << ", Reference: " << static_cast<int>(reference)
            << " }";
        return oss.str();
    }
};

#endif // PAGETABLEENTRY_H
