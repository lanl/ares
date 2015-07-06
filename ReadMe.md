ToyLang
=======
This is a compiler for a toy language which will be compiled into
HLIR. It will act as a playground to test our HLIR constructs.

It's syntax is currently C-like. More documentation on the language
itself will exist when the compiler exists.

Building
--------
To build this project, be sure you cloned recursively; we use a
submodule for PEGTL, our parsing library. Then, simply running `make`
will build everything. You do not need to add rules to this Makefile,
as long as you use the file heirarchy.

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

This branch should be able to codegen everything. Once I have enough
features, I'll begin targeting HLIR.

Code Style
----------
I am using the Google Style Guide, simply because it has a nice
document others can follow:
https://google-styleguide.googlecode.com/svn/trunk/cppguide.html
