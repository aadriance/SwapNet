# Makefile for CPE464 library

#Set to a, b since we don't know - should have been past in as variables 
# e.g. make -f build464lib.mk BUILD_MAJOR=2 BUILD_MINOR=16

BUILD_MAJOR=a
BUILD_MINOR=b
TEST=test

CC = g++
CFLAGS = -g -Wall

#create unique library names for each architecture
FILE = 32
ARCH = $(shell arch)
OS = $(shell uname -s)
ifeq ("$(OS)", "Linux")
	ifeq ("$(ARCH)", "x86_64")
		FILE = 64
	endif
endif

ifeq ("$(OS)", "Darwin")
	FILE = Mac
endif

PACKAGES = sendtoErr sendErr checksum
HDRS = $(shell cd networks && ls *.hpp *.h 2> /dev/null)
SRCS = $(shell find . -name "*.cpp" -o -name "*.c" 2> /dev/null)
OBJS = $(shell find . -name "*.cpp" -o -name "*.c" 2> /dev/null | sed s/\.c[p]*$$/\.o/ )
HFILE = cpe464

CPE464_VER = libcpe464_$(FILE).$(BUILD_MAJOR).$(BUILD_MINOR)
CPE464_LIB = $(CPE464_VER).a
CPE464_TAR = libcpe464.$(BUILD_MAJOR).$(BUILD_MINOR).tar

all: header $(OBJS) link combHeader clean
	@echo "-------------------------------"

test: test_setup echo all

test_setup:
	$(eval CPE464_VER = libcpe464_$(FILE).$(TEST))
	$(eval CPE464_LIB = $(CPE464_VER).a)
	$(eval CPE464_TAR = libcpe464.$(TEST).tar)

	
partial: header $(OBJS)
	@echo "-------------------------------"

echo:
	@echo $(OBJS)
	@echo "BUILD_MAJOR: " $(BUILD_MAJOR) "BUILD_MINOR: " $(BUILD_MINOR)
	@echo "CPE464_VER: $(CPE464_VER)"
	@echo "CPE464_LIB: $(CPE464_LIB) "
	@echo "CPE464_TAR: $(CPE464_TAR)"

version:
	@echo $(BUILD_MAJOR).$(BUILD_MINOR) 

header:
	@echo "-------------------------------"
	@echo "Building $(CPE464_LIB) Objects"

.cpp.o:
	@echo "-------------------------------"
	@echo "  C++ Compiling $@"
	$(V)$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)
.c.o:
	@echo "-------------------------------"
	@echo "  C Compiling $@"
	$(V)$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)

link:
	@echo "-------------------------------"
	@echo "Loading objects into $(CPE464_LIB) library"
	$(V)ar -rcv ../$(CPE464_LIB) $(OBJS)

combHeader:
	@echo "-------------------------------"
	@echo "Creating Master, Unified Header"
	@cd networks && cat $(HDRS) > ../../$(HFILE).h

# clean targets for Solaris and Linux
clean: 
	-@find $(CURDIR) -name "*.o" | xargs rm -f

clean-full: clean
	-@rm -f ../*libcpe464*.a
