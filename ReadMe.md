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

Code Style
----------
I am using the Google Style Guide, simply because it has a nice
document others can follow:
https://google-styleguide.googlecode.com/svn/trunk/cppguide.html
