cmake_minimum_required(VERSION 3.29)
project(VirtualMemorySimulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(VirtualMemorySimulator
    PageTable/test/testPage.cpp
    PageTable/pageTable.cpp
    PageTable/pageTableEntry.cpp
    PageTable/physicalFrameManager.cpp
        PageTable/TLB.cpp
        PageTable/MemoryManager.h
        PageTable/MemoryManager.cpp
        PageTable/PhysicalFrameManager.h
        PageTable/test/MemoryManagerTest.cpp)
