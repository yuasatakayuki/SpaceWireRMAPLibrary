#Check CxxUtilities
ifndef CXXUTILITIES_PATH
CXXUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/CxxUtilities
endif

#Check XMLUtilities
ifndef XMLUTILITIES_PATH
XMLUTILITIES_PATH = $(SPACEWIRERMAPLIBRARY_PATH)/externalLibraries/XMLUtilities
endif

CXXFLAGS = -I$(SPACEWIRERMAPLIBRARY_PATH)/includes -I$(CXXUTILITIES_PATH)/includes -I$(XMLUTILITIES_PATH) -I/$(XERCESDIR)/include
LDFLAGS = -L/$(XERCESDIR)/lib -lxerces-c

TARGETS = \
tutorial_RMAPLayer \
tutorial_SpaceWireLayer \
tutorial_SpaceWireLayer_periodicTimecodeEmission \
tutorial_RMAPPacket_creationInterpretation \
main_RMAP_replyStatusToString \
main_RMAP_readWriteRMAPTargetNode \
main_RMAP_interpretAsAnRMAPPacket \
main_RMAP_calculateCRC \
main_RMAP_instructionToString \
main_SpaceWire_packetSink
#test_RMAPTarget_ReadWriteRegisters
#test_SpaceWireSSDTPModule \
#test_RMAPTargetNode_fromXMLFile \
#test_RMAPInitiator_RMAPEngine \
#test_periodic_packet_source \
#test_SpaceWireIFOverIPClient_timeout \
#test_RMAPTargetNode \
#test_RMAPPacket \
#test_RMAPMemoryObject_fromXMLFile

TARGETS_OBJECTS = $(addsuffix .o, $(basename $(TARGETS)))
TARGETS_SOURCES = $(addsuffix .cc, $(basename $(TARGETS)))

.PHONY : all

all : $(TARGETS)

$(TARGETS) : $(TARGETS_SOURCES)
	$(CXX) -g $(CXXFLAGS) -o $@ $@.cc $(LDFLAGS)
        
clean :
	rm -rf $(TARGETS) $(addsuffix .o, $(TARGETS))
