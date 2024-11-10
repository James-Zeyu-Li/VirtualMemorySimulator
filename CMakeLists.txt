cmake_minimum_required(VERSION 3.29)
project(VirtualMemorySimulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(VirtualMemorySimulator
        PageTable/test/testPage.cpp
        PageTable/PageTable.cpp
        PageTable/PageTableEntry.cpp
        PageTable/PhysicalFrameManager.cpp
        PageTable/TLB.cpp
        PageTable/TLBEntry.h
        PageTable/TLB.h
        PageTable/PhysicalFrameManager.h
        main.cpp)