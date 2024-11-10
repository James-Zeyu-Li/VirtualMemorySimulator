#ifndef PHYSICALFRAMEMANAGER_H
#define PHYSICALFRAMEMANAGER_H

#include <queue>
#include <cstdint>
#include <stdexcept>
#include <string>

class PhysicalFrameManager
{
private:
    std::queue<uint32_t> freeFrames; // Queue to store free frames
    uint32_t totalFrames;            // Total number of frames

public:
    // Constructor to initialize the total number of frames
    PhysicalFrameManager(uint32_t totalFrames);

    // Allocate a frame; return -1 if no free frames are available
    uint32_t allocateFrame();

    // Free a frame; throw an error if the frame is invalid
    void freeAFrame(uint32_t frame);

    // Get the total number of frames
    uint32_t getTotalFrames() const;
};

#endif // PHYSICALFRAMEMANAGER_H
