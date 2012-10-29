/*
 * SpaceWirePacket.hh
 *
 *  Created on: Apr 21, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIREPACKET_HH_
#define SPACEWIREPACKET_HH_

#include <vector>

class SpaceWirePacket {
public:
	static const uint8_t DefaultLogicalAddress = 0xFE;

public:
	static const uint8_t DefaultProtocolID = 0x00;

protected:
	std::vector<uint8_t> wholePacket;

protected:
	uint8_t protocolID;
	std::vector<uint8_t> targetSpaceWireAddress;
	uint8_t targetLogicalAddress;

public:
	SpaceWirePacket() {
		this->targetLogicalAddress = SpaceWirePacket::DefaultLogicalAddress;
		this->protocolID = DefaultProtocolID;
	}

public:
	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

	std::vector<uint8_t> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

	uint8_t getDestinationLogicalAddress() const {
		return targetLogicalAddress;
	}

	std::vector<uint8_t> getDestinationSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

	uint8_t getProtocolID() const {
		return protocolID;
	}

	void setProtocolID(uint8_t protocolID) {
		this->protocolID = protocolID;
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTargetSpaceWireAddress(std::vector<uint8_t> targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

public:
	std::vector<uint8_t>& getPacket() {
		return wholePacket;
	}

	std::vector<uint8_t>* getPacketBufferPointer() {
		return &wholePacket;
	}

};

#endif /* SPACEWIREPACKET_HH_ */
