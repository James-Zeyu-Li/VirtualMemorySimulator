#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <cstdint>
#include "PageTable/PageTable.h"
#include "TLB/TLB.h"
#include "PageTable/PhysicalFrameManager.h"

using namespace std;

class Process {
private:
    uint32_t id;
    uint32_t addressBits;
    uint32_t pageSize;
    PageTable* pageTable;
    list<uint32_t> availableFrames; // A list of physical frames to use
    uint32_t maxFrames; // Max number of frames for this process
    uint32_t allocatedFrames;  // Number of frames assigned to this process, should never exceed maxFrames

public:
    Process(uint32_t pid, uint32_t addressBits, uint32_t pageSize, uint32_t numPages, list<uint32_t> allocatedFrames);
    uint32_t getPid();
    uint32_t getMaxFrames();
    uint32_t getAllocationQuota();
    PageTable* getPageTable();
    void allocateMemory(list<uint32_t> allocatedFrames);
    void freeMemory(uint32_t frameNumber);
    uint32_t getAFrame();
    void returnAFrame(uint32_t frame);
};

Process::Process(uint32_t pid, uint32_t virtualAddressLen, uint32_t pageSize_, uint32_t numPages, list<uint32_t> frames): id(pid), addressBits(virtualAddressLen), pageSize(pageSize_), maxFrames(numPages), availableFrames(frames), allocatedFrames(frames.size()), pageTable(new PageTable(virtualAddressLen, pageSize_)) {}

uint32_t Process::getPid() {
    return id;
}
uint32_t Process::getMaxFrames() {
    return maxFrames;
}

uint32_t Process::getAllocationQuota() {
    return maxFrames - allocatedFrames;
}

PageTable* Process::getPageTable() {
    return pageTable;
}

void Process::allocateMemory(list<uint32_t> frames) {
    while (!frames.empty()) {
        availableFrames.push_back(frames.front());
        frames.pop_front();
    }
    allocatedFrames = availableFrames.size();
}

void Process::freeMemory(uint32_t frameNumber) {
    allocatedFrames--;
}

uint32_t Process::getAFrame() {
    if (availableFrames.size() < 1) {
        return -1;
    }
    uint32_t frame = availableFrames.front();
    availableFrames.pop_front();
    return frame;
}

void Process::returnAFrame(uint32_t frame) {
    availableFrames.push_back(frame);
}

class Simulator {
private:
    map<uint32_t, Process> processTable;
    PhysicalFrameManager pfManager;
    TLB tlb;
    uint32_t currentProcessId;
    uint32_t physicalFrames;
    uint32_t pageSize;
    uint32_t tlbSize;
    uint32_t getPhysicalMemory();
    Process getCurrentProcess();
    bool createProcess(uint32_t pid, uint32_t numPages);
    uint32_t translateVirtualAddress(uint32_t virtualAddress);
    uint32_t getPagesFromBytes(uint32_t size) const;

public:
    Simulator(uint32_t addressBits, uint32_t pageSize, uint32_t numFrames, uint32_t tlbSize, const vector<uint32_t>& processMemSizes);
    void accessMemory(uint32_t virtualAddress);
    void switchProcess(uint32_t pid);
    void allocateMemory(uint32_t sizeInBytes);
    void freeMemory(uint32_t virtualAddress);
    bool handlePageFault(uint32_t vpn);
};

uint32_t Simulator::getPhysicalMemory(){
    return pageSize * physicalFrames;
}

Process Simulator::getCurrentProcess() {
    return processTable.at(currentProcessId);
}

uint32_t Simulator::getPagesFromBytes(uint32_t size) const {
    return (size + pageSize - 1) / pageSize;
}

bool Simulator::handlePageFault(uint32_t vpn) {
    // Get the current process's page table
    Process process = processTable.at(currentProcessId);
    PageTable* pageTable = process.getPageTable();

    // Use PageTable's isValidRange function to check if the VPN is valid
    if (!pageTable->isValidRange(vpn)) {
        cerr << "Invalid VPN: " << vpn << ". Out of range." << endl;
        return false;
    }

    // Try to allocate a new frame for the page
    int newFrame = process.getAFrame();
    if (newFrame != -1) {
        // Free frame available, update page table with new mapping
        pageTable->updatePageTable(vpn, newFrame, true, false, true, true, true, 0);
        cout << "Page fault handled. Assigned new frame " << newFrame << " to VPN " << vpn << endl;
        return true;
    } else {
        // No free frames, attempt page replacement using the Clock Algorithm
        bool replaced = pageTable->replacePageUsingClockAlgo(vpn);

        if (replaced) {
            cout << "Page fault handled by page replacement for VPN " << vpn << endl;
            return true;
        } else {
            cerr << "Error: Failed to handle page fault for VPN " << vpn << " - page replacement failed." << endl;
            return false;
        }
    }
}


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
        cout << "TLB hit for VPN " << vpn << ", PFN: " << pfn << endl;
        return (pfn << pageOffsetBits) | offset;
    }

    // 2. TLB miss - check the page table
    Process process = processTable.at(currentProcessId);
    PageTable* pageTable = process.getPageTable();
    pfn = pageTable->lookupPageTable(vpn);
    if (pfn != -1) {
        // Page table hit - update TLB and return physical address
        cout << "Page table hit for VPN " << vpn << ", PFN: " << pfn << endl;
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB with permissions as needed
        return (pfn << pageOffsetBits) | offset;
    }

    // 3. Page fault - Handle page fault
    cout << "Page fault for VPN " << vpn << endl;
    if (!handlePageFault(vpn)) {
        cerr << "Error: Unable to handle page fault for VPN " << vpn << endl;
        return UINT32_MAX; // Return an error if page fault handling fails
    }

    // Retry after handling page fault
    pfn = pageTable->lookupPageTable(vpn);
    if (pfn != -1) {
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB after page fault resolution
        return (pfn << pageOffsetBits) | offset;
    }

    // If we still can't resolve the address, return an error
    cerr << "Error: Failed to translate virtual address " << virtualAddress << endl;
    return UINT32_MAX;
}

