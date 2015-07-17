HLIR --- LLVM Transform passes
==============================

Purpose
-------
In the pursuit of a higher level intermediate representation (HLIR)
of programs during the compilation (especially in HPC and parallel
aplications), we have begun experimenting with the
[LLVM compiler framework](www.llvm.org). Specifically, this file aims
at translated a standard set of metadata annotations on LLVM IR to
cononical LLIR. It is our hope that this metadata can one day be the
HLIR needed for parallel and high level optimizations and
verification.


Conent
------
LLVM allows transformations of their IR via
[passes](http://llvm.org/docs/WritingAnLLVMPass.html). This file
contains a pass called `HLIRLower` (an incomplete class), who's intent
it to lower the HLIR into LLIR. The bulk of the work takes place in
`runOnModule`, which is responsible for the actual representation
transformation.

This work, in its current state, is a prototype for methods that may
be used in the future. More than anything it is allowing us to explore
the LLVM framework, and asses the possibilities for our work. It is
also a work in progress, and is currently not function as it is in the
process of architectural changes.
