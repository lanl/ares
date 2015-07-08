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
CPPFLAGS   = -Wall -Wextra `llvm-config-git --cxxflags`
CPPFLAGS  += $(FLAG)
LIB       := `llvm-config-git --ldflags --libs`
INC       := -I include -I lib/PEGTL -I lib/PEGTL/pegtl
TESTFLAGS := --gtest_color=yes

# Directories and Files
SRCDIR   := src
BUILDDIR := build
TESTDIR  := test
TARGET   := bin/toy
TEST     := $(TARGET)_test

# Auto-find source files and track them
CPPSRCEXT:= cc
MAINSRC  := $(SRCDIR)/driver.$(CPPSRCEXT)
MAINOBJ  := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(MAINSRC:.$(CPPSRCEXT)=.o))
SOURCES  := $(shell find $(SRCDIR) -type f -name *.$(CPPSRCEXT))
SOURCES  := $(filter-out $(MAINSRC), $(SOURCES))
OBJECTS  := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(CPPSRCEXT)=.o))
TESTSRC  := $(shell find $(TESTDIR) -type f -name *.$(SRCEXT))
TESTOBJ  := $(patsubst $(TESTDIR)/%,$(BUILDDIR)/$(TESTDIR)/%,$(TESTSRC:.$(SRCEXT)=.o))
DEP      := $(OBJECTS:%.o=%.d)

# Google Test Framework variable
GTEST_DIR     = lib/googletest
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_   = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)
CPPFLAGS     += -isystem $(GTEST_DIR)/include -DGTEST_USE_OWN_TR1_TUPLE=1
GTALL   = $(BUILDDIR)/test/gtest-all.o
GTMAINO = $(BUILDDIR)/test/gtest_main.o
GTMAINA = $(BUILDDIR)/test/gtest_main.a
GTA     = $(BUILDDIR)/test/gtest.a

.SECONDEXPANSION:
.PHONY: clean test initld

all: $(TARGET)

$(TARGET): $(OBJECTS) $(MAINOBJ)
	@echo " Linking..."
	@mkdir -p $$(dirname $@)
	$(CPP) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(CPPSRCEXT)
	@echo " Building $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) $(INC) -c -o $@ -MD -MP -MF ${@:.o=.d} $<

$(MAINOBJ): $(MAINSRC)
	@echo " Building $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) $(INC) -c -o $@ -MD -MP -MF ${@:.o=.d} $<

$(TEST): $(TESTOBJ) $(OBJECTS) $(GTMAINA)
	@echo " Linking test..."
	@mkdir -p $$(dirname $@)
	$(CPP) -lpthread $(LIB) $^ -o $@

$(BUILDDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.$(CPPSRCEXT) $(GTEST_HEADERS)
	@echo " Building test $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) $(INC) -c -o $@ -MD -MP -MF ${@:.o=.d} $<

clean:
	@echo " Cleaning...";
	$(RM) -r $(BUILDDIR) $(TARGET)

test: $(TEST)
	@echo " Running tests..."
	./$(TEST) $(TESTFLAGS)

# initld: export LD_LIBRARY_PATH=/usr/local/lib:~/Documents/workspace/llvm-git/install/lib
#	@echo " Configuring load path for LLVM..."

# Google Tests rules
################################################################
$(GTALL): $(GTEST_SRCS_)
	@echo " Building gtest file $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(GTMAINO): $(GTEST_SRCS_)
	@echo " Building gtest file $@..."
	@mkdir -p $$(dirname $@)
	$(CPP) $(CPPFLAGS) -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest_main.cc -o $@

$(GTA): $(GTALL)
	@echo " Archiving gtest file $@..."
	@mkdir -p $$(dirname $@)
	$(AR) $(ARFLAGS) $@ $^

$(GTMAINA): $(GTALL) $(GTMAINO)
	@echo " Archiving gtest file $@..."
	@mkdir -p $$(dirname $@)
	$(AR) $(ARFLAGS) $@ $^

-include $(DEP)
