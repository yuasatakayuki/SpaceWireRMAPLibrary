/*
 * SpaceWireRPacket.hh
 *
 *  Created on: Apr 21, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERPACKET_HH_
#define SPACEWIRERPACKET_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWireR/SpaceWireRProtocol.hh"
#include "SpaceWirePacket.hh"
#include "SpaceWireUtilities.hh"

class SpaceWireRUtility {
public:
	static uint16_t calculateCRCForHeaderAndData(const std::vector<uint8_t>& header, const std::vector<uint8_t>& data) {
		uint16_t crc16 = 0xFFFF;
		//todo
		return crc16;
	}

public:
	static uint16_t calculateCRCForArray(uint8_t* buffer, size_t length) {
		uint16_t crc16 = 0xFFFF;
		//todo
		return crc16;
	}
};

class SpaceWireRPacketType {
public:
	static const uint8_t DataPacket = 0x00;
	static const uint8_t DataAckPacket = 0x01;
	static const uint8_t ControlPacketOpenCommand = 0x02;
	static const uint8_t ControlPacketCloseCommand = 0x03;
	static const uint8_t KeepAlivePacket = 0x04;
	static const uint8_t KeepAliveAckPacket = 0x05;
	static const uint8_t FlowControlPacket = 0x06;
	static const uint8_t ControlAckPacket = 0x07;
};

class SpaceWireRSequenceFlagType {
public:
	static const uint8_t FirstSegment = 0x01;
	static const uint8_t ContinuedSegment = 0x00;
	static const uint8_t LstSegment = 0x02;
	static const uint8_t CompleteSegment = 0x03;
};

class SpaceWireRSecondaryHeaderFlagType {
public:
	static const uint8_t SecondaryHeaderIsNotUsed = 0x00;
	static const uint8_t SecondaryHeaderIsUsed = 0x01;
};

class SpaceWireRPacketException: public CxxUtilities::Exception {
public:
	enum {
		InvalidPacket, //
		InvalidPayloadLength, //
		TooShortPacketSize, //
		InvalidHeaderFormat, //
		InvalidProtocolID, //
		InvalidPrefixLength, //
		PacketHasExtraBytesAfterTrailer
	};

public:
	SpaceWireRPacketException(uint32_t status) :
			CxxUtilities::Exception(status) {
	}

public:
	virtual ~SpaceWireRPacketException() {
	}

public:
	std::string toString() {
		std::string str;
		switch (status) {
		case InvalidPacket:
			str = "InvalidPacket";
			break;
		case InvalidPayloadLength:
			str = "InvalidPayloadLength";
			break;
		case TooShortPacketSize:
			str = "TooShortPacketSize";
			break;
		case InvalidHeaderFormat:
			str = "InvalidHeaderFormat";
			break;
		case InvalidProtocolID:
			str = "InvalidProtocolID";
			break;
		case InvalidPrefixLength:
			str = "InvalidPrefixLength";
			break;
		case PacketHasExtraBytesAfterTrailer:
			str = "PacketHasExtraBytesAfterTrailer";
			break;
		default:
			str = "UndefinedException";
			break;
		}
		return str;
	}
};

/**
 *
 * SpaceWire-R Packet Format
 *
 * [Header] [Payload] [Trailer]
 *
 * where
 *
 * [Header] =  [Destination SLA] (1 byte)
 *             [Protocol ID]     (1 byte)
 *             [Packet Control]  (1 byte)
 *             [Payload Length]  (2 bytes)
 *             [Channel Number]  (2 bytes)
 *             [Sequence Number] (1 byte)
 *             [Address Control] (1 byte)
 *             [Source Address]  (N+1 bytes)
 *
 * [Packet Control] =
 *             [Protocol Version]       (2 bits)
 *             [Secondary Header Flag]  (1 bit)
 *             [Sequence Flag]          (2 bits)
 *             [Packet Type]            (3 bits)
 *
 * [Address Control] =
 *             [Reserved Spare] (4 bits)
 *             [Prefix Length]  (4 bits)
 *
 * [Source Address] =
 *             [Prefix]     (N bytes)
 *             [Source SLA] (1 byte)
 *
 * [Payload] = [Secondary Header (optional)]
 *             [Application Data]
 *
 * [Trailer] = [CRC] (2 bytes)
 *
 *
 */
