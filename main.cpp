#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
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

    // Counters for tracking individual process statistics
    uint32_t tlbHits = 0;
    uint32_t tlbMisses = 0;
    uint32_t pageTableHits = 0;
    uint32_t pageTableMisses = 0;
    uint32_t memoryAccessAttempts = 0;

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

    // Functions to increment counters
    void incrementTLBHit() { tlbHits++; }
    void incrementTLBMiss() { tlbMisses++; }
    void incrementPageTableHit() { pageTableHits++; }
    void incrementPageTableMiss() { pageTableMisses++; }
    void incrementMemoryAccess() { memoryAccessAttempts++; }

    // Functions to calculate hit rates
    double getTLBHitRate() const;
    double getPageTableHitRate() const;

    // Display statistics for the process
    void displayStatistics() const;
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

double Process::getTLBHitRate() const {
    return memoryAccessAttempts > 0 ? static_cast<double>(tlbHits) / memoryAccessAttempts : 0.0;
}

double Process::getPageTableHitRate() const {
    return tlbMisses > 0 ? static_cast<double>(pageTableHits) / tlbMisses : 0.0;
}

void Process::displayStatistics() const {
    cout << "Process " << id << " Statistics:" << endl;
    // cout << "  Memory Access Attempts: " << memoryAccessAttempts << endl;
    cout << "  Memory Access Attempts: " << std::dec << memoryAccessAttempts << endl;
    cout << "  TLB Hit Rate: " << getTLBHitRate() * 100 << "%" << endl;
    cout << "  Page Table Hit Rate: " << getPageTableHitRate() * 100 << "%" << endl;
    cout << endl;
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
    uint32_t offsetBits;
    uint32_t getPhysicalMemory();
    Process& getCurrentProcess();
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
    const map<uint32_t, Process>& getProcessTable();
};

const map<uint32_t, Process>& Simulator::getProcessTable() {
    return processTable;
}

uint32_t Simulator::getPhysicalMemory(){
    return pageSize * physicalFrames;
}

Process& Simulator::getCurrentProcess() {
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
    // Get the current process
    Process& process = processTable.at(currentProcessId);
    process.incrementMemoryAccess();

    // Constants for address components based on page size
    const int pageOffsetBits = offsetBits;
    const uint32_t pageOffsetMask = pageSize - 1;

    // Calculate VPN (Virtual Page Number) and offset within the page
    uint32_t vpn = virtualAddress >> pageOffsetBits;
    uint32_t offset = virtualAddress & pageOffsetMask;

    // 1. Check the TLB first for the VPN
    int pfn = tlb.lookupTLB(vpn);
    if (pfn != -1) {
        // TLB hit - construct the physical address
        process.incrementTLBHit();
        cout << "TLB hit for VPN " << vpn << ", PFN: " << pfn << endl;
        return (pfn << pageOffsetBits) | offset;
    } else {
        // TLB miss - increment TLB miss counter for this process
        process.incrementTLBMiss();
    }

    // 2. TLB miss - check the page table
    PageTable* pageTable = process.getPageTable();
    pfn = pageTable->lookupPageTable(vpn);
    if (pfn != -1) {
        // Page table hit - update TLB and return physical address
        process.incrementPageTableHit();
        cout << "Page table hit for VPN " << vpn << ", PFN: " << pfn << endl;
        tlb.updateTLB(vpn, pfn, true, true, true); // Update TLB with permissions as needed
        return (pfn << pageOffsetBits) | offset;
    } else {
        // Page table miss - increment page table miss counter for this process
        process.incrementPageTableMiss();
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
                     uint32_t tlbSize, const vector<uint32_t>& processMemSizes) : processTable(), pfManager(PhysicalFrameManager(numFrames)), tlb(TLB(tlbSize)), currentProcessId(-1), physicalFrames(numFrames), pageSize(pageSize), tlbSize(tlbSize), offsetBits(int(log(pageSize)/log(2))) {

    // Create processes
    for (uint32_t i = 0; i < processMemSizes.size(); i++) {
        uint32_t memSize = processMemSizes[i];
        uint32_t numPages = static_cast<uint32_t>(ceil(static_cast<double>(memSize) / pageSize));

        // NOTE: Here we only check if physical memory is enough for every single process
        if (numPages > numFrames) {
            throw runtime_error("Not enough physical memory for process " + to_string(i));
        }
        list<uint32_t> frames;
        int preAllocatedFrames = 8; // TODO: Make it configurable
        for (uint32_t j = 0; j < preAllocatedFrames; j++) {
            frames.push_back(pfManager.allocateFrame());
        }
        Process process(i, addressBits, pageSize, numPages, frames);

        //manually pre-allocate some frames for process
        PageTable* pageTable = process.getPageTable();
        uint32_t vpn = 0;
        for (uint32_t k = 0; k < preAllocatedFrames; k++) {
            int frame = process.getAFrame();
            pageTable->updatePageTable(vpn, frame, true, false, true, true, true, 0);
            vpn++;
        }
        processTable.insert({i, std::move(process)});
    }
    cout << "Virtual memory simulator created with page size " << pageSize << ", physical memory " << getPhysicalMemory() << endl;
    cout << "==========" << endl;
}

void Simulator::accessMemory(uint32_t virtualAddress) {
    uint32_t physicalAddress = translateVirtualAddress(virtualAddress);
    if (physicalAddress != UINT32_MAX) {
        cout << "Translated Virtual Address " << std::hex << virtualAddress
                << " to Physical Address " << physicalAddress << std::dec << endl;
    } else {
        cerr << "Error: Translation failed for Virtual Address " << std::hex << virtualAddress << std::dec << endl;
    }
}

void Simulator::switchProcess(uint32_t pid){
    cout << "Switched current process to " << pid << endl;
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
    uint32_t vpn = virtualAddress >> offsetBits;
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

    // Get process memory sizes from user
    vector<uint32_t> processMemSizes;
    for (int i = 5; i < argc - 1; i++) {
        processMemSizes.push_back(stoul(argv[i]));
    }
    try {
        Simulator simulator(VA_LEN, PAGE_SIZE, PHYSICAL_FRAMES, TLB_SIZE, processMemSizes);

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

            cout << "Execute instruction: " << line << endl;
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
            cout << "----------" << endl;
        }

        // Display statistics for each process after simulation
        cout << "\n--- Process Statistics ---" << endl;
        for (const auto& [pid, process] : simulator.getProcessTable()) {
            process.displayStatistics();
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
