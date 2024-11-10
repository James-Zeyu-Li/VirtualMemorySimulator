// physicalFrame.cpp
#include <queue>
#include <vector>
#include <iostream>
using namespace std;
#include "physicalFrameManger.h"

PhysicalFrameManager::PhysicalFrameManager(uint32_t totalFrames) : totalFrames(totalFrames)
{
    for (uint32_t i = 0; i < totalFrames; ++i)
    {
        freeFrames.push(i);
    }
}

// allocate a frame, if no free frames return -1,
// used in pageTable page replacement
uint32_t PhysicalFrameManager::allocateFrame()
{
    if (freeFrames.empty())
    {
        return static_cast<uint32_t>(-1); // Return -1 if no frames are available
    }
    uint32_t frame = freeFrames.front();
    freeFrames.pop();
    return frame;
}

// free a frame, if the frame is invalid, print error message
void PhysicalFrameManager::freeAFrame(uint32_t frame)
{
    if (frame >= totalFrames)
    {
        throw std::invalid_argument("Invalid frame number: " + std::to_string(frame));
    }
    freeFrames.push(frame);
}

// get the total number of frames
// used in page fault to check if page replacement is needed
uint32_t PhysicalFrameManager::getTotalFrames() const
{
    return totalFrames;
}
