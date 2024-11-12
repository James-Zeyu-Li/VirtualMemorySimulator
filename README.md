# Virtual Memory Simulator

This project implements a virtual memory simulator with a two-level page table.

## Usage

### Compile & Test

```bash
# Test page table
make test-page-table

# Test simulator
make run-simulator
```

### Run simulator with custom params

```bash
# Say the compiled program is `vmsimulator`
# Page size is in bytes, e.g. 4096
# Virtual address length represents number of bits, e.g. 32
# Physical memory is in bytes
# TLB size represents maximum number of entries, e.g. 8
# Process memory sizes are in bytes, same as used for generator.py
# Instruction file should contain generated instructions by generator.py
./vmsimulator <page_size> <virtual_address_len> <physical_memory> <tlb_size> <process_memory_sizes> <instruction_file>
```

## Assumptions

1. Physical memory must be able to fulfill for any one of the processes, but not necessarily all of them.
2. New process will be allocated **8** physical frames for initializing the **first** few pages.
3. For now we don't differentiate memory access in terms of code, heap or stack.
4. Swapping mechanism is not yet implemented.

## Features

### Configurable parameters

All memory-related params are configurable:

1. Page size
2. Address space size
3. Physical memory
4. TLB size
5. Max memory for every process

### Two-Level page table

- The page table is organized in two levels using nested unordered_maps.
- Each Virtual Page Number (VPN) is mapped to a PageTableEntry,
- PageTable Entry includes `frame number, validity, dirty flag, access permissions, and a reference counter`.

### Clock page replacement algorithm

- The page table uses a clock replacement algorithm to manage page faults when memory is full according to the reference counter.
- Global Reference Bit Management:

  - On each lookupPageTable call, the reference bit of the accessed page is set to 3, while all other pages have their reference bit decremented -1 (if above 0).

- Page Fault Handling and Frame Allocation:
  - When a page fault occurs, the handlePageFault function attempts to allocate a new frame.
  - If no frames are available, the clock algorithm is triggered to free up memory by replacing an existing page.
  - Check if any page has a reference bit of 0, if not decrement the reference bit for all entries and check again.

### TLB and TLBEntry

- Each TLBEntry is organized by unordered_maps in TLB with a limitation of TLB size.
- Each Virtual Page Number (VPN) is mapped to a TLBEntry, which is also used as a parameter to look up, update, and delete a TLB Entry.
- When TLB is full, the simulator would evict the Least Recent used (LRU) Entry according to the access time.
- TLBEntry includes `Virtual Page Number(VPN), Page Frame Number (PFN), validity and access permissions`.
- If there is a TLB miss, the simulator will look up page table and then update the VPN and PFN in the TLB Entry.

### Initial page table warm-up

For now every time a process is created, we will pre-assign **8** physical frames for it so that its page table can establish the first few pages. The goal is to reduce the initialization cost of page faults as a process starts to access its memory.

A randomized algorithm might perform better since code/heap/stack are not distinguished currently, yet for simplicity we just pick the first **8** pages for warm-up.

## Benchmark

To test our simulator, we run the instructions generated with various input parameters:

```bash
# For simplicity, we set page size as 4KB, physical memory as 128MB, and TLB size as 8 entries.
# Also to get average results, we generate 1000 instructions for every test.

# Test case 1: process 1(0.2, 8192), process 2(0.8, 4096)

--- Process Statistics ---
Process 0 Statistics:
  Memory Access Attempts: 431
  TLB Hit Rate: 96.2877%
  Page Table Hit Rate: 50%

Two-Level Page Table Statistics:
  Total L1 Entries Allocated: 6
  Total L2 Entries Allocated: 16
  Total Allocated Entries: 22
  Total Memory Usage (Two-Level): 432 bytes
  For comparison, a single-level page table requires 1048576 entries and 12582912 bytes

Process 1 Statistics:
  Memory Access Attempts: 481
  TLB Hit Rate: 97.921%
  Page Table Hit Rate: 70%

Two-Level Page Table Statistics:
  Total L1 Entries Allocated: 6
  Total L2 Entries Allocated: 11
  Total Allocated Entries: 17
  Total Memory Usage (Two-Level): 372 bytes
  For comparison, a single-level page table requires 1048576 entries and 12582912 bytes

# Test case 2: process 1(0.2, 4MB), process 2(0.8, 10MB)

--- Process Statistics ---
Process 0 Statistics:
  Memory Access Attempts: 512
  TLB Hit Rate: 81.0547%
  Page Table Hit Rate: 4.12371%

Two-Level Page Table Statistics:
  Total L1 Entries Allocated: 22
  Total L2 Entries Allocated: 101
  Total Allocated Entries: 123
  Total Memory Usage (Two-Level): 2092 bytes
  For comparison, a single-level page table requires 1048576 entries and 12582912 bytes

Process 1 Statistics:
  Memory Access Attempts: 391
  TLB Hit Rate: 92.8389%
  Page Table Hit Rate: 35.7143%

Two-Level Page Table Statistics:
  Total L1 Entries Allocated: 10
  Total L2 Entries Allocated: 26
  Total Allocated Entries: 36
  Total Memory Usage (Two-Level): 712 bytes
  For comparison, a single-level page table requires 1048576 entries and 12582912 bytes

```

We analyzed the results in two aspects: space and time.

For space usage, we calculated the number of entries and bytes occupied by our 2-level page table and a single-level page table. Apparently, a multi-level page table saves much more memory than a flat one since only accessed memory will take up real pages. In addition, bigger memory usage of a process will consume more memory for its page table.

For time-related efficiency, we calculated hit rate for both TLB and page table. Depending on the locality, our TLB achieves very high cache hit rate and so does the page table. However, with processes that use more memory, the page table hit rate decreases, which results from a more coarse address space for its page access.

For future work, we plan to count memory access time by simulating different cycles for TLB hit, page table query and page fault, so that we'll have a more clear picture of how much our TLB and page table contribute to the paging memory speed-up.
