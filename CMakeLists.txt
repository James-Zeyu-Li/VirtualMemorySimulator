cmake_minimum_required(VERSION 3.29)
project(VirtualMemorySimulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(VirtualMemorySimulator
        PageTable/test/testPage.cpp
        PageTable/PageTable.cpp
        PageTable/PageTableEntry.cpp
        PageTable/PhysicalFrameManager.cpp
        TLB/TLB.cpp
        TLB/TLBEntry.cpp
        TLB/TLBEntry.h
        TLB/TLB.h
        PageTable/PhysicalFrameManager.h
        main.cpp)