class SpaceWireRPacket: public SpaceWirePacket {

private:
	std::vector<uint8_t> header;

private:
	uint8_t packetControl;
	uint8_t payloadLength[2];
	uint8_t channelNumber[2];
	uint8_t sequenceNumber;
	uint8_t prefixLength;
	std::vector<uint8_t> prefix;
	uint8_t sourceSpaceWireLogicalAddress;

private:
	/// for Packet Control Field
	const static uint8_t protocolVersion = 0x01; //2bits
	uint8_t secondaryHeaderFlag; //1bit
	uint8_t sequenceFlags; //2bits
	uint8_t packetType; //3bits

private:
	std::vector<uint8_t> payload;
	uint16_t crc16;

public:
	const static size_t MaxPayloadLength = 65535;
	const static size_t MinimumHeaderLength = 10;

public:
	SpaceWireRPacket() :
			SpaceWirePacket() {
		protocolID = SpaceWireRProtocol::ProtocolID;
		packetType = SpaceWireRPacketType::DataPacket; //dummy
		unuseSecondaryHeader();
		prefixLength = 0;
		sourceSpaceWireLogicalAddress = SpaceWirePacket::DefaultLogicalAddress;
	}

public:
	std::vector<uint8_t>* getPacketBufferPointer() {
		setByteArrayMode();
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>();
		constructHeader();

		//Target SpaceWire Address
		size_t targetSpaceWireAddressSize = targetSpaceWireAddress.size();
		for (size_t i = 0; i < targetSpaceWireAddressSize; i++) {
			buffer->push_back(targetSpaceWireAddress[i]);
		}

		//Header
		size_t headerSize = header.size();
		for (size_t i = 0; i < headerSize; i++) {
			buffer->push_back(header[i]);
		}

		//Payload
		size_t payloadSize = payload.size();
		for (size_t i = 0; i < payloadSize; i++) {
			buffer->push_back(payload[i]);
		}

		//Calculate CRC
		crc16 = SpaceWireRUtility::calculateCRCForHeaderAndData(header, payload);

		//Trailer
		buffer->push_back(crc16 / 0x100);
		buffer->push_back(crc16 % 0x100);

		return buffer;
	}

public:
	size_t getPacket(uint8_t* buffer, size_t maxLength) {
		setByteArrayMode();
		constructHeader();

		//Check buffer length
		size_t targetSpaceWireAddressSize = targetSpaceWireAddress.size();
		size_t headerSize = header.size();
		size_t payloadSize = payload.size();
		const size_t crcSize = 2;
		if (targetSpaceWireAddressSize + headerSize + payloadSize + crcSize > maxLength) {
			return 0;
		}

		size_t index = 0;

		//Target SpaceWire Address
		for (size_t i = 0; i < targetSpaceWireAddressSize; i++) {
			buffer[index] = targetSpaceWireAddress[i];
			index++;
		}

		//Header
		for (size_t i = 0; i < headerSize; i++) {
			buffer[index] = header[i];
			index++;
		}

		//Payload
		for (size_t i = 0; i < payloadSize; i++) {
			buffer[index] = payload[i];
			index++;
		}

		//Calculate CRC
		crc16 = SpaceWireRUtility::calculateCRCForArray(buffer, index - targetSpaceWireAddressSize + 1);

		//Trailer
		buffer[index] = crc16 / 0x100;
		index++;
		buffer[index] = crc16 % 0x100;
		index++;

		return index;
	}

private:
	void constructHeader() {
		header.clear();
		//Destination SLA
		header.push_back(this->getDestinationLogicalAddress());

		//Protocol ID
		header.push_back(SpaceWireRProtocol::ProtocolID);

		//Packet Control
		this->packetControl = protocolVersion * 0x40 /* 0100_0000 */
		+ secondaryHeaderFlag * 0x20 /* 0010_0000 */
		+ sequenceFlags * 0x08 /* 0000_1000 */
		+ packetType;
		header.push_back(this->packetControl);

		//Payload Length
		header.push_back(payloadLength[0]);
		header.push_back(payloadLength[1]);

		//Channel Number
		header.push_back(channelNumber[0]);
		header.push_back(channelNumber[1]);

		//Sequence Number
		header.push_back(sequenceNumber);

		//Address Control
		header.push_back(0x0000 + targetSpaceWireAddress.size());

		//Source Address
		for (size_t i = 0; i < targetSpaceWireAddress.size(); i++) {
			header.push_back(targetSpaceWireAddress[i]);
		}
		header.push_back(sourceSpaceWireLogicalAddress);
	}

public:
	void interpretPacket(std::vector<uint8_t>* buffer) throw (SpaceWireRPacketException) {
		if (buffer->size() < MinimumHeaderLength) {
			throw SpaceWireRPacketException(SpaceWireRPacketException::TooShortPacketSize);
		}

		try {
			//SpaceWire Address
			size_t index = 0;
			try {
				std::vector<uint8_t> temporarySpaceWireAddress;
				while (buffer->at(index) < 0x20) {
					temporarySpaceWireAddress.push_back(buffer->at(index));
					index++;
				}
			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidHeaderFormat);
			}

			//Destination SLA
			this->targetLogicalAddress = buffer->at(index);
			index++;

			//Protocol ID
			if (buffer->at(index) != SpaceWireRProtocol::ProtocolID) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidProtocolID);
			}
			index++;

			//Packet Control
			this->packetControl = buffer->at(index);
			interpretPacketControl();
			index++;

			//Payload Length
			this->payloadLength[0] = buffer->at(index);
			this->payloadLength[1] = buffer->at(index + 1);
			index += 2;

			//Channel Number
			this->channelNumber[0] = buffer->at(index);
			this->channelNumber[1] = buffer->at(index + 1);
			index += 2;

			//Sequence Number
			this->sequenceNumber = buffer->at(index);
			index++;

			//Address Control
			uint8_t prefixLength = buffer->at(index);
			index++;

			//Prefix
			try {
				prefix.clear();
				for (size_t i = 0; i < prefixLength; i++) {
					prefix.push_back(buffer->at(index));
				}
			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPrefixLength);
			}

			//Source SLA
			this->sourceSpaceWireLogicalAddress = buffer->at(index);
			index++;

			//Payload
			try {
				size_t payloadLengthValue = payloadLength[0] * 0x100 + payloadLength[1];
				payload.clear();
				for (size_t i = 0; i < payloadLengthValue; i++) {
					payload.push_back(buffer->at(index));
				}
			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPayloadLength);
			}

			//Trailer
			this->crc16 = buffer->at(index) * 0x100 + buffer->at(index + 1);
			index += 2;

			//Size check
			if (index != buffer->size()) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::PacketHasExtraBytesAfterTrailer);
			}
		} catch (...) {
			throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPacket);
		}
	}

