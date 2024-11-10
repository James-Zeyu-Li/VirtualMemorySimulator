import random
import sys
from typing import List, Tuple, Union
from io import TextIOWrapper
import numpy as np
import argparse
'''
Usage: python test_generator.py <num_instructions> <process list>
E.g.: python3 test_generator.py 10000 0.5:1024 0.6:102400 0.95:`expr 1024 \\* 1024 \\* 1024`
For each process, two parameters are needed to be specified: locality and max memory usage
Locality means the chance of the address of a heap access is adjacent to the previous access.
High locality program example: matrix multiplication application, sequential memory access
Low locality program example:  graph application, such as compilers and social media analysis apps, random memory access
For each time step, it could generate one of the following operations
1. switch to another process
2. fetch a instruction from the code segment
3. access stack memory
4. access heap memory (that's where the access pattern plays a role in)
5. allocate memory
6. free memory
For each access to code segment, it could be
1. fetch next instruction (sequential execution)
2. jump to a location near the current location (loop)
3. jump to another completely random address (function call)
For each stack access, it could be
1. access the same address as before (accessing local variables)
2. access another address near the previous one (accessing local variables)
3. perform a sequence of memory accesses and increase the pointer (mimicking a function call)
4. jump back for a little bit (function return)
For each heap access, it could be
1. access an address based on locality
'''

MAX_ADDR = 0xffffffff
MIN_STACK_ADDR = MAX_ADDR - 4 * 1024 * 1024  # 4 MB max stack space
CODE_SIZE = 4 * 1024 * 1024  # 4 MB static data
MIN_PAGE_SIZE = 4096


class Process:
    def __init__(self, id, heap_locality, max_mem) -> None:
        self.HEAP_LOCALITY: float = heap_locality
        self.MAX_MEMORY: int = max_mem
        self.id: int = id
        self.code_p: int = 0
        self.ret_addrs: List[int] = []
        self.stack_p: int = MAX_ADDR
        self.stack_bases: List[int] = []
        self.heap_p: int = CODE_SIZE
        self.heap_size: int = 0

    def access_code(self) -> int:
        FETCH_NEXT = 0.8
        JUMP_BACK = 0.95
        JUMP_RAND = 1
        p = random.random()
        if p < FETCH_NEXT or p < 2:
            self.code_p = self.code_p + 1
        elif p < JUMP_BACK:
            MAX_JUMP_BACK = 8 * 128  # jump back at most 128 instructions
            jump_back = random.randint(1, min(self.code_p, MAX_JUMP_BACK))
            self.code_p -= jump_back
        elif p < JUMP_RAND:
            target = random.randint(0, CODE_SIZE)
            self.code_p = target

        return self.code_p

    def access_stack(self) -> List[int]:
        ACCESS_SAME = 0.475
        ACCESS_NEAR = 0.95
        FUNC_CALL = 0.975
        FUNC_RET = 1

        def clamp(addr) -> int:
            if addr > MAX_ADDR:
                return MAX_ADDR
            if addr < MIN_STACK_ADDR:
                return MIN_STACK_ADDR
            return addr

        p = random.random()
        ret: List[int] = []
        if p < ACCESS_SAME:
            ret.append(self.stack_p)
        elif p < ACCESS_NEAR:
            jump = random.randint(-8 * 16, 8 * 16)  # some intuitive value
            self.stack_p = clamp(self.stack_p + jump)
            ret.append(self.stack_p)
        elif p < FUNC_CALL:
            # **simulate** a function call
            num_args = random.randint(0, 5)
            for i in range(num_args):
                # pushing arguments to stack
                self.stack_p = clamp(self.stack_p - 1)
                ret.append(self.stack_p)
            # push ret address
            self.stack_p = clamp(self.stack_p - 1)
            ret.append(self.stack_p)
            # update PC and stack info
            self.ret_addrs.append(self.code_p)
            target = random.randint(0, CODE_SIZE)
            self.code_p = target
            self.stack_bases.append(self.stack_p)
            self.stack_p = clamp(self.stack_p - 8 * 32)
        elif p < FUNC_RET:
            if len(self.ret_addrs) != 0:
                self.code_p = self.ret_addrs.pop()
                self.stack_p = self.stack_bases.pop()
                ret.append(clamp(self.stack_p))
            else:
                # no call has been made, return.
                pass

        return ret

    def access_heap(self) -> int:
        def clamp(addr) -> int:
            if addr >= CODE_SIZE + self.heap_size:
                return CODE_SIZE + self.heap_size - 1
            if addr < CODE_SIZE:
                return CODE_SIZE
            return addr

        if self.heap_size == 0:
            return -1

        p = random.random()
        if p < self.HEAP_LOCALITY:
            stride = random.randint(-1, 1)
            self.heap_p = clamp(self.heap_p + stride)
        else:
            p = random.random()
            if p < 0.5:
                self.heap_p = clamp(
                    random.randint(CODE_SIZE, CODE_SIZE + self.heap_size))
            else:
                # In this case, we randomly choose an addr in the largest
                # possible memory space. If the target addr is not valid,
                # we return -1 to allocate new memory.
                # In this way, we can increase the possibility of memory allocation.
                self.heap_p = clamp(
                    random.randint(CODE_SIZE, CODE_SIZE + self.MAX_MEMORY))
        return self.heap_p

    def allocate_mem(self) -> int:
        '''
        Decide how much memory to allocate base on zipf distrib and update self.heap_p.
        return: page size
        '''
        if self.heap_size == self.MAX_MEMORY:
            return 0
        size_to_allocate = min(np.random.zipf(2) * MIN_PAGE_SIZE, self.MAX_MEMORY - self.heap_size)
        self.heap_size += size_to_allocate
        return size_to_allocate

    def free_mem(self) -> int:
        '''
        Free the last mem page in the self.page list and update self.heap_p if necessary.
        '''
        if self.heap_size == 0:
            return 0
        size_to_free = min(self.heap_size, np.random.zipf(2) * MIN_PAGE_SIZE)
        self.heap_size -= size_to_free
        if self.heap_p > self.heap_size + CODE_SIZE:
            self.heap_p = random.randint(CODE_SIZE, CODE_SIZE + self.heap_size)
        return size_to_free


