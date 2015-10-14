ToyLang
=======
This is a compiler for a toy language which will be compiled into
HLIR. It will act as a playground to test our HLIR constructs.

Language Overview
-----------------
Current constructs that we wish to represent for targetting HLIR:
* Memory regions
* Tasks
* Task data dependencies

Current Language Features:
* All variables are of type `double`.
* No mutable variables.
* No statements.
* Able to declare functions and tasks.
* If/Then/Else expressions.
* For expressions.
* Single returns from functions and tasks.

To be implemented:
* Be able to declare regions.
* Be able to declare partitions.
* Be able to declare structs / arrays.
* Map operation.

A typical program will read as follows:
```
task smallOp(x) = x+10;
task bigOp(x,y,z) =
    smallOp( smallOp(x) + smallOp(x+y) + smallOp(y) + smallOp(z+x) )
```

Note that these features are not yet finished. See
[Current State](Current State).

Building
--------
To build this project, be sure you cloned recursively; we use a
submodule for PEGTL, our parsing library. Then, simply running `make`
will build everything. You do not need to add rules to this Makefile,
as long as you use the file heirarchy.

Currently the Makefile uses `llvm-config-git`. This is a simlink I
have set up for the bleeding-edge version of llvm.

File Structure
--------------
* `src` --- Source files
* `include` --- Header files
* `lib` --- External libraries
* `build` --- Object files and the like
* `bin` --- Target executable

Current State
-------------
Currently the parser for this language does not actually generate
ASTs. I think going for a fancy parser may have been a bit of a
boondoggle. I'll either finish it or make a simple recurisve decent
parser when I have some free time. For now, I'm just hardcoding AST's,
which is not terrible --- what I care about is the codegen, anyway.

In the process of changing the frontend to support the above mentioned
features, as well as actually produce AST's now.

Code Style
----------
I am using the Google Style Guide, simply because it has a nice
document that others can follow:
https://google-styleguide.googlecode.com/svn/trunk/cppguide.html
