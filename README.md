# VirtualMemorySimulator

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

### Initial page table warm-up

For now every time a process is created, we will pre-assign **8** physical frames for it so that its page table can establish the first few pages. The goal is to reduce the initialization cost of page faults as a process starts to access its memory.

A randomized algorithm might perform better since code/heap/stack are not distinguished currently, yet for simplicity we just pick the first **8** pages for warm-up.

## Benchmark

### 1. Advantages of Design

The primary advantage of a two-level page table over a single-level page table is memory savings. In a single-level page table, the page table must allocate memory for every possible page in the virtual address space, even if many of these pages are not used. This design works but leads to high memory consumption, especially in systems with sparse memory usage.

**Why Two-Level Page Tables Are More Memory Efficient:**

•	**Sparse Memory Usage:** In real-world applications, processes rarely use every page in their virtual address space. A two-level page table only allocates memory for entries when they are needed. This dynamic allocation greatly reduces the memory footprint.

•	**Reduced Memory Allocation**: With a two-level page table, memory is only allocated for used pages. Thus, if a process only needs a small portion of its address space, the two-level page table saves memory by avoiding unnecessary allocation.

### 2. Performance (Hit Rates)

The two-level page table structure contributes indirectly to performance, particularly through better management of TLB (Translation Lookaside Buffer) and page table entries.

**Key Observations:**

•	**High TLB Hit Rate:** TLB hit rates for both processes are consistently high, ranging from 91.95% to 97.95% across different runs.

High TLB hit rates indicate that the two-level page table, with its organized structure, allows efficient caching of recently used translations in the TLB.

A high TLB hit rate improves performance by reducing the need for costly memory accesses to the page table, as virtual-to-physical translations are often resolved within the TLB.

•	**Moderate to High Page Table Hit Rate:** The page table hit rates are also fairly good, ranging from 58.33% to 83.33%.

This suggests that even when a TLB miss occurs, the page is often found in the two-level page table without triggering a page fault. This is particularly beneficial in managing memory for larger address spaces.

The two-level design minimizes page faults by making page table entries more accessible, thus reducing the likelihood of disk accesses.

### 3. Memory Access Analysis

The statistics show that for each process, hundreds of memory accesses are handled effectively with high hit rates. This implies that the two-level page table helps efficiently manage and cache active pages, enhancing memory access speed.

**Why Two-Level Page Tables Contribute to Higher Hit Rates:**

•	The hierarchical structure of a two-level page table improves spatial locality by clustering active pages in smaller tables, making frequently accessed entries quicker to retrieve.
 
•	The multi-level design organizes pages hierarchically, ensuring that more pages can be mapped with fewer page table entries, which works well with the TLB’s caching mechanism.

### 4. Summary

Based on the memory efficiency and performance improvements observed, the two-level page table design offers several advantages over a single-level page table:

•	**Memory Savings:** The two-level design allocates memory only for pages that are actually used, which saves significant memory in sparse address spaces. This is particularly valuable in large address spaces, where memory savings can reach several megabytes.

•	**Higher TLB and Page Table Hit Rates:** The two-level page table structure supports high hit rates, both at the TLB level and within the page table itself. High hit rates reduce the number of slow memory accesses and page faults, leading to faster overall performance.

•	**Scalability:** The two-level page table is better suited for systems with large virtual address spaces, as it prevents the excessive memory usage seen in single-level tables.

**Benchmark Conclusion**

The two-level page table design provides clear advantages in terms of memory efficiency and hit rate performance. With real-world applications where address spaces are often sparsely populated, this design offers significant memory savings and ensures faster memory accesses due to its structured layout, making it an optimal choice for modern systems with large virtual memory requirements.
