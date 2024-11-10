#include <cassert>
#include <iostream>
#include "../pageTable.cpp" // 在实际项目中，应使用头文件，如 "pageTable.h"

using namespace std;

void testPageFaultHandling()
{
    cout << "\n===== Testing Page Fault Handling =====" << endl;

    // 创建一个 PageTable 实例，物理帧数设置为 3
    PageTable pt(3);

    // 1. 测试初始状态下的查找
    cout << "Test Case 1: Initial lookup should result in page fault..." << endl;
    int frameNumber = pt.lookupPageTable(1);
    assert(frameNumber == -1);
    cout << "Initial lookup for VPN 1 returned -1, as expected (page fault)." << endl;

    // 2. 测试处理页面缺页中断
    cout << "Test Case 2: Handling page fault for VPN 1..." << endl;
    bool handled = pt.handlePageFault(1);
    assert(handled);
    cout << "Page fault for VPN 1 handled successfully." << endl;

    // 3. 再次查找 VPN 1，确认页面已加载
    cout << "Test Case 3: Re-check VPN 1 after handling page fault..." << endl;
    frameNumber = pt.lookupPageTable(1);
    assert(frameNumber != -1);
    cout << "VPN 1 is now mapped to PFN " << frameNumber << "." << endl;

    // 4. 加载更多页面以填满物理内存
    cout << "Test Case 4: Loading VPN 2 and VPN 3 to fill memory..." << endl;
    pt.handlePageFault(2);
    pt.handlePageFault(3);
    assert(pt.lookupPageTable(2) != -1);
    assert(pt.lookupPageTable(3) != -1);
    cout << "VPN 2 and VPN 3 are successfully loaded into memory." << endl;

    // 5. 访问 VPN 1 和 VPN 2 以设置引用位
    cout << "Test Case 5: Access VPN 1 and VPN 2 to set reference bits..." << endl;
    pt.lookupPageTable(1);
    pt.lookupPageTable(2);

    // 6. 触发页面替换
    cout << "Test Case 6: Loading VPN 4 should trigger a page replacement..." << endl;
    handled = pt.handlePageFault(4);
    assert(handled);
    assert(pt.lookupPageTable(4) != -1);
    cout << "VPN 4 is successfully loaded, triggering a page replacement." << endl;

    // 检查 VPN 1、VPN 2 和 VPN 3 是否在内存中
    cout << "Checking memory state after page replacement..." << endl;
    if (pt.lookupPageTable(1) == -1)
        cout << "VPN 1 has been replaced as expected." << endl;
    if (pt.lookupPageTable(2) == -1)
        cout << "VPN 2 has been replaced as expected." << endl;
    if (pt.lookupPageTable(3) == -1)
        cout << "VPN 3 has been replaced as expected." << endl;

    // 7. 加载超出范围的 VPN 以触发无效输入测试
    cout << "Test Case 7: Attempting to handle page fault for an out-of-range VPN (-1)..." << endl;
    handled = pt.handlePageFault(-1);
    assert(!handled);
    cout << "Page fault handling for VPN -1 failed as expected due to invalid VPN." << endl;

    // 8. 尝试加载超过地址空间大小的 VPN
    cout << "Test Case 8: Attempting to handle page fault for VPN beyond address space limit..." << endl;
    handled = pt.handlePageFault(1 << 22); // 超出32位地址空间的VPN
    assert(!handled);
    cout << "Page fault handling for out-of-range VPN failed as expected." << endl;

    cout << "===== Page Fault Handling Tests Completed =====\n"
         << endl;
}

void testEdgeCasesAndResets()
{
    cout << "\n===== Testing Edge Cases and Resets =====" << endl;

    // 初始化页面表并加载多个页面
    PageTable pt(2); // 设置为只有 2 个物理帧

    cout << "Test Case 9: Loading initial pages (VPN 1 and VPN 2)..." << endl;
    pt.handlePageFault(1);
    pt.handlePageFault(2);
    assert(pt.lookupPageTable(1) != -1);
    assert(pt.lookupPageTable(2) != -1);
    cout << "VPN 1 and VPN 2 are successfully loaded." << endl;

    // 触发页面替换
    cout << "Test Case 10: Loading VPN 3 to trigger page replacement in a full memory scenario..." << endl;
    pt.handlePageFault(3);
    int lookupResult = pt.lookupPageTable(3);
    assert(lookupResult != -1);
    cout << "VPN 3 successfully loaded with replacement, mapped to PFN " << lookupResult << "." << endl;

    // 测试重置功能
    cout << "Test Case 11: Resetting page table and verifying clear state..." << endl;
    pt.resetPageTable();
    for (int vpn = 1; vpn <= 3; ++vpn)
    {
        lookupResult = pt.lookupPageTable(vpn);
        assert(lookupResult == -1);
        cout << "After reset, VPN " << vpn << " is not in memory, as expected." << endl;
    }

    cout << "===== Edge Cases and Reset Tests Completed =====\n"
         << endl;
}

int main()
{
    cout << "Starting Page Table Unit Tests..." << endl;

    // 运行各个测试函数
    testPageFaultHandling();
    testEdgeCasesAndResets();

    cout << "All tests passed successfully!" << endl;
    return 0;
}
