#!/bin/sh
#
# Author: Tyler Cecil
# Date: Thu Jul 16 16:17:57 MDT 2015
#
# Updates all llvm code from upstream repo, keeping the subtree structure/history.
#

git subtree pull -P llvm git@github.com:llvm-mirror/llvm.git \
    master --squash -m 'Merged from llvm upstream.' && \
    git subtree pull -P llvm/tools/clang git@github.com:llvm-mirror/clang.git \
    master --squash -m 'Merged from clang upstream.' && \
    git subtree pull -P llvm/projects/compiler-rt git@github.com:llvm-mirror/compiler-rt.git \
    master --squash -m 'Merged from compiler-rt upstream.' && \
    git subtree pull -P llvm/tools/clang/tools/clang-tools-extra git@github.com:llvm-mirror/clang-tools-extra.git \
    master --squash -m 'Merged from clang-tools-extra upstream.'
