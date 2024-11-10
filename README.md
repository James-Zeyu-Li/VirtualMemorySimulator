# VirtualMemorySimulator


This project implements a two-level page table in C++ with a clock replacement algorithm for page eviction according to reference bit management.

Page table test

`g++ -std=c++11 test/testPage.cpp pageTable.cpp pageTableEntry.cpp physicalFrameManager.cpp -o page_table_test`


### Key Features

#### 1.	Two-Level Page Table Structure:
- The page table is organized in two levels using nested unordered_maps. 
- Each Virtual Page Number (VPN) is mapped to a PageTableEntry, 
- PageTable Entry includes `frame number, validity, dirty flag, access permissions, and a reference counter`.

#### 2.	Clock Page Replacement Algorithm:
- 	The page table uses a clock replacement algorithm to manage page faults when memory is full according to the reference counter.

- 	Global Reference Bit Management:
	- On each lookupPageTable call, the reference bit of the accessed page is set to 3, while all other pages have their reference bit decremented -1 (if above 0). 

- 	Page Fault Handling and Frame Allocation:
    - When a page fault occurs, the handlePageFault function attempts to allocate a new frame. 
    - If no frames are available, the clock algorithm is triggered to free up memory by replacing an existing page.
    - Check if any page has a reference bit of 0, if not decrement the reference bit for all entries and check again. 


#### *Previous implementation*
```
- Current implemented version is a boolean manipulation reference version, which provides a easier and faster swapping time.
    -  The boolean will be set to true when created and when looked up. 
    - Not going to be set to false until there is a pageFault and no free physical frame / physical memory full
        - The clock hand iterate through all Frames (go through 2 rounds for the first page eviction), if all reference bit are True, which will be , it will turn all reference to false
        - This will make the first look up go through the page table twice but the next evection will be finding the first false among all pages.
        - (Using time as an reference to change the reference bit is another good choice which is not implemented.)
- This approach reduces unnecessary resets and enables efficient page replacement by ensuring only necessary changes to the reference bit, improving swap performance.
```


#### Future Work
- When context switch instead of clear everything, save it to physical memory which can be located through PTBR which get the page table back for the process
    - maybe only save only 1 or 2 process's page table 

