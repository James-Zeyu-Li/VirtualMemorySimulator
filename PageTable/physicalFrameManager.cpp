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

    // release a frame
    void makeFrameFree(int frameNumber)
    {
        if (frameNumber < 0 || frameNumber >= totalFrames)
        {
            cerr << "Invalid frame number: " << frameNumber << endl;
            return;
        }
        freeFrames.push(frameNumber);
    }

    // check if there are free frames
    bool hasFreeFrames() const
        return !freeFrames.empty();
    }

    // get the total number of frames
    int getTotalFrames() const
    {
        return totalFrames;
    }
};