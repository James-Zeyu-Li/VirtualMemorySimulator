#include <iostream>
// #include "../pageTable.cpp" // Adjust the include path as necessary
#include <cassert>
#include <stdexcept>
#include <string>
#include <stdio.h>

#include "../PageTable.h"
#include "../helperFiles/ClockAlgorithm.h"

using namespace std;
using namespace std;

int main()
{
    PageTable pageTable; // 默认构造函数，256个frame
    int frameNumber;

    // 测试 1: 在空的页表中查找
    cout << "测试 1: 在空的页表中查找" << endl;
    frameNumber = pageTable.lookupPageTable(1);
    assert(frameNumber == -1); // 预期返回 -1，因为页表为空
    cout << "通过：在空的页表中查找返回 -1。" << endl;

    // 测试 2: 添加有效页并查找
    cout << "测试 2: 添加有效页并查找" << endl;
    pageTable.updatePageTable(5, 10, true, false, true, true, true, 1);
    frameNumber = pageTable.lookupPageTable(5);
    assert(frameNumber == 10); // 预期返回正确的frame编号
    cout << "通过：查找返回了正确的frame编号。" << endl;

    // 测试 3: 查找无效VPN（超出范围）
    cout << "测试 3: 查找无效VPN（超出范围）" << endl;
    frameNumber = pageTable.lookupPageTable(PageTable::addressSpaceSize / PageTable::pageSize + 1);
    assert(frameNumber == -1); // 预期返回 -1，因为VPN超出范围
    cout << "通过：查找无效VPN返回 -1。" << endl;

    // 测试 4: 更新页面条目并检查引用计数递增
    cout << "测试 4: 更新页面条目并检查引用计数递增" << endl;
    pageTable.lookupPageTable(5); // 引用计数增至 2
    pageTable.lookupPageTable(5); // 引用计数增至 3（最大值）
    frameNumber = pageTable.lookupPageTable(5);
    assert(frameNumber == 10); // Frame应保持不变
    PageTableEntry *entry = pageTable.getPageTableEntry(5);
    assert(entry && entry->reference == 3); // 引用计数最大值为3
    cout << "通过：引用计数正确递增。" << endl;

    // 测试 5: 使页面无效并确认不可访问
    cout << "测试 5: 使页面无效并确认不可访问" << endl;
    pageTable.updatePageTable(5, 10, false, false, true, true, true, 3); // 使页面无效
    frameNumber = pageTable.lookupPageTable(5);
    assert(frameNumber == -1); // 预期返回 -1，因为页面已无效
    cout << "通过：无效页面查找返回 -1。" << endl;

    // 测试 6: 使用Clock算法进行页面替换
    cout << "测试 6: 使用Clock算法进行页面替换" << endl;
    for (int i = 0; i < 256; i++)
    {
        pageTable.updatePageTable(i, i, true, false, true, true, true, 1); // 添加256个页面
    }
    bool replaced = pageTable.replacePageUsingClockAlgo(300);
    assert(replaced); // 确认替换成功
    frameNumber = pageTable.lookupPageTable(300);
    assert(frameNumber != -1); // 新页面应可访问
    cout << "通过：Clock算法替换页面成功。" << endl;

    // 测试 7: 处理脏页替换和写回
    cout << "测试 7: 处理脏页替换和写回" << endl;
    pageTable.updatePageTable(200, 50, true, true, true, true, true, 1); // 设置页面为脏页
    replaced = pageTable.replacePageUsingClockAlgo(301);
    assert(replaced); // 确认替换成功
    frameNumber = pageTable.lookupPageTable(301);
    assert(frameNumber != -1); // 新页面应可访问
    cout << "通过：正确处理脏页替换。" << endl;

    // 测试 8: 边界情况 - 最大引用计数检查
    cout << "测试 8: 边界情况 - 最大引用计数检查" << endl;
    pageTable.updatePageTable(100, 40, true, false, true, true, true, 2);
    pageTable.lookupPageTable(100); // 引用计数增加至最大（3）
    pageTable.lookupPageTable(100); // 应保持在3
    entry = pageTable.getPageTableEntry(100);
    assert(entry && entry->reference == 3); // 引用计数不应超过3
    cout << "通过：最大引用计数保持在3。" << endl;

    // 测试 9: 移除页面并同步Clock算法
    cout << "测试 9: 移除页面并同步Clock算法" << endl;
    int removedFrame = pageTable.removeAddressForOneEntry(100);
    assert(removedFrame == 40); // 确认移除返回正确frame
    frameNumber = pageTable.lookupPageTable(100);
    assert(frameNumber == -1); // 移除后查找应失败
    cout << "通过：页面移除并同步Clock算法。" << endl;

    // 测试 10: 大规模插入和边界情况
    cout << "测试 10: 大规模插入和边界情况" << endl;
    for (uint32_t i = 0; i < 1000; i++)
    {
        pageTable.updatePageTable(i, i + 1, true, false, true, true, true, 1); // 插入多个页面
    }
    for (uint32_t i = 0; i < 1000; i++)
    {
        frameNumber = pageTable.lookupPageTable(i);
        assert(frameNumber == (i + 1)); // 确认每个页面映射到期望frame
    }
    cout << "通过：大规模插入和查找成功。" << endl;

    cout << "所有测试均已通过！" << endl;
    return 0;
}
