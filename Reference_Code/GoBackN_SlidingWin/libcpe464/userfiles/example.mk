# Makefile for CPE464 library 
# Can link with 32, 64 and Mac versions of the library 
#
# Use if you want - you do NOT need to use this - provided to help only
#
# Usage (assume its called Makefile): make  (makes all for a particular machine architecture)
# Usage: make rcopy32   (if making on 32 bit machine)
# Usage: make rcopyMac	(if making on Mac)
#
# note - if you are moving between machine architectures you must first delete your .o files
# Usage: make clean  (only deletes .o files)

CC = gcc
CFLAGS = -g -Wall -Werror

LIBS += -lstdc++
SRCS = $(shell ls *.cpp *.c 2> /dev/null | grep -v rcopy.c | grep -v server.c | grep -v rcopy.cpp | grep -v server.cpp)
OBJS = $(shell ls *.cpp *.c 2> /dev/null | grep -v rcopy.c | grep -v server.c | sed s/\.c[p]*$$/\.o/ )
LIBNAME = $(shell ls *cpe464_32*.a 2> /dev/null | tail -n 1)
FILE = 32

#use the unique library names for each architecture
ARCH = $(shell arch)
OS = $(shell uname -s)
ifeq ("$(OS)", "Linux")
	LIBS1 = -lstdc++
	ifeq ("$(ARCH)", "x86_64")
		FILE = 64
		LIBNAME = $(shell ls *cpe464_$(FILE)*.a 2> /dev/null | tail -n 1)
	endif
endif

#handle macs
ifeq ("$(OS)", "Darwin")
	FILE = "Mac"
	LIBNAME = $(shell ls *cpe464_$(FILE)*.a 2> .dev.null | tail -n 1)
endif

ALL = check_lib rcopy$(FILE) server$(FILE)

all:  $(OBJS) $(ALL)

lib:
	make -f lib.mk

check_lib: 
	@if [ "$(LIBNAME)" = "" ]; then \
		echo " ";  \
		echo "Library missing: libcpe464_$(FILE).X.Y.a";  \
		echo "Need to install the CPE464 library - see polyLearn"; \
		echo ""; \
		exit -1; fi
	
	
echo:
	@echo "CFLAGS: " $(CFLAGS)  
	@echo "SRCS: $(SRCS)"
	@echo "Objects: $(OBJS)"
	@echo "LIBNAME: $(LIBNAME)"

.cpp.o:
	@echo "-------------------------------"
	@echo "*** Building $@"
	$(CC) -c $(CFLAGS)  $< -o $@ $(LIBS1)

.c.o:
	@echo "-------------------------------"
	@echo "*** Building $@"
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS1)

rcopy$(FILE): rcopy.c $(OBJS)
	@echo "-------------------------------"
	@echo "*** Linking $@ with library: $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ rcopy.c $(OBJS) $(LIBNAME) $(LIBS)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

server$(FILE): server.c $(OBJS)
	@echo "-------------------------------"
	@echo "*** Linking $@ with library: $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ server.c $(OBJS) $(LIBNAME) $(LIBS)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

# clean .o
clean: 
	@echo "-------------------------------"
	@echo "*** Cleaning Files..."
	@echo "Deleting *.o's only"
	rm -f *.o 
	@echo "-------------------------------"
	
# clean targets
clean_all: 
	@echo "-------------------------------"
	@echo "*** Cleaning Files..."
	@echo "Deleting *.o's and '$(FILE)' bit versions of rcopy and server"
	rm -f *.o $(ALL)
	@echo "-------------------------------"
