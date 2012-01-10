###############################################################
#An example Makefile for SpaceWire RMAP Library.
###############################################################

#Note 1:
#To compile a user application with SpaceWire RMAP Library,
#set SPACEWIRERMAPLIBRARY_PATH and XERCESDIR in the shell 
#initialization file first.
#
#Execute below to check if these variables are correctly
#set in your shell.
#
# > ls $SPACEWIRERMAPLIBRARY_PATH
# > ls $XERCESDIR
#
#If no error is observed, the paths seem valid.

#Note 2:
#This Makefile assumes a user-application source code named
#UserApplication.cc. If other source files, include paths,
#and/or linker flags are necessary for compile, add them to
#CXXFLAGS and LDFLAGS.

###############################################################


#Set target (binary names)
#See also the rule part below.
TARGETS = \
UserApplication

#Check CxxUtilities
ifndef $(CXXUTILITIES_PATH)
CXXUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/CxxUtilities
endif

#Check XMLUtility
ifndef $(XMLUTILITY_PATH)
XMLUTILITY_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/XMLUtility
endif

#Set compiler/linker flags
CXXFLAGS = -I$(SPACEWIRERMAPLIBRARY_PATH)/includes -I$(CXXUTILITIES_PATH)/includes -I$(XMLUTILITY_PATH) -I/$(XERCESDIR)/include
LDFLAGS = -L/$(XERCESDIR)/lib -lxerces-c


TARGETS_OBJECTS = $(addsuffix .o, $(basename $(TARGETS)))
TARGETS_SOURCES = $(addsuffix .cc, $(basename $(TARGETS)))

###############################################################

.PHONY : all

all : $(TARGETS)

UserApplication : UserApplication.o
	$(CXX) -g $(CXXFLAGS) -o $@ $@.cc $(LDFLAGS)
        
clean :
	rm -rf $(TARGETS) $(addsuffix .o, $(TARGETS))
