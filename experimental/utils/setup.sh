#!/bin/sh
#
# Author: Tyler Cecil
# Date: Thu Jul 16 16:17:57 MDT 2015
#
# Used to setupt the git subtrees. No longer needed. Only kept for posterity.
#

if false; then
    git remote add llvm git@github.com:llvm-mirror/llvm.git
    git remote add clang git@github.com:llvm-mirror/clang.git
    git remote add compiler-rt git@github.com:llvm-mirror/compiler-rt.git
    git remote add clang-tools-extra git@github.com:llvm-mirror/clang-tools-extra.git

    git subtree add -P llvm llvm master --squash -m 'Merged from llvm upstream.'
    git subtree add -P llvm/tools/clang clang master --squash -m 'Merged from clang upstream.'
    git subtree add -P llvm/projects/compiler-rt compiler-rt master --squash -m 'Merged from compiler-rt upstream.'
    git subtree add -P llvm/tools/clang/tools/clang-tools-extra clang-tools-extra master --squash -m 'Merged from clang-tools-extra upstream.'
else
    echo "YOU SHOULD NOT BE USING ME!!!"
fi
