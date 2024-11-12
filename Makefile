SHELL := /bin/bash

.PHONY: test-page-table compile-simulator run-simulator

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

test-page-table: ## Compile and run test cases for page table
	g++ -std=c++11 test/testPage.cpp pageTable.cpp pageTableEntry.cpp physicalFrameManager.cpp -o page_table_test
	./page_table_test

compile-simulator: ## Compile the main program of simulator
	g++ -std=c++11 main.cpp PageTable/PageTable.cpp PageTable/PageTableEntry.cpp PageTable/PhysicalFrameManager.cpp PageTable/helperFiles/ClockAlgorithm.cpp TLB/TLB.cpp TLB/TLBEntry.cpp -I PageTable -I PageTable/helperFiles -I TLB -o vmsimulator

run-simulator: ## Generate instruction file and run simulator for testing
	@$(MAKE) compile-simulator
	python3 generator.py 100 0.3:8192 0.8:4096 > instructions.txt
	./vmsimulator 4096 32 $$((128 * 1024 * 1024)) 8 8192 4096 instructions.txt
