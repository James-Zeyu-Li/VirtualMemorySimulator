
#include <cstdint>

class PageTableEntry
{
public:
    uint32_t frameNumber;
    bool valid;
    bool dirty;
    bool read;
    bool write;
    bool execute;
    uint8_t reference; // This is used for the clock algorithm, binary

    // Constructor
    PageTableEntry(uint32_t frameNumber = -1,
                   bool valid = false,
                   bool dirty = false,
                   bool read = false,
                   bool write = false,
                   bool execute = false,
                   uint8_t reference = 0) : frameNumber(frameNumber), valid(valid), dirty(dirty),
                                            read(read), write(write), execute(execute), reference(reference) {}

    void reset()
    {
        frameNumber = -1;
        valid = false;
        dirty = false;
        read = false;
        write = false;
        execute = false;
        reference = 0;
    }

    // Increment the reference level, the max level is 3
    void referenceInc() {
        if (reference < 3) {
            reference++;
        }
    }

    void referenceDec() {
        if (reference > 0) {
            reference--;
        }
    }

};