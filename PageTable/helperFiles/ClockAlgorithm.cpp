#include "ClockAlgorithm.h"
#include "PageTable.h"
#include <algorithm>
#include <iostream>

ClockAlgorithm::ClockAlgorithm()
{
    clockHand = activePages.begin();
}

void ClockAlgorithm::addPage(uint32_t VPN)
{
    if (activeVPNs.find(VPN) == activeVPNs.end())
    {
        activePages.push_back(VPN);
        activeVPNs.insert(VPN);
        if (activePages.size() == 1)
        {
            clockHand = activePages.begin();
        }
    }
}

void ClockAlgorithm::removePage(uint32_t VPN)
{
    auto it = std::find(activePages.begin(), activePages.end(), VPN);
    if (it != activePages.end())
    {
        if (it == clockHand)
        {
            clockHand = activePages.erase(it);
            if (clockHand == activePages.end() && !activePages.empty())
            {
                clockHand = activePages.begin();
            }
        }
        else
        {
            activePages.erase(it);
        }
        activeVPNs.erase(VPN);
    }
}

bool ClockAlgorithm::selectPageToReplace(uint32_t &targetVPN, PageTable &pageTable)
{
    if (activePages.empty())
    {
        std::cerr << "Error: No active pages available for replacement." << std::endl;
        return false;
    }

    int maxScans = activePages.size();
    int scans = 0;

    while (true) // 循环直到找到可替换的页面
    {
        if (scans >= maxScans)
        {
            // 如果扫描了一圈没有找到任何引用为0的页面，则递减所有页面的引用计数
            for (uint32_t vpn : activePages)
            {
                PageTableEntry *entry = pageTable.getPageTableEntry(vpn);
                if (entry && entry->reference > 0)
                {
                    entry->reference--; // 递减引用计数
                }
            }
            scans = 0; // 重置扫描计数以重新扫描
        }

        // 继续扫描直到找到引用为0的页面
        if (clockHand == activePages.end())
        {
            clockHand = activePages.begin();
        }

        uint32_t currentVPN = *clockHand;
        PageTableEntry *entry = pageTable.getPageTableEntry(currentVPN);

        if (entry == nullptr)
        {
            std::cerr << "Error: PageTableEntry not found for VPN: " << currentVPN << std::endl;
            moveClockHandNext();
            scans++;
            continue;
        }

        if (entry->reference == 0)
        {
            // 找到可替换的页面
            targetVPN = currentVPN;
            moveClockHandNext();
            return true;
        }

        moveClockHandNext();
        scans++;
    }

    return false; // 此处实际上不会被执行
}

void ClockAlgorithm::reset()
{
    activePages.clear();
    activeVPNs.clear();
    clockHand = activePages.begin();
}

void ClockAlgorithm::moveClockHandNext()
{
    if (activePages.empty())
    {
        return;
    }
    ++clockHand;
    if (clockHand == activePages.end())
    {
        clockHand = activePages.begin();
    }
}
