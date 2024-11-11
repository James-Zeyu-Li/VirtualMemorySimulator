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
