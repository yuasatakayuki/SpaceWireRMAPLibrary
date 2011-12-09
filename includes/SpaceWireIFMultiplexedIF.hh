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
	void send(uint8_t* data, size_t length, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
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