def parse_process_params(config: str) -> Tuple[float, int]:
    try:
        A, B = config.split(':')
        return float(A), int(B)
    except ValueError:
        raise argparse.ArgumentTypeError(
            "Processes must be defined in the format 'locality:max_memory' where locality is a float and max_memory is a int."
        )

def write_access_mem(f: TextIOWrapper, id: int, addr: Union[int, List[int]], type: str) -> None:
    if isinstance(addr, List):
        addrs = addr
        for addr in addrs:
            f.write(f'{id}\taccess_{type}\t{hex(addr)}\n')
    else:
        f.write(f'{id}\taccess_{type}\t{hex(addr)}\n')

def write_alloc_mem(f: TextIOWrapper, id: int, size: int) -> None:
    f.write(f'{id}\talloc\t\t{hex(size)}\n')

def write_free_mem(f: TextIOWrapper, id: int, addr: int) -> None:
    f.write(f'{id}\tfree\t\t{hex(addr)}\n')

def write_switch_proc(f: TextIOWrapper, id: int) -> None:
    f.write(f'{id}\tswitch\t\n')

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('num_tests', type = int, help='Num of tests generated')
    parser.add_argument('process_configs', nargs='+', type=parse_process_params)

    args = parser.parse_args()
    num_tests = args.num_tests
    process_configs = args.process_configs
    processes = []
    for i, (locality, max_mem) in enumerate(process_configs):
        processes.append(Process(i, locality, max_mem))

    FETCH = 0.3
    STACK = 0.6
    HEAP = 0.9
    ALLOC = 0.98
    FREE = 0.99
    SWITCH = 1
    with sys.stdout as f:
        current_process: Process = random.choice(processes)
        write_switch_proc(f, current_process.id)
        for i in range(num_tests):
            p = random.random()
            if p < FETCH:
                addr = current_process.access_code()
                write_access_mem(f, current_process.id, addr, 'code')
            elif p < STACK:
                addr = current_process.access_stack()
                write_access_mem(f, current_process.id, addr, 'stak')
            elif p < HEAP:
                addr = current_process.access_heap()
                if addr != -1:
                    write_access_mem(f, current_process.id, addr, 'heap')
                else:
                    size = current_process.allocate_mem()
                    if size != 0:
                        write_alloc_mem(f, current_process.id, size)
            elif p < ALLOC:
                size = current_process.allocate_mem()
                if size != 0:
                    write_alloc_mem(f, current_process.id, size)
            elif p < FREE:
                addr = current_process.free_mem()
                if addr != 0:
                    write_free_mem(f, current_process.id, addr)
            elif p < SWITCH:
                proc: Process = random.choice(processes)
                while proc.id == current_process.id and len(processes) != 1:
                    proc = random.choice(processes)
                write_switch_proc(f, proc.id)
                current_process = proc



if __name__ == '__main__':
    main()