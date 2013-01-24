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
