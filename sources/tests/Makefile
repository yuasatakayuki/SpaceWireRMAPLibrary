#Check CxxUtilities
ifndef CXXUTILITIES_PATH
CXXUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/CxxUtilities
endif

#Check XMLUtilities
ifndef XMLUTILITIES_PATH
XMLUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/XMLUtilities
endif

CXXFLAGS = -I$(SPACEWIRERMAPLIBRARY_PATH)/includes -I$(CXXUTILITIES_PATH)/includes -I$(XMLUTILITIES_PATH)/include -I/$(XERCESDIR)/include
LDFLAGS = -L/$(XERCESDIR)/lib -lxerces-c

TARGETS = \
test_RMAPEngine_transactionIDLeak \
test_SpaceWireR_sendReceive

TARGETS_OBJECTS = $(addsuffix .o, $(basename $(TARGETS)))
TARGETS_SOURCES = $(addsuffix .cc, $(basename $(TARGETS)))

.PHONY : all

all : $(TARGETS)

$(TARGETS) : $(TARGETS_SOURCES)
	$(CXX) -O0 -g $(CXXFLAGS) -o $@ $@.cc $(LDFLAGS)
        
clean :
	rm -rf $(TARGETS) $(addsuffix .o, $(TARGETS))
