
#include "PageTableEntry.h"

// Constructor
PageTableEntry::PageTableEntry(uint32_t frameNumber,
                               bool valid,
                               bool dirty,
                               bool read,
                               bool write,
                               bool execute,
                               uint8_t reference)
    : frameNumber(frameNumber), valid(valid), dirty(dirty),
      read(read), write(write), execute(execute), reference(reference) {}

void PageTableEntry::reset()
{
    frameNumber = static_cast<uint32_t>(-1); // Using UINT32_MAX to represent invalid frame number
    valid = false;
    dirty = false;
    read = false;
    write = false;
    execute = false;
    reference = 0;
}

// Increment the reference level, with max level 3
void PageTableEntry::referenceInc()
{
    if (reference < 3)
    {
        reference++;
    }
}

// Decrement the reference level, with min level 0
void PageTableEntry::referenceDec()
{
    if (reference > 0)
    {
        reference--;
    }
}
