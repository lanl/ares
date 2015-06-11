################################################################################
# Makefile
# Izzy Cecil, 2014
#
# `make all` will build the simulator.
# `make test` will build and run unit tests.
#
# All .c files in the ./src directory are automatically tracked and managed, so
# that if a new file is added, the build does not need to be changed. This file
# will also automatically track interfile dependencies.
#
# All .cc files in the ./test/unit directory are tracked in the same way, and
# will be linked together by the `test` rule, which makes and runs the unit
# test suit.
#
# FLAG env variable will be added to CPPFLAGS, if you need to
# pass compile options from make.
#
# Variables of note:
# LIB <- What library flags need to be given for linking.
# INC <- What directories need to be included for header files.
################################################################################

# Compilers
CPP = clang++ --std=c++11

# Compiler Flags
CPPFLAGS   = -Wall -Wextra
CPPFLAGS  += $(FLAG)
LIB       :=
INC       := -I include -I lib/PEGTL -I lib/PEGTL/pegtl

# Directories and Files
SRCDIR   := src
BUILDDIR := build
TESTDIR  := test
TARGET   := bin/toy
TEST     := $(TARGET)_test

# Auto-find source files and track them
CPPSRCEXT:= cc
SOURCES  := $(shell find $(SRCDIR) -type f -name *.$(CPPSRCEXT))
OBJECTS  := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(CPPSRCEXT)=.o))
DEP      := $(OBJECTS:%.o=%.d)

.SECONDEXPANSION:
.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@mkdir -p $$(dirname $@)
	$(CPP) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(CPPSRCEXT)
	@echo " Building $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) $(INC) -c -o $@ -MD -MP -MF ${@:.o=.d} $<

clean:
	@echo " Cleaning...";
	$(RM) -r $(BUILDDIR) $(TARGET)

-include $(DEP)
