// physicalFrame.cpp
#include <queue>
#include <vector>
#include <iostream>
using namespace std;

class PhysicalFrameManager
{
private:
    queue<int> freeFrames; // to store the free frames
    int totalFrames;

public:
    // Constructor to initialize the total number of frames
    PhysicalFrameManager(int totalFrames) : totalFrames(totalFrames)
    {
        for (int i = 0; i < totalFrames; ++i)
        {
            freeFrames.push(i);
        }
    }

    // allocate a frame, if no free frames return -1
    int allocateFrame()
    {
        if (freeFrames.empty())
        {
            return -1;
        }
        int frame = freeFrames.front();
        freeFrames.pop();
        return frame;
    }

    void freeAFrame(int frame)
    {
        if (frame < 0 || frame >= totalFrames)
        {
            std::cerr << "Invalid frame number." << std::endl;
            return;
        }
        freeFrames.push(frame);
    }

    // get the total number of frames
    int getTotalFrames() const
    {
        return totalFrames;
    }
};