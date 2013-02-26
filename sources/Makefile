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
main_RMAP_calculateCRC \
main_RMAP_instructionToString \
main_RMAP_interpretAsAnRMAPPacket \
main_RMAP_readWriteRMAPTargetNode \
main_RMAP_replyStatusToString \
main_RouterConfigurator \
main_SpaceWire_packetSink \
tutorial_SimpleRMAPTarget \
tutorial_RMAPTarget \
tutorial_RMAPLayer \
tutorial_RMAPPacket_creationInterpretation \
tutorial_SpaceWireLayer \
tutorial_SpaceWireLayer_periodicTimecodeEmission 


TARGETS_OBJECTS = $(addsuffix .o, $(basename $(TARGETS)))
TARGETS_SOURCES = $(addsuffix .cc, $(basename $(TARGETS)))

.PHONY : all

all : $(TARGETS)

$(TARGETS) : $(TARGETS_SOURCES)
	$(CXX) -g $(CXXFLAGS) -o $@ $@.cc $(LDFLAGS)
        
clean :
	rm -rf $(TARGETS) $(addsuffix .o, $(TARGETS))
