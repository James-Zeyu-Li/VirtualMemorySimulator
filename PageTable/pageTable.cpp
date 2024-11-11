#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_set>
#include "helperFiles/ClockAlgorithm.h"
#include "PageTableEntry.h"
#include "PhysicalFrameManager.h"
#include "PageTable.h"

using namespace std;

// Helper functions to extract level-1 and level-2 indices from a VPN
uint32_t PageTable::getL1Index(uint32_t VPN)
{
    return VPN >> l2Bits; // Shift right by l2Bits
}

uint32_t PageTable::getL2Index(uint32_t VPN)
{
    return VPN & ((1 << l2Bits) - 1); // Get the last l2Bits
}

// Helper function to check and create a second-level map if necessary
unordered_map<uint32_t, PageTableEntry> &PageTable::checkL2(uint32_t l1Index)
{
    if (pageTable.find(l1Index) == pageTable.end()) // If the first level map does not exist
    {
        pageTable[l1Index] = unordered_map<uint32_t, PageTableEntry>(); // Create a new second-level map
    }
    return pageTable[l1Index]; // Return the second-level map
}

// Check if a VPN is within a valid range
bool PageTable::isValidRange(uint32_t VPN)
{
    return VPN < addressSpaceSize / pageSize;
}

// Default constructor with 256 frames
PageTable::PageTable() : pfManager(256), clockAlgo() {}

// Constructor with custom frame count
PageTable::PageTable(uint32_t totalFrames) : pfManager(totalFrames), clockAlgo() {}

// Lookup the page table for a given VPN and return the frame number or -1 if not found
int32_t PageTable::lookupPageTable(uint32_t VPN)
{
    if (!isValidRange(VPN))
    {
        cerr << "Invalid VPN: " << VPN << " Out of range" << endl;
        return -1;
    }

    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    auto it1 = pageTable.find(l1Index);
    if (it1 != pageTable.end())
    {
        auto it2 = it1->second.find(l2Index);
        if (it2 != it1->second.end() && it2->second.valid)
        {
            // Increment reference count if less than 3
            if (it2->second.reference < 3)
            {
                it2->second.referenceInc();
                cout << "After incrementing reference, VPN " << VPN
                     << " has reference " << static_cast<int>(it2->second.reference) << endl;
            }

            // Update active pages via ClockAlgorithm
            clockAlgo.addPage(VPN);

            return it2->second.frameNumber;
        }
    }

    return -1; // Page fault
}

// Update the page table with the given VPN and PFN
void PageTable::updatePageTable(uint32_t VPN, uint32_t frameNumber, bool valid, bool dirty, bool read, bool write, bool execute, uint8_t reference)
{
    if (!isValidRange(VPN))
    {
        cerr << "Invalid VPN: " << VPN << " Out of range" << endl;
        return;
    }

    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    // Get the second-level map
    auto &entry = checkL2(l1Index)[l2Index];

    // Update the page table entry
    entry.frameNumber = frameNumber;
    entry.valid = valid;
    entry.dirty = dirty;
    entry.read = read;
    entry.write = write;
    entry.execute = execute;
    entry.reference = reference;

    // If the page is valid, update the active pages via ClockAlgorithm
    if (valid)
    {
        clockAlgo.addPage(VPN);
    }
    else
    {
        // If the page is invalid, remove it from the active pages
        clockAlgo.removePage(VPN);
    }
}

