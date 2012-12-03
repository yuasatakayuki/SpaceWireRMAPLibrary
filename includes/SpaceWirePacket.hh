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

class SpaceWirePacket {
public:
	enum AccessMode {
		StructuredMode, ByteArrayMode
	};

private:
	AccessMode mode;

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
	std::vector<uint8_t> cargo;
	SpaceWireEOPMarker::EOPType eopType;

public:
	SpaceWirePacket() {
		this->targetLogicalAddress = SpaceWirePacket::DefaultLogicalAddress;
		this->protocolID = DefaultProtocolID;
		eopType = SpaceWireEOPMarker::EOP;
		mode = StructuredMode;
	}

public:
	inline bool isStructuredMode() {
		if (mode == StructuredMode) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline bool isByteArrayMode() {
		if (mode == ByteArrayMode) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setStructuredMode() {
		mode = StructuredMode;
	}

public:
	inline void setByteArrayMode() {
		mode = ByteArrayMode;
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
		setStructuredMode();
		this->protocolID = protocolID;
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		setStructuredMode();
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTargetSpaceWireAddress(std::vector<uint8_t> targetSpaceWireAddress) {
		setStructuredMode();
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

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

public:
	SpaceWireEOPMarker::EOPType getEOPType() {
		return eopType;
	}

public:
	void setEOPType(SpaceWireEOPMarker::EOPType eopType) {
		this->eopType = eopType;
	}

public:
	std::vector<uint8_t>* getPacketBufferPointer() {
		if (isStructuredMode()) {
			constructPacket();
			return &wholePacket;
		} else {
			return &wholePacket;
		}
	}

};

#endif /* SPACEWIREPACKET_HH_ */
