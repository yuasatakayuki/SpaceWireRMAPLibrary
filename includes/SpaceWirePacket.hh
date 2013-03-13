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
 * SpaceWirePacket.hh
 *
 *  Created on: Apr 21, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIREPACKET_HH_
#define SPACEWIREPACKET_HH_

#include "SpaceWireEOPMarker.hh"
#include <vector>

/**
 */
class SpaceWirePacket {
public:
	static const uint8_t DefaultLogicalAddress = 0xFE;

public:
	static const uint8_t DefaultProtocolID = 0x00;

protected:
	std::vector<uint8_t> wholePacket;

protected:
	uint8_t protocolID;
//	std::vector<uint8_t> cargo;
	SpaceWireEOPMarker::EOPType eopType;

public:
	SpaceWirePacket() {
		this->protocolID = DefaultProtocolID;
		eopType = SpaceWireEOPMarker::EOP;
	}

public:
	virtual ~SpaceWirePacket() {
	}

public:
	void setProtocolID(uint8_t protocolID) {
		this->protocolID = protocolID;
	}
	uint8_t getProtocolID() const {
		return protocolID;
	}
//	uint8_t getDestinationLogicalAddress() const {
//		return targetLogicalAddress;
//	}
	//	std::vector<uint8_t> getDestinationSpaceWireAddress() const {
	//		return targetSpaceWireAddress;
	//	}
/*
public:
	void setCargo(std::vector<uint8_t>& cargo) {
		setStructuredMode();
		this->cargo = cargo;
	}

public:
	std::vector<uint8_t> getCargo() {
		return this->cargo;
	}

public:
	std::vector<uint8_t>* getCargoPointer() {
		return &cargo;
	}

private:
	void constructPacket() {
		wholePacket.clear();
		wholePacket = targetSpaceWireAddress;
		wholePacket.push_back(targetLogicalAddress);
		wholePacket.push_back(protocolID);
		for (size_t i = 0; i < cargo.size(); i++) {
			wholePacket.push_back(cargo[i]);
		}
	}

public:
	std::vector<uint8_t>& getPacket() {
		if (isStructuredMode()) {
			constructPacket();
			return wholePacket;
		} else {
			return wholePacket;
		}
	}

public:
	void setPacket(std::vector<uint8_t>& packet) {
		setByteArrayMode();
		this->wholePacket = packet;
	}
*/

public:
	SpaceWireEOPMarker::EOPType getEOPType() {
		return eopType;
	}

public:
	void setEOPType(SpaceWireEOPMarker::EOPType eopType) {
		this->eopType = eopType;
	}

	/*
public:
	virtual std::vector<uint8_t>* getPacketBufferPointer() {
		if (isStructuredMode()) {
			constructPacket();
			return &wholePacket;
		} else {
			return &wholePacket;
		}
	}
*/
};

#endif /* SPACEWIREPACKET_HH_ */
