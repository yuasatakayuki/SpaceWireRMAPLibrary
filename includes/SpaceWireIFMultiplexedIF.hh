/* 
============================================================================
SpaceWire/RMAP Library is provided under the MIT License.
============================================================================

Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

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
/*
 * SpaceWireIFMultiplexedIF.hh
 *
 *  Created on: Nov 1, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIFMULTIPLEXEDIF_HH_
#define SPACEWIREIFMULTIPLEXEDIF_HH_

#include "SpaceWireIF.hh"
#include "SpaceWireIFMultiplexerSuperClass.hh"
#include "CxxUtilities/CommonHeader.hh"

class SpaceWireIFMultiplexedIF : public SpaceWireIF {
private:
	SpaceWireIF* parent;
	CxxUtilities::Condition receiveTimeoutCondition;
	CxxUtilities::Mutex receiveMutex;

public:
	bool packetArrived;
	CxxUtilities::Mutex receiveRequestMutex;

public:
	SpaceWireIFMultiplexedIF(SpaceWireIF* parentMultiplexer){
		this->parent=parentMultiplexer;
	}

public:
	void send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
		parent->send(data,length,eopType);
	}

public:
	void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) {
		receiveMutex.lock();
		this->packetArrived=false;
		((SpaceWireIFMultiplexerSuperClass*)parent)->receive(buffer,this);
		receiveTimeoutCondition(timeoutDurationInMicroSec);
		if(packetArrived=false){
			receiveRequestMutex.lock();
			parent->
			receiveRequestMutex.unlock();
			throw SpaceWireIFException(SpaceWireIFException::Timeout);
		}
		receiveMutex.unlock();
	}

public:
	void setTimeoutDuration(double microsecond) throw (SpaceWireIFException){
		this->timeoutDurationInMicroSec=microsecond;
	}

public:
	CxxUtilities::Condition* getReceiveTimeoutCondition(){
		return &receiveTimeoutCondition;
	}

};

#endif /* SPACEWIREIFMULTIPLEXEDIF_HH_ */