Simulator::Simulator(uint32_t addressBits, uint32_t pageSize, uint32_t numFrames,
                     uint32_t tlbSize, const vector<uint32_t>& processMemSizes) : tlb(tlbSize), pfManager(numFrames){

    currentProcessId = -1;
    physicalFrames = numFrames;
    pageSize = pageSize;
    tlbSize = tlbSize;
    tlb = TLB(tlbSize);
    pfManager = PhysicalFrameManager(numFrames);

    // Create processes
    for (uint32_t i = 0; i < processMemSizes.size(); i++) {
        uint32_t memSize = processMemSizes[i];
        uint32_t numPages = static_cast<uint32_t>(ceil(static_cast<double>(memSize) / pageSize));

        // NOTE: Here we only check if physical memory is enough for every single process
        if (numPages > numFrames) {
            throw runtime_error("Not enough physical memory for process " + to_string(i));
        }
        list<uint32_t> frames;
        int preAllocatedFrames = 8; // FIXME: Hard-coded 8 frames allocated for a new process
        for (uint32_t j = 0; j < preAllocatedFrames; j++) {
            frames.push_back(pfManager.allocateFrame());
        }
        Process process(i, addressBits, pageSize, numPages, frames);

        //manually pre-allocate some frames for process
        PageTable* pageTable = process.getPageTable();
        uint32_t vpn = 0;
        for (uint32_t frame : frames) {
            pageTable->updatePageTable(vpn, frames.front(), true, false, true, true, true, 0);
            frames.pop_front();
            vpn++;
        }

        processTable.at(i) = process;
    }
    cout << "Virtual memory simulator created with page size " << pageSize << ", physical memory " << getPhysicalMemory() << endl;
}

void Simulator::accessMemory(uint32_t virtualAddress) {
    uint32_t physicalAddress = translateVirtualAddress(virtualAddress);
    if (physicalAddress != UINT32_MAX) {
        cout << "Translated Virtual Address " << hex << virtualAddress
                  << " to Physical Address " << physicalAddress << endl;
    } else {
        cerr << "Error: Translation failed for Virtual Address " << hex << virtualAddress << endl;}
}

void Simulator::switchProcess(uint32_t pid){
    cout << "Switch from process " << currentProcessId << " to " << pid << endl;
    currentProcessId = pid;
    tlb.flush();
    // TODO: better if we can check TLB status
}

void Simulator::allocateMemory(uint32_t sizeInBytes){
    int requestedPages = getPagesFromBytes(sizeInBytes);
    Process process = getCurrentProcess();
    uint32_t quota = process.getAllocationQuota();
    if (requestedPages > quota) {
        cout << "Requested memory exceeds maximum memory for the process: " << process.getMaxFrames() << endl;
        return;
    }
    uint32_t frames = pfManager.getFreeFrames();
    if (requestedPages > frames) {
        cout << "Requested memory exceeds available physical memory: " << frames << " frames" << endl;
        return;
    }
    list<uint32_t> allocatedFrames;
    for (uint32_t i = 0; i < requestedPages; i++) {
        allocatedFrames.push_back(pfManager.allocateFrame());
    }
    process.allocateMemory(allocatedFrames);
    cout << "Allocated " << requestedPages << " pages for process " << process.getPid() << endl;
}

void Simulator::freeMemory(uint32_t virtualAddress){
    Process process = getCurrentProcess();
    uint32_t vpn = virtualAddress >> 12;
    if (!process.getPageTable()->isValidRange(vpn)) {
        cout << "Virtual address is out of range: " << virtualAddress << ", vpn: " << vpn << endl;
        return;
    }
    int pfn = process.getPageTable()->removeAddressForOneEntry(vpn);
    if (pfn == -1) {
        cout << "Virtual address for memory free is not found in page table: " << pfn << endl;
        return;
    }
    pfManager.freeAFrame(pfn);
    process.freeMemory(pfn);
    tlb.deleteTLB(vpn);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << "<page_size> <virtual_address_len> <physical_memory> <tlb_size> <process_memory_sizes> <instruction_file>" << endl;
        return 1;
    }

    const uint32_t PAGE_SIZE = stoul(argv[1]);
    const uint32_t VA_LEN = stoul(argv[2]);
    const uint32_t PHYSICAL_MEM = stoul(argv[3]);
    const uint32_t PHYSICAL_FRAMES = PHYSICAL_MEM / PAGE_SIZE;
    const uint32_t TLB_SIZE = stoul(argv[4]);
    // cout << PAGE_SIZE << VA_LEN << PHYSICAL_FRAMES << TLB_SIZE << processMemSizes[0] << endl;

    // Get process memory sizes from user
    vector<uint32_t> processMemSizes;
    for (int i = 5; i < argc - 1; i++) {
        processMemSizes.push_back(stoul(argv[i]));
    }
    try {
        Simulator simulator(PAGE_SIZE, VA_LEN, PHYSICAL_FRAMES, TLB_SIZE, processMemSizes);

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
                simulator.switchProcess(pid);
            }
            else if (command == "alloc") {
                string hexSize;
                iss >> hexSize;
                uint32_t size = stoul(hexSize, nullptr, 16);
                simulator.allocateMemory(size);
            }
            else if (command.substr(0, 6) == "access") {
                string hexAddr;
                iss >> hexAddr;
                uint32_t addr = stoul(hexAddr, nullptr, 16);
                simulator.accessMemory(addr);
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