// handle page fault with ClockAlgorithm, page replacement
bool PageTable::replacePageUsingClockAlgo(uint32_t VPN)
{
    uint32_t targetVPN;

    std::cout << "Attempting to replace page using ClockAlgorithm for VPN: " << VPN << std::endl;

    // 使用 ClockAlgorithm 选择要替换的页面
    while (clockAlgo.selectPageToReplace(targetVPN, *this))
    {
        std::cout << "Selected target VPN for replacement: " << targetVPN << std::endl;

        PageTableEntry *targetEntry = getPageTableEntry(targetVPN);

        // 检查目标页面是否有效
        if (targetEntry && targetEntry->valid)
        {
            uint32_t oldFrame = targetEntry->frameNumber;
            std::cout << "Target page found with frame number: " << oldFrame << ", proceeding with replacement." << std::endl;

            // 如果页面为脏，则写回
            if (targetEntry->dirty)
            {
                std::cout << "Target page is dirty. Writing back to disk." << std::endl;
                writeBackToDisk(oldFrame);
            }

            // 从页表中移除目标页面（在无效化之前）
            int removedFrame = removeAddressForOneEntry(targetVPN);
            if (removedFrame == -1)
            {
                cerr << "Error: Failed to remove victim VPN: " << targetVPN << endl;
                return false;
            }

            // 现在无效化并重置目标页面
            targetEntry->valid = false;
            targetEntry->reset();

            // 将旧的帧分配给新的 VPN
            std::cout << "Allocating frame " << oldFrame << " to new VPN " << VPN << std::endl;
            updatePageTable(VPN, oldFrame, true, false, true, true, true, 0);

            return true; // 替换成功
        }
        else
        {
            // 记录并移除无效或不存在的页面
            cerr << "Warning: Invalid or non-existent page selected by ClockAlgorithm: " << targetVPN << endl;
            clockAlgo.removePage(targetVPN); // 确保从 ClockAlgorithm 中移除
        }
    }

    cerr << "Failed to replace page for VPN: " << VPN << endl;
    return false; // 替换失败
}

// Write the page back to disk
void PageTable::writeBackToDisk(uint32_t frameNumber)
{
    cout << "Writing frame " << frameNumber << " back to disk." << endl;
}

// Remove the address for one entry
int PageTable::removeAddressForOneEntry(uint32_t VPN)
{
    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    // 检查 VPN 是否在页表中
    auto it1 = pageTable.find(l1Index);
    if (it1 == pageTable.end())
    {
        std::cerr << "Error: L1 index " << l1Index << " not found in the page table for VPN: " << VPN << std::endl;
        return -1;
    }

    auto it2 = it1->second.find(l2Index);
    if (it2 == it1->second.end())
    {
        std::cerr << "Error: L2 index " << l2Index << " not found in the page table for VPN: " << VPN << std::endl;
        return -1;
    }

    PageTableEntry &entry = it2->second;
    int pfn = entry.frameNumber;
    std::cout << "Removing VPN: " << VPN << " with frame number: " << pfn << " from page table." << std::endl;

    // 从页表中删除 VPN
    it1->second.erase(it2);
    if (it1->second.empty())
    {
        pageTable.erase(it1);
    }

    // 同步 ClockAlgorithm
    std::cout << "Removing VPN " << VPN << " from ClockAlgorithm." << std::endl;
    clockAlgo.removePage(VPN);

    return pfn; // 返回已删除条目的 PFN
}

// Get the PageTableEntry for a given VPN
PageTableEntry *PageTable::getPageTableEntry(uint32_t VPN)
{
    if (!isValidRange(VPN))
    {
        return nullptr;
    }

    uint32_t l1Index = getL1Index(VPN);
    uint32_t l2Index = getL2Index(VPN);

    auto it1 = pageTable.find(l1Index);
    if (it1 == pageTable.end())
    {
        return nullptr;
    }

    auto it2 = it1->second.find(l2Index);
    if (it2 == it1->second.end())
    {
        return nullptr;
    }

    if (it2->second.valid)
    {
        return &(it2->second);
    }
    return nullptr;
}

// reset the page table
void PageTable::resetPageTable()
{
    pageTable.clear();
    clockAlgo.reset();
}

// For testing purposes only
void PageTable::printPageTable() const
{
    for (const auto &l1Entry : pageTable)
    {
        for (const auto &l2Entry : l1Entry.second)
        {
            const auto &entry = l2Entry.second;
            cout << "VPN: " << ((l1Entry.first << l2Bits) | l2Entry.first)
                 << " -> Frame Number: " << entry.frameNumber
                 << ", Valid: " << entry.valid
                 << ", Dirty: " << entry.dirty
                 << ", Reference: " << static_cast<int>(entry.reference)
                 << endl;
        }
    }
}