private:
	inline void interpretPacketControl() {
		this->secondaryHeaderFlag = (packetControl & 0x20) >> 5 /* 0010_0000 */;
		this->sequenceFlags = (packetControl & 0x18) >> 3 /* 0001_1000 */;
		this->packetType = (packetControl & 0x07) /* 0000_0111 */;
	}

public:
	uint16_t getChannelNumber() const {
		return channelNumber[0] * 0x100 + channelNumber[1];
	}

	uint16_t getCRC() const {
		return crc16;
	}

	std::vector<uint8_t> getHeader() const {
		return header;
	}

	uint8_t getPacketControl() const {
		return packetControl;
	}

	uint8_t getPacketType() const {
		return this->packetType;
	}

	std::vector<uint8_t> getPayload() const {
		return payload;
	}

	uint16_t getPayloadLength() const {
		return payloadLength[0] * 0x100 + payloadLength[1];
	}

	uint8_t getSequenceNumber() const {
		return sequenceNumber;
	}

	std::vector<uint8_t> getSourceAddressPrefix() const {
		return targetSpaceWireAddress;
	}

	uint8_t getSourceSpaceWireLogicalAddress() const {
		return sourceSpaceWireLogicalAddress;
	}

	void setChannelNumber(uint16_t channelNumber) {
		this->channelNumber[0] = channelNumber / 0x100;
		this->channelNumber[1] = channelNumber % 0x100;
	}

	void setCRC(uint16_t crc) {
		this->crc16 = crc;
	}

	void setHeader(std::vector<uint8_t> header) {
		this->header = header;
	}

	void setPacketControl(uint8_t packetControl) {
		this->packetControl = packetControl;
	}

	void setPacketType(uint8_t packetType) {
		this->packetType = packetType;
	}

	void setPayload(std::vector<uint8_t> payload) throw (SpaceWireRPacketException) {
		if (payload.size() > SpaceWireRPacket::MaxPayloadLength) {
			throw SpaceWireRPacketException::InvalidPayloadLength;
		}
		this->payload = payload;
		setPayloadLength(payload.size());
	}

	void setPayloadLength(uint16_t payloadLength) {
		this->payloadLength[0] = payloadLength / 0x100;
		this->payloadLength[1] = payloadLength % 0x100;
	}

	void setSequenceNumber(uint8_t sequenceNumber) {
		this->sequenceNumber = sequenceNumber;
	}

	void setSourceAddressPrefix(std::vector<uint8_t> sourceAddressPrefix) {
		this->targetSpaceWireAddress = sourceAddressPrefix;
	}

	void setSourceSpaceWireLogicalAddress(uint8_t sourceSpaceWireLogicalAddress) {
		this->sourceSpaceWireLogicalAddress = sourceSpaceWireLogicalAddress;
	}

	void setTrailer(uint16_t trailer) {
		setCRC(trailer);
	}

