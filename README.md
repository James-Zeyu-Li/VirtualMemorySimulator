# VirtualMemorySimulator


This project implements a two-level page table in C++ with a clock replacement algorithm for page eviction according to reference bit management.

Key Features

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
