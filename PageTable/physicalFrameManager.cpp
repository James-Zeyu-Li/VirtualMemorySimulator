// physicalFrame.cpp
#include <queue>
#include <vector>
#include <iostream>
using namespace std;

class PhysicalFrameManager
{
private:
    queue<uint32_t> freeFrames; // to store the free frames
    uint32_t totalFrames;

public:
    // Constructor to initialize the total number of frames
    // used in page table to initialize
    PhysicalFrameManager(uint32_t totalFrames) : totalFrames(totalFrames)
    {
        for (uint32_t i = 0; i < totalFrames; ++i)
        {
            freeFrames.push(i);
        }
    }

    // allocate a frame, if no free frames return -1,
    // used in pageTable page replacement
    uint32_t allocateFrame()
    {
        if (freeFrames.empty())
        {
            return -1;
        }
        uint32_t frame = freeFrames.front();
        freeFrames.pop();
        return frame;
    }

    // free a frame, if the frame is invalid, print error message
    void freeAFrame(uint32_t frame)
    {
        if (frame < 0 || frame >= totalFrames)
        {
            throw invalid_argument("Invalid frame number: " + to_string(frame));
        }
        freeFrames.push(frame);
    }

    // get the total number of frames
    // used in page fault to check if page replacement is needed
    int getTotalFrames() const
    {
        return totalFrames;
    }
};