#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <cstdint>
#include "PageTable/PageTable.cpp"
#include "PageTable/TLB.h"
#include "PageTable/TLBEntry.h"

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
    uint32_t translateVirtualAddress(uint32_t virtualAddress);

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

// Translation function implementation
uint32_t Simulator::translateVirtualAddress(uint32_t virtualAddress) {
    // Constants for address components based on page size
    const int pageOffsetBits = 12; // 4KB page size
    const uint32_t pageSize = 1 << pageOffsetBits;
    const uint32_t pageOffsetMask = pageSize - 1;

    // Calculate VPN (Virtual Page Number) and offset within the page
    uint32_t vpn = virtualAddress >> pageOffsetBits;
    uint32_t offset = virtualAddress & pageOffsetMask;

    // 1. Check the TLB first for the VPN
    int pfn = tlb.lookupTLB(vpn);
    if (pfn != -1) {
        // TLB hit - construct the physical address
        std::cout << "TLB hit for VPN " << vpn << ", PFN: " << pfn << std::endl;
        return (pfn << pageOffsetBits) | offset;
    }

    // 2. TLB miss - check the page table
    Process& process = processTable[currentProcessId];
    PageTable* pageTable = process.getPageTable();
    pfn = pageTable->lookupPageTable(vpn);
    if (pfn != -1) {
        // Page table hit - update TLB and return physical address
        std::cout << "Page table hit for VPN " << vpn << ", PFN: " << pfn << std::endl;
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB with permissions as needed
        return (pfn << pageOffsetBits) | offset;
    }

    // 3. Page fault - Handle page fault
    std::cout << "Page fault for VPN " << vpn << std::endl;
    if (!pageTable->handlePageFault(vpn)) {
        std::cerr << "Error: Unable to handle page fault for VPN " << vpn << std::endl;
        return UINT32_MAX; // Return an error if page fault handling fails
    }

    // Retry after handling page fault
    pfn = pageTable->lookupPageTable(vpn);
    if (pfn != -1) {
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB after page fault resolution
        return (pfn << pageOffsetBits) | offset;
    }

    // If we still can't resolve the address, return an error
    std::cerr << "Error: Failed to translate virtual address " << virtualAddress << std::endl;
    return UINT32_MAX;
}

// Simulator constructor and other methods...

void Simulator::accessMemory(uint32_t virtualAddress) {
    uint32_t physicalAddress = translateVirtualAddress(virtualAddress);
    if (physicalAddress != UINT32_MAX) {
        std::cout << "Translated Virtual Address " << std::hex << virtualAddress
                  << " to Physical Address " << physicalAddress << std::endl;
    } else {
        std::cerr << "Error: Translation failed for Virtual Address " << std::hex << virtualAddress << std::endl;}
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
