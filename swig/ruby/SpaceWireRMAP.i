/* 
============================================================================
Ruby Binding of SpaceWire/RMAP Library is provided under the MIT License.
============================================================================

Copyright (c) 2013 Hirokazu Odaka

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

// SWIG Ruby interface to SpaceWireRMAPLibrary
// @author Hirokazu Odaka
// @date 2013-03-22

%module SpaceWireRMAP
%{

const int __TMP_RUBY_T_STRING = T_STRING;
#undef T_STRING

#include "CxxUtilities/Exception.hh"
#include "SpaceWireIF.hh"
#include "SpaceWireIFOverTCP.hh"
#include "RMAPMemoryObject.hh"
#include "RMAPTargetNode.hh"
#include "RMAPEngine.hh"
#include "RMAPInitiator.hh"
#include "RMAPPacket.hh"

#define T_STRING __TMP_RUBY_T_STRING

%}

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

%template(VectorUInt8) std::vector<uint8_t>;
%template(ByteArray) std::vector<uint8_t>;

//============================================
// Namespace CxxUtilities
//============================================
namespace CxxUtilities {

%nodefault;
	class Exception {
	public:
	  std::string toString();
	};

%default;

}

%exception {
	try {
		$action 
	} catch (CxxUtilities::Exception& e) {
		SWIG_exception(SWIG_RuntimeError, e.toString().c_str());
	}
}

%nodefault;


//============================================
// root namespace
//============================================
class SpaceWireIF {

};

%default;

class SpaceWireEOPMarker{};
%nodefault;

class SpaceWireIFOverTCP : public SpaceWireIF {
public:
	SpaceWireIFOverTCP(std::string hostname, uint32_t portnumber);
	SpaceWireIFOverTCP(uint32_t portnumber);
	
public:
	virtual ~SpaceWireIFOverTCP();
	virtual void open() throw (CxxUtilities::Exception);
	virtual void close() throw (CxxUtilities::Exception);
	%rename(send) sendVectorPointer;
	virtual void sendVectorPointer(std::vector<uint8_t>* data, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) throw (CxxUtilities::Exception);
	//void send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) throw (CxxUtilities::Exception);
	void receive(std::vector<uint8_t>* buffer) throw (CxxUtilities::Exception);
	void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (CxxUtilities::Exception);
};

class RMAPEngine {
public:
	explicit RMAPEngine(SpaceWireIF* spwif);
	~RMAPEngine();
	virtual int start();
	virtual void stop();
};


class RMAPInitiator{
public:
	RMAPInitiator(RMAPEngine* rmapEngine);
	~RMAPInitiator();
	
	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress);
	uint8_t getInitiatorLogicalAddress();
	
	void setRMAPTargetNodeDB(RMAPTargetNodeDB* targetNodeDB);
	RMAPTargetNodeDB* getRMAPTargetNodeDB();
	
	%rename(read) readConstructingNewVecotrBuffer;
	std::vector<uint8_t>*
		readConstructingNewVecotrBuffer(
			std::string targetNodeID,
			std::string memoryObjectID,
			double timeoutDuration=DefaultTimeoutDuration
		) throw (CxxUtilities::Exception);
	
	void write(
			std::string targetNodeID,
			uint32_t memoryAddress,
			std::vector<uint8_t>* data,
			double timeoutDuration =DefaultTimeoutDuration
		) throw (CxxUtilities::Exception);
};


%include "RMAPMemoryObject.hh"
%include "RMAPTargetNode.hh"
%include "RMAPPacket.hh"