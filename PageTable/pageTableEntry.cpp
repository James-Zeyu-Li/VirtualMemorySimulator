
#include "pageTableEntry.h"

// Constructor
PageTableEntry::PageTableEntry(uint32_t frameNumber = static_cast<uint32_t>(-1),
                               bool valid = false,
                               bool dirty = false,
                               bool read = false,
                               bool write = false,
                               bool execute = false,
                               uint8_t reference = 0) : frameNumber(frameNumber), valid(valid), dirty(dirty),
                                                        read(read), write(write), execute(execute), reference(reference) {}

void PageTableEntry::reset()
{
    frameNumber = static_cast<uint32_t>(-1);
    valid = false;
    dirty = false;
    read = false;
    write = false;
    execute = false;
    reference = 0;
}

// Increment the reference level, the
void PageTableEntry::referenceInc()
{
    if (reference == false)
    {
        reference = true;
    }
}

void PageTableEntry::referenceDec()
{
    if (reference == true)
    {
        reference = false;
    }
}
