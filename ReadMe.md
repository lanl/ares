ARES : Abstract Representation for the Extreme-Scale Stack
==========================================================

Project ARES represents a joint effort between [LANL](https://www.lanl.gov/) and
[ORNL](https://www.lanl.gov/) to introduce a common compiler representation and
tool-chain for HPC applications. At the project's core is the *High Level
Intermediate Representation*, or HLIR, for common compiler toolchains. HLIR is
built ontop of the LLVM IR, using metadata to represent high-level parallel
constructs.

To learn more about HLIR, see
[the HLIR passes](www.github.com/losalamos/ares/tree/master/llvm/lib/Transforms/HLIR).
for the current prototype of the HLIR system.
