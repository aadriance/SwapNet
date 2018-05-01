#
# CPE464 Library Download & Build Makefile
#
# To Use, copy this file into your development directory and type...
#
# make -f lib.mk
#
# The library (.a) and header (.h) will now be compiled for the system.
# 
# Note:
# An example Makefile is also included which will build using the library.
# This example Makefile has a target which will re-build the library.
#

BUILD_MAJOR=2
BUILD_MINOR=16
TEST=test

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

CPE464_VER = libcpe464_$(FILE).$(BUILD_MAJOR).$(BUILD_MINOR)
CPE464_LIB = $(CPE464_VER).a
CPE464_TAR = libcpe464.$(BUILD_MAJOR).$(BUILD_MINOR).tar


CPE464_PATH =http://users.csc.calpoly.edu/~csc-cpe464/cpe464
LIBPATH=libcpe464
  
all:  libcpe464

test: test_setup echo libcpe464

test_setup:
	$(eval CPE464_VER = libcpe464_$(FILE).$(TEST))
	$(eval CPE464_LIB = $(CPE464_VER).a)
	$(eval CPE464_TAR = libcpe464.$(TEST).tar)
	$(eval TEST_STRING = test)

echo:
	@echo "BUILD_MAJOR: " $(BUILD_MAJOR) "BUILD_MINOR: " $(BUILD_MINOR)
	@echo "CPE464_VER: $(CPE464_VER)"
	@echo "CPE464_LIB: $(CPE464_LIB) "
	@echo "CPE464_TAR: $(CPE464_TAR)"
	
	
version:
	@echo "*********************************************"
	@echo "Released Version: $(CPE464_TAR)" 

header:
	@echo "*********************************************"
	@echo "  CPE464 Program Library Build Script " 

libcpe464: $(CPE464_LIB)

$(CPE464_LIB): header get_464lib get_Makefile build_464lib header
	@echo "*********************************************"

get_464lib:
	@echo "*********************************************"
	@echo " Retrieving CPE 464 Source Files - Path: $(CPE464_PATH) File: $(CPE464_TAR)"
	@echo
	@curl -O "$(CPE464_PATH)/$(CPE464_TAR)"
	@tar -xf $(CPE464_TAR)
	@rm -f $(CPE464_TAR)
	-@cp -f  ./$(LIBPATH)/userfiles/* .
	
get_Makefile:
	@if [ -f Makefile ]; then \
		echo "*********************************************";  \
		echo ""; echo "You already have a file called Makefile."; \
		echo "Creating an example makefile called Makefile2 instead. "; \
		echo "(you can delete Makefile2 if you don't need it)";  \
		echo "Makefile2 shows how to link in the libcpe464 library."; echo "";  \
		mv -f example.mk Makefile2; fi
	@if [ ! -f Makefile ]; then  \
		echo "*********************************************";  \
		echo ""; echo "Creating an example Makefile for you."; \
		echo "You need to modify this Makefile to meet your needs."; \
		echo "This Makefile shows how to link in the libcpe464 library."; echo ""; \
		mv -f example.mk Makefile; fi
	
build_464lib:
	@echo "*********************************************"
	@echo " Building CPE 464 Library: $(CPE464_LIB)"
	@if [ "$(ls $(CPE464_LIB))" != "" ]; then echo "Removing Old Lib"; rm -f $(CPE464_LIB); fi
	@if [ "$(ls cpe464.h)" != "" ]; then echo "Removing Old Header"; rm -f cpe464.h; fi
	@make -C ./$(LIBPATH) -f build464Lib.mk $(TEST_STRING) BUILD_MAJOR=$(BUILD_MAJOR) BUILD_MINOR=$(BUILD_MINOR) --no-print-directory

clean_464lib_src:
	@echo "*********************************************"
	@echo " Removing CPE 464 Library Source... "
	@echo -n ">>> Please confirm removal of directory $(LIBPATH) [y/n]: "
	@read -n1 \
		&& echo \
		&& test "$$REPLY" == "y" -o "$$REPLY" == "Y" \
		&& rm -Rf $(LIBPATH)

clean_464lib:
	@echo "*********************************************"
	@echo " Removing CPE 464 Library... "
	-@rm -f libcpe464_$(FILE)*.a cpe464.h Makefile.example


	