################################################################################
# Makefile for Project Ares
# Author: Izzy Cecil
# Date:   Fri Jul 17 10:58:25 MDT 2015
#
# Use:
# This Makefile only provides utilities for development of HLIR. Essentially it
# is about to run CMake with some default configuration, launch a ninja build
# or update the project with the upstream llvm code. USE WITH CAUTION. This
# is currently only in a state that benefits my (the authors) own convenience,
# and may not fit your work-flow. It may be developed further down the road.
################################################################################

LLVM_SRC_DIR     = llvm
LLVM_BUILD_DIR   = build
LLVM_INSTALL_DIR = install
CMAKE_FLAGS      = -DCMAKE_BUILD_TYPE="Release" -DLLVM_TARGETS_TO_BUILD="X86" -G Ninja -DCMAKE_INSTALL_PREFIX=../$(LLVM_INSTALL_DIR)

.PHONY: ninja cmake pull-upstream clean

ninja:
	@echo "Looking for build dir..."
	@((test -d $(LLVM_BUILD_DIR)) || make cmake)
	@echo "Running ninja build / install..."
	@(cd $(LLVM_BUILD_DIR); ninja && ninja install)

cmake:
	@echo -n "Making build and install directories... "
	@mkdir -p $(LLVM_BUILD_DIR)
	@mkdir -p $(LLVM_INSTALL_DIR)
	@echo "DONE!"
	@echo "Launching CMake with default settigns..."
	@(cd $(LLVM_BUILD_DIR); cmake ../$(LLVM_SRC_DIR) $(CMAKE_FLAGS))

pull-upstream:
	@echo "Pulling upstream changes into project..."
	@echo "(Will add merges to git history)"
	git subtree pull -P llvm git@github.com:llvm-mirror/llvm.git \
	master --squash -m 'Merged from llvm upstream.' && \
	git subtree pull -P llvm/tools/clang git@github.com:llvm-mirror/clang.git \
	master --squash -m 'Merged from clang upstream.' && \
	git subtree pull -P llvm/projects/compiler-rt git@github.com:llvm-mirror/compiler-rt.git \
	master --squash -m 'Merged from compiler-rt upstream.' && \
	git subtree pull -P llvm/tools/clang/tools/clang-tools-extra git@github.com:llvm-mirror/clang-tools-extra.git \
	master --squash -m 'Merged from clang-tools-extra upstream.'

clean:
	@echo "Cleaning away all build files..."
	@rm -r $(LLVM_BUILD_DIR) $(LLVM_INSTALL_DIR)
