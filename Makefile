SHELL := /bin/bash

.PHONY: test-page-table compile-simulator run-simulator

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

test-page-table: ## Compile and run test cases for page table
	g++ -std=c++11 test/testPage.cpp pageTable.cpp pageTableEntry.cpp physicalFrameManager.cpp -o page_table_test
	./page_table_test

compile-simulator: ## Compile the main program of simulator
	g++ -std=c++11 main.cpp -o vmsimulator

run-simulator: ## Generate instruction file and run simulator for testing
	@$(MAKE) compile-simulator
	python generator.py 100 0.3:1024 0.8:4096 > instructions.txt
	vmsimulator 4096 32 16384 8 1024 4096 instructions.txt
