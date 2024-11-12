cmake_minimum_required(VERSION 3.29)
project(VirtualMemorySimulator)


set(CMAKE_CXX_STANDARD 14)


add_executable(VirtualMemorySimulator
        main.cpp
        PageTable/PageTable.cpp
        PageTable/PageTableEntry.cpp
        PageTable/PhysicalFrameManager.cpp
        PageTable/helperFiles/ClockAlgorithm.cpp
        TLB/TLB.cpp
)


target_include_directories(VirtualMemorySimulator PUBLIC
        PageTable
        PageTable/helperFiles
        TLB
        PageTable/test
)
