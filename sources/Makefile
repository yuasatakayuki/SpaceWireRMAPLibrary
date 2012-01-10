SPACEWIRERMAPLIBRARY_PATH = /Users/yuasa/Documents/workspace/SpaceWireRMAPLibrary

CXXUTILITIES_PATH = /usr/local/CxxUtilities
XMLUTILITY_PATH = /Users/yuasa/ws/XMLUtilities
XERCESDIR = /Users/yuasa/work/install/xerces-c-3.1.1_src

CXXFLAGS = -I$(SPACEWIRERMAPLIBRARY_PATH)/includes -I$(CXXUTILITIES_PATH)/includes -I$(XMLUTILITY_PATH) -I/$(XERCESDIR)/include
LDFLAGS = -L/$(XERCESDIR)/lib -lxerces-c

TARGETS = \
main_RMAP_replyStatusToString
#main_RMAP_readWriteRMAPTargetNode \
#test_RMAPTarget_ReadWriteRegisters
#test_SpaceWireSSDTPModule \
#test_RMAPTargetNode_fromXMLFile \
#main_RMAP_interpretAsAnRMAPPacket \
#main_RMAP_calculateCRC \
#main_RMAP_instructionToString \
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