///Packet Control
	uint8_t getSecondaryHeaderFlag() const {
		return secondaryHeaderFlag;
	}

	void setSecondaryHeaderFlag(uint8_t secondaryHeaderFlag) {
		this->secondaryHeaderFlag = secondaryHeaderFlag;
	}

	uint8_t getSequenceFlags() const {
		return sequenceFlags;
	}

	void setSequenceFlags(uint8_t sequenceFlags) {
		this->sequenceFlags = sequenceFlags;
	}

public:
	//set packet types
	void setDataPacketFlag() {
		setPacketType(SpaceWireRPacketType::DataPacket);
	}

	void setDataAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::DataAckPacket);
	}

	void setControlPacketOpenCommandFlag() {
		setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);
	}

	void setControlPacketCloseCommandFlag() {
		setPacketType(SpaceWireRPacketType::ControlPacketCloseCommand);
	}

	void setKeepAlivePacketFlag() {
		setPacketType(SpaceWireRPacketType::KeepAlivePacket);
	}

	void setKeepAliveAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::KeepAliveAckPacket);
	}

	void setFlowControlPacketFlag() {
		setPacketType(SpaceWireRPacketType::FlowControlPacket);
	}

	void setControlAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::ControlAckPacket);
	}

public:
	void unuseSecondaryHeader() {
		this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed;
	}

	void useSecondaryHeader() {
		this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed;
	}

public:
	void setFirstSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::FirstSegment;
	}

	void setContinuedSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::ContinuedSegment;
	}

	void setLstSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::LstSegment;
	}

	void setCompleteSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::CompleteSegment;
	}

public:
	//prefix-related
	size_t getPrefixLength() const {
		return prefix.size();
	}

	std::vector<uint8_t> getPrefix() const {
		return prefix;
	}

	void setPrefix(std::vector<uint8_t>& prefix) {
		this->prefix = prefix;
	}

};

class SpaceWireRDataPacket: public SpaceWireRPacket {
public:
	SpaceWireRDataPacket() {
		this->setDataPacketFlag();
	}
};

class SpaceWireRDataAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRDataAckPacket() {
		this->setDataAckPacketFlag();
	}
};

class SpaceWireROpenCommandPacket: public SpaceWireRPacket {
public:
	SpaceWireROpenCommandPacket() {
		this->setControlPacketOpenCommandFlag();
	}
};

class SpaceWireRCloseCommandPacket: public SpaceWireRPacket {
public:
	SpaceWireRCloseCommandPacket() {
		this->setControlPacketCloseCommandFlag();
	}
};

class SpaceWireRKeepAlivePacket: public SpaceWireRPacket {
public:
	SpaceWireRKeepAlivePacket() {
		this->setKeepAlivePacketFlag();
	}
};

class SpaceWireRKeepAliveAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRKeepAliveAckPacket() {
		this->setKeepAliveAckPacketFlag();
	}
};

class SpaceWireRFlowControlPacket: public SpaceWireRPacket {
public:
	SpaceWireRFlowControlPacket() {
		this->setFlowControlPacketFlag();
	}
};

class SpaceWireRControlAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRControlAckPacket() {
		this->setControlAckPacketFlag();
	}
};

#endif /* SPACEWIRERPACKET_HH_ */
