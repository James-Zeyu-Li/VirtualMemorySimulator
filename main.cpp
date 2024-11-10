#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <cstdint>
#include "PageTable/PageTable.h"
#include "PageTable/TLB.h"

using namespace std;

class Process {
private:
    uint32_t pid;
    PageTable pageTable;
    list<uint32_t> freePages;
    uint32_t totalPages;

public:
    Process(uint32_t pid, uint32_t numPages, uint32_t pageSize){};
    uint32_t getPid() const;
    PageTable* getPageTable();
    bool accessMemory(uint32_t virtualAddress, bool isWrite);
    uint32_t allocateMemory(uint32_t sizeInBytes);
    void freeMemory(uint32_t virtualAddress, uint32_t sizeInBytes);
    bool hasFreePage() const;
    uint32_t getFreePage();
    void addFreePage(uint32_t pageNumber);
};

class Simulator {
private:
    map<uint32_t, Process> processTable;
    list<uint32_t> freeFrames;
    TLB tlb;
    uint32_t currentProcessId;
    uint32_t totalFrames;
    uint32_t pageSize;
    Process getCurrentProcess(uint32_t pid);
    bool createProcess(uint32_t pid, uint32_t numPages);

public:
    Simulator(uint32_t numFrames, uint32_t pageSize, uint32_t tlbSize, const vector<uint32_t>& processMemSizes);
    void accessMemory(uint32_t virtualAddress);
    void switchProcess(uint32_t pid);
    void allocateMemory(uint32_t sizeInBytes);
    void freeMemory(uint32_t virtualAddress);
};

Process Simulator::getCurrentProcess(uint32_t pid) {
    return processTable.at(pid);
}

Simulator::Simulator(uint32_t numFrames, uint32_t pageSize, uint32_t tlbSize, const vector<uint32_t>& processMemSizes) {

    // TODO: Initialize free frames list

    // TODO: Check
    // Create processes
    for (uint32_t i = 0; i < processMemSizes.size(); i++) {
        uint32_t memSize = processMemSizes[i];
        uint32_t numPages = static_cast<uint32_t>(ceil(static_cast<double>(memSize) / pageSize));

        // TODO: Check if we have enough free frames for this process
        if (numPages > freeFrames.size()) {
            throw runtime_error("Not enough physical memory for process " + to_string(i));
        }
        Process process(i, numPages, pageSize);
        processTable[i] = process;
    }
    currentProcessId = -1;
}

void Simulator::accessMemory(uint32_t virtualAddress){
    // TODO: integrate translation code
}

void Simulator::switchProcess(uint32_t pid){
    currentProcessId = pid;
    // TODO: flush TLB
}

void Simulator::allocateMemory(uint32_t sizeInBytes){
    // TODO: check physical memory capacity
}

void Simulator::freeMemory(uint32_t virtualAddress){
    // TODO: validate virtual address
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << "<process_memory_sizes> <instruction_file>\n";
        return 1;
    }

    const uint32_t PAGE_SIZE = 4096;
    const uint32_t PHYSICAL_FRAMES = 1024;
    const uint32_t TLB_SIZE = 16;

    // Get process memory sizes from user
    vector<uint32_t> processMemSizes;
    for (int i = 1; i < argc - 1; i++) {
        processMemSizes.push_back(stoul(argv[i]));
    }

    try {
        // Simulator simulator(PHYSICAL_FRAMES, PAGE_SIZE, TLB_SIZE, processMemSizes);

        // Parse instruction file
        ifstream inFile(argv[argc - 1]);
        if (!inFile) {
            throw runtime_error("Cannot open instruction file");
        }

        string line;
        while (getline(inFile, line)) {
            istringstream iss(line);
            uint32_t pid;
            string command;

            iss >> pid >> command;

            cout << line << endl;
            if (command == "switch") {
                cout << "pid: " << pid << endl;
                // simulator.switchProcess(pid);
            }
            else if (command == "alloc") {
                string hexSize;
                iss >> hexSize;
                uint32_t size = stoul(hexSize, nullptr, 16);
                cout << "size: " << size << endl;
                // simulator.allocateMemory(size);
            }
            else if (command.substr(0, 6) == "access") {
                string hexAddr;
                iss >> hexAddr;
                uint32_t addr = stoul(hexAddr, nullptr, 16);
                cout << "addr: " << addr << endl;
                // simulator.accessMemory(addr);
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
