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
#include "SpaceWireR/SpaceWireRUtilities.hh"

#undef debugSpaceWireRPacket

class SpaceWireRPacketType {
public:
	static const uint8_t DataPacket = 0x00;
	static const uint8_t DataAckPacket = 0x01;
	static const uint8_t ControlPacketOpenCommand = 0x02;
	static const uint8_t ControlPacketCloseCommand = 0x03;
	static const uint8_t HeartBeatPacket = 0x04;
	static const uint8_t HeartBeatAckPacket = 0x05;
	static const uint8_t FlowControlPacket = 0x06;
	static const uint8_t ControlAckPacket = 0x07;
};

class SpaceWireRSequenceFlagType {
public:
	static const uint8_t FirstSegment = 0x01;
	static const uint8_t ContinuedSegment = 0x00;
	static const uint8_t LastSegment = 0x02;
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
		InvalidCRC, //
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
		case InvalidCRC:
			str = "InvalidCRC";
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
 *--------------------------------------
 * ==Packet Type==
 * 0 Data Packet
 * 1 Data Ack Packet
 * 2 Control Packet (Open Command)
 * 3 Control Packet (Close Command)
 * 4 Keep Alive Packet
 * 5 Keep Alive Ack Packet
 * 6 Flow Control Packet
 * 7 Control Ack Packet
 *
 */
class SpaceWireRPacket: public SpaceWirePacket {
public:
	enum {
		DataPacketType = 0x00, //
		DataAckPacketType = 0x01, //
		ControlPacketOpenCommandType = 0x02, //
		ControlPacketCloseCommandType = 0x03, //
		HeartBeatPacketType = 0x04, //
		HeartBeatAckPacketType = 0x05, //
		FlowControlPacket = 0x06, //
		ControlAckPacket = 0x07 //
	};

private:
	std::vector<uint8_t> header;

private:
	uint8_t packetControl;
	uint8_t payloadLength[2];
	uint8_t channelNumber[2];
	uint8_t sequenceNumber;
	uint8_t prefixLength;

private:
	uint8_t destinationLogicalAddress;
	std::vector<uint8_t> destinationSpaceWireAddress;

private:
	std::vector<uint8_t> prefix;
	uint8_t sourceLogicalAddress;

private:
	/// for Packet Control Field
	const static uint8_t protocolVersion = 0x01; //2bits
	uint8_t secondaryHeaderFlag; //1bit
	uint8_t sequenceFlags; //2bits
	uint8_t packetType; //3bits

private:
	std::vector<uint8_t> payload;
	uint16_t crc16;

private:
	double sentoutTimeStamp;

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
		sourceLogicalAddress = SpaceWirePacket::DefaultLogicalAddress;
	}

public:
	std::string  toString(){
		return "";
		//todo implement toString()
	}

public:
	std::vector<uint8_t>* getPacketBufferPointer() {
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>();
		constructHeader();

		//Target SpaceWire Address
		size_t destinationSpaceWireAddressSize = destinationSpaceWireAddress.size();
		for (size_t i = 0; i < destinationSpaceWireAddressSize; i++) {
			buffer->push_back(destinationSpaceWireAddress[i]);
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
		crc16 = SpaceWireRUtilities::calculateCRCForHeaderAndData(header, payload);

		//Trailer
		buffer->push_back(crc16 / 0x100);
		buffer->push_back(crc16 % 0x100);

		return buffer;
	}

public:
	size_t getPacket(uint8_t* buffer, size_t maxLength) {
		constructHeader();

		//Check buffer length
		size_t destinationSpaceWireAddressSize = destinationSpaceWireAddress.size();
		size_t headerSize = header.size();
		size_t payloadSize = payload.size();
		const size_t crcSize = 2;
		if (destinationSpaceWireAddressSize + headerSize + payloadSize + crcSize > maxLength) {
			return 0;
		}

		size_t index = 0;

		//SpaceWire Address
		for (size_t i = 0; i < destinationSpaceWireAddressSize; i++) {
			buffer[index] = destinationSpaceWireAddress[i];
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
		crc16 = SpaceWireRUtilities::calculateCRCForArray(buffer, index - destinationSpaceWireAddressSize + 1);

		//Trailer
		buffer[index] = crc16 / 0x100;
		index++;
		buffer[index] = crc16 % 0x100;
		index++;

		return index;
	}

public:
	bool isAckPacket() {
		if (this->packetType == SpaceWireRPacketType::DataAckPacket
				|| this->packetType == SpaceWireRPacketType::ControlAckPacket
				|| this->packetType == SpaceWireRPacketType::HeartBeatAckPacket) {
			return true;
		} else {
			return false;
		}
	}

public:
	void constructAckForPacket(SpaceWireRPacket* packet, std::vector<uint8_t> prefix) {
		this->constructAckForPacket(packet);
		this->prefix = prefix;
	}

public:
	void constructAckForPacket(SpaceWireRPacket* packet) {
		if (packet->isAckPacket()) {
			return;
		}

		//destination addresses
		this->destinationSpaceWireAddress = packet->getPrefix();
		this->destinationLogicalAddress = packet->getSourceLogicalAddress();

		//secondary header flag
		this->setSecondaryHeaderFlag(SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed);

		//set sequence flag bits
		this->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

		//set packet type
		if (packet->isControlPacketOpenCommand()) {
			this->setPacketType(SpaceWireRPacketType::ControlAckPacket);
		} else if (packet->isControlPacketCloseCommand()) {
			this->setPacketType(SpaceWireRPacketType::ControlAckPacket);
		} else if (packet->isDataPacket()) {
			this->setPacketType(SpaceWireRPacketType::DataAckPacket);
		} else if (packet->isHeartBeatPacketType()) {
			this->setPacketType(SpaceWireRPacketType::HeartBeatAckPacket);
		} else if (packet->isFlowControlPacket()) {
			//Note: Flow Control Packet and Flow Control Ack Packet use the
			//same packet type (0x06).
			this->setPacketType(SpaceWireRPacketType::FlowControlPacket);
		}

		//set payload length
		this->setPayloadLength(0);

		//set channel number
		this->setChannelNumber(packet->getChannelNumber());

		//sequence number
		this->setSequenceNumber(packet->getSequenceNumber());

		//source addresses
		this->sourceLogicalAddress = packet->destinationLogicalAddress;
		this->prefix.clear();

		//clear payload
		this->clearPayload();

	}

private:
	inline void clearPayload() {
		payload.clear();
	}

private:
	void constructHeader() {
		header.clear();
		header.push_back(this->destinationLogicalAddress);

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
		header.push_back(0x0000 + prefix.size());

		//Source Address
		for (size_t i = 0; i < prefix.size(); i++) {
			header.push_back(prefix[i]);
		}
		header.push_back(sourceLogicalAddress);
	}

public:
	void interpretPacket(std::vector<uint8_t>* buffer) throw (SpaceWireRPacketException) {
		using namespace std;

		if (buffer->size() < MinimumHeaderLength) {
			throw SpaceWireRPacketException(SpaceWireRPacketException::TooShortPacketSize);
		}

#ifdef debugSpaceWireRPacket
		cout << "SpaceWireRPacket::interpretPacket() #1" << endl;
#endif

		try {
			//SpaceWire Address
			size_t index = 0;
			std::vector<uint8_t> temporarySpaceWireAddress;
			size_t destinationSpaceWireAddressLength=0;
			try {
#ifdef debugSpaceWireRPacket
				cout << "SpaceWireRPacket::interpretPacket() #2" << endl;
#endif
				while (buffer->at(index) < 0x20) {
					temporarySpaceWireAddress.push_back(buffer->at(index));
					index++;
				}
#ifdef debugSpaceWireRPacket
				cout << "SpaceWireRPacket::interpretPacket() #3 Remaining path address " << index << " bytes [";
				for (size_t i = 0; i < temporarySpaceWireAddress.size(); i++) {
					cout << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) temporarySpaceWireAddress[i] << " ";
				}
				cout << "]" << endl;
#endif
			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidHeaderFormat);
			}
			this->destinationSpaceWireAddress = temporarySpaceWireAddress;
			destinationSpaceWireAddressLength=index;

			//Destination SLA
			this->destinationLogicalAddress = buffer->at(index);
			index++;

			//Protocol ID
#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() #4 ProtocolID=" << "0x" << hex << right << setw(2) << setfill('0')
					<< (uint32_t) buffer->at(index) << endl;
#endif
			if (buffer->at(index) != SpaceWireRProtocol::ProtocolID) {
				cerr << "Invalid Protocol ID: " << "0x" << hex << right << setw(2) << setfill('0')  << (uint32_t)buffer->at(index) << endl;
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidProtocolID);
			}
			index++;

#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() #5" << endl;
#endif
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

#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() packetControl=" << "0x" << hex << right << setw(2) << setfill('0')
					<< (uint32_t) packetControl << endl;
			cout << "SpaceWireRPacket::interpretPacket() channel number=" << "0x" << hex << right << setw(2) << setfill('0')
					<< channelNumber << endl;
			cout << "SpaceWireRPacket::interpretPacket() sequenceNumber=" << "0x" << hex << right << setw(2) << setfill('0')
					<< (uint32_t) sequenceNumber << endl;
#endif

#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() #6" << endl;
#endif
			//Prefix
			try {
				prefix.clear();
#ifdef debugSpaceWireRPacket
				cout << "SpaceWireRPacket::interpretPacket() #7 prefixLength=" << (size_t) prefixLength << endl;
#endif
				for (size_t i = 0; i < prefixLength; i++) {
#ifdef debugSpaceWireRPacket
					cout << "SpaceWireRPacket::interpretPacket() #8" << endl;
#endif
					prefix.push_back(buffer->at(index));
					index++;
				}
#ifdef debugSpaceWireRPacket
				cout << "SpaceWireRPacket::interpretPacket() #8-1 prefix = ";
				for (size_t i = 0; i < prefix.size(); i++) {
					cout << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) prefix[i] << " ";
				}
				cout << endl;
#endif

			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPrefixLength);
			}

			//Source SLA
			this->sourceLogicalAddress = buffer->at(index);
			index++;

			//Payload
			try {
				size_t payloadLengthValue = payloadLength[0] * 0x100 + payloadLength[1];
				payload.clear();
#ifdef debugSpaceWireRPacket
				cout << "SpaceWireRPacket::interpretPacket() #9 payloadLength=" << dec << payloadLengthValue << endl;
#endif
				for (size_t i = 0; i < payloadLengthValue; i++) {
					payload.push_back(buffer->at(index));
				}
				index += payloadLengthValue;
			} catch (...) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPayloadLength);
			}

			//Trailer
#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() #10" << endl;
#endif
			this->crc16 = buffer->at(index) * 0x100 + buffer->at(index + 1);
			index += 2;

			//Size check
#ifdef debugSpaceWireRPacket
			cout << "SpaceWireRPacket::interpretPacket() #11 index=" << dec << index << " buffer.size()=" << buffer->size()
					<< endl;
#endif
			if (index != buffer->size()) {
				throw SpaceWireRPacketException(SpaceWireRPacketException::PacketHasExtraBytesAfterTrailer);
			}

			//CRC Check
			uint16_t calculatedCRC = SpaceWireRUtilities::calculateCRCForArray((uint8_t*) (&(buffer->at(destinationSpaceWireAddressLength))),
					buffer->size() - 2 - destinationSpaceWireAddressLength);
			if (this->crc16 != calculatedCRC) {
				cerr << "SpaceWireRPacket::interpretPacket() #12 CRC received=" << "0x" << hex << right << setw(2) << setfill('0')
						<< (uint32_t) this->crc16 << " calculated=" << "0x" << hex << right << setw(2) << setfill('0')
						<< (uint32_t) calculatedCRC << endl;
				throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidCRC);
			}

		} catch (SpaceWireRPacketException& e) {
			throw e;
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
	inline uint16_t getChannelNumber() const {
		return channelNumber[0] * 0x100 + channelNumber[1];
	}

	inline uint16_t getCRC() const {
		return crc16;
	}

	inline std::vector<uint8_t> getHeader() const {
		return header;
	}

	inline uint8_t getPacketControl() const {
		return packetControl;
	}

	inline uint8_t getPacketType() const {
		return this->packetType;
	}

	inline std::vector<uint8_t>* getPayload() {
		return &payload;
	}

	inline uint16_t getPayloadLength() const {
		return payloadLength[0] * 0x100 + payloadLength[1];
	}

	inline uint8_t getSequenceNumber() const {
		return sequenceNumber;
	}

	inline std::vector<uint8_t> getSourceAddressPrefix() const {
		return prefix;
	}

	inline uint8_t getSourceLogicalAddress() const {
		return sourceLogicalAddress;
	}

	inline void setChannelNumber(uint16_t channelNumber) {
		this->channelNumber[0] = channelNumber / 0x100;
		this->channelNumber[1] = channelNumber % 0x100;
	}

	inline void setCRC(uint16_t crc) {
		this->crc16 = crc;
	}

	inline void setHeader(std::vector<uint8_t>& header) {
		this->header = header;
	}

	inline void setPacketControl(uint8_t packetControl) {
		this->packetControl = packetControl;
	}

	inline void setPacketType(uint8_t packetType) {
		this->packetType = packetType;
	}

	inline void setPayload(std::vector<uint8_t>& payload) throw (SpaceWireRPacketException) {
		if (payload.size() > SpaceWireRPacket::MaxPayloadLength) {
			throw SpaceWireRPacketException::InvalidPayloadLength;
		}
		this->payload = payload;
		setPayloadLength(payload.size());
	}

	inline void setPayload(uint8_t* payload, size_t length) throw (SpaceWireRPacketException) {
		if (length > SpaceWireRPacket::MaxPayloadLength) {
			throw SpaceWireRPacketException::InvalidPayloadLength;
		}
		this->payload.clear();
		for (size_t i = 0; i < length; i++) {
			this->payload.push_back(payload[i]);
		}
		setPayloadLength(length);
	}

	inline void setPayload(std::vector<uint8_t>* payload, size_t index, size_t length) throw (SpaceWireRPacketException) {
		if (length > SpaceWireRPacket::MaxPayloadLength) {
			throw SpaceWireRPacketException::InvalidPayloadLength;
		}
		this->payload.clear();
		size_t loopEnd = length + index;
		for (size_t i = index; i < loopEnd; i++) {
			this->payload.push_back(payload->at(i));
		}
		setPayloadLength(length);
	}

	inline void setPayloadLength(uint16_t payloadLength) {
		this->payloadLength[0] = payloadLength / 0x100;
		this->payloadLength[1] = payloadLength % 0x100;
	}

	inline void setSequenceNumber(uint8_t sequenceNumber) {
		this->sequenceNumber = sequenceNumber;
	}

	inline void setSourceAddressPrefix(std::vector<uint8_t>& sourceAddressPrefix) {
		this->prefix = sourceAddressPrefix;
	}

	inline void setSourceLogicalAddress(uint8_t sourceSpaceWireLogicalAddress) {
		this->sourceLogicalAddress = sourceSpaceWireLogicalAddress;
	}

	inline void setTrailer(uint16_t trailer) {
		setCRC(trailer);
	}

public:
	inline void setDestinationLogicalAddress(uint8_t destinationLogicalAddress) {
		this->destinationLogicalAddress = destinationLogicalAddress;
	}

public:
	inline void setDestinationSpaceWireAddress(std::vector<uint8_t>& destinationSpaceWireAddress) {
		this->destinationSpaceWireAddress = destinationSpaceWireAddress;
	}

///Packet Control
	inline uint8_t getSecondaryHeaderFlag() const {
		return secondaryHeaderFlag;
	}

	inline void setSecondaryHeaderFlag(uint8_t secondaryHeaderFlag) {
		this->secondaryHeaderFlag = secondaryHeaderFlag;
	}

	inline uint8_t getSequenceFlags() const {
		return sequenceFlags;
	}

	inline void setSequenceFlags(uint8_t sequenceFlags) {
		this->sequenceFlags = sequenceFlags;
	}

public:
	//set packet types
	inline void setDataPacketFlag() {
		setPacketType(SpaceWireRPacketType::DataPacket);
	}

	inline void setDataAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::DataAckPacket);
	}

	inline void setControlPacketOpenCommandFlag() {
		setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);
	}

	inline void setControlPacketCloseCommandFlag() {
		setPacketType(SpaceWireRPacketType::ControlPacketCloseCommand);
	}

	inline void setHeartBeatPacketFlag() {
		setPacketType(SpaceWireRPacketType::HeartBeatPacket);
	}

	inline void setHeartBeatAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::HeartBeatAckPacket);
	}

	inline void setFlowControlPacketFlag() {
		setPacketType(SpaceWireRPacketType::FlowControlPacket);
	}

	inline void setControlAckPacketFlag() {
		setPacketType(SpaceWireRPacketType::ControlAckPacket);
	}

public:
	inline void unuseSecondaryHeader() {
		this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed;
	}

	inline void useSecondaryHeader() {
		this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed;
	}

public:
	inline void setFirstSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::FirstSegment;
	}

public:
	inline void setContinuedSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::ContinuedSegment;
	}

public:
	inline void setLastSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::LastSegment;
	}

public:
	inline void setCompleteSegmentFlag() {
		this->sequenceFlags = SpaceWireRSequenceFlagType::CompleteSegment;
	}

public:
	inline bool isFirstSegment() {
		return (this->sequenceFlags == SpaceWireRSequenceFlagType::FirstSegment) ? true : false;
	}

public:
	inline bool isLastSegment() {
		return (this->sequenceFlags == SpaceWireRSequenceFlagType::LastSegment) ? true : false;
	}

public:
	inline bool isContinuedSegment() {
		return (this->sequenceFlags == SpaceWireRSequenceFlagType::ContinuedSegment) ? true : false;
	}

public:
	inline bool isCompleteSegment() {
		return (this->sequenceFlags == SpaceWireRSequenceFlagType::CompleteSegment) ? true : false;
	}

public:
	//prefix-related
	inline size_t getPrefixLength() const {
		return prefix.size();
	}

public:
	inline std::vector<uint8_t> getPrefix() const {
		return prefix;
	}

public:
	inline void setPrefix(std::vector<uint8_t>& prefix) {
		this->prefix = prefix;
		this->prefixLength = prefix.size();
	}

public:
	inline bool isDataPacket() {
		return (packetType == DataPacketType) ? true : false;
	}

public:
	inline bool isDataAckPacket() {
		return (packetType == DataAckPacketType) ? true : false;
	}

public:
	inline bool isControlPacketOpenCommand() {
		return (packetType == ControlPacketOpenCommandType) ? true : false;
	}

public:
	inline bool isControlPacketCloseCommand() {
		return (packetType == ControlPacketCloseCommandType) ? true : false;
	}

public:
	inline bool isHeartBeatPacketType() {
		return (packetType == HeartBeatPacketType) ? true : false;
	}

public:
	inline bool isHeartBeatAckPacketType() {
		return (packetType == HeartBeatAckPacketType) ? true : false;
	}

public:
	inline bool isFlowControlPacket() {
		return (packetType == FlowControlPacket) ? true : false;
	}

public:
	inline bool isControlAckPacket() {
		return (packetType == ControlAckPacket) ? true : false;
	}

public:
	inline bool hasSecondaryHeader() {
		return (secondaryHeaderFlag == 0x01) ? true : false;
	}

public:
	inline double getSentoutTimeStamp() const {
		return sentoutTimeStamp;
	}

public:
	inline void setSentoutTimeStamp(double sentoutTimeStamp) {
		this->sentoutTimeStamp = sentoutTimeStamp;
	}

public:
	void setCurrentTimeToSentoutTimeStamp() {
		this->sentoutTimeStamp = CxxUtilities::Time::getClockValueInMilliSec();
	}
};

class SpaceWireRDataPacket: public SpaceWireRPacket {
public:
	SpaceWireRDataPacket() {
		this->setDataPacketFlag();
	}

public:
	virtual ~SpaceWireRDataPacket() {
	}
};

class SpaceWireRDataAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRDataAckPacket() {
		this->setDataAckPacketFlag();
	}

public:
	virtual ~SpaceWireRDataAckPacket() {
	}
};

class SpaceWireROpenCommandPacket: public SpaceWireRPacket {
public:
	SpaceWireROpenCommandPacket() {
		this->setControlPacketOpenCommandFlag();
	}

public:
	virtual ~SpaceWireROpenCommandPacket() {
	}
};

class SpaceWireRCloseCommandPacket: public SpaceWireRPacket {
public:
	SpaceWireRCloseCommandPacket() {
		this->setControlPacketCloseCommandFlag();
	}

public:
	virtual ~SpaceWireRCloseCommandPacket() {
	}
};

class SpaceWireRHeartBeatPacket: public SpaceWireRPacket {
public:
	SpaceWireRHeartBeatPacket() {
		this->setHeartBeatPacketFlag();
	}

public:
	virtual ~SpaceWireRHeartBeatPacket() {
	}
};

class SpaceWireRHeartBeatAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRHeartBeatAckPacket() {
		this->setHeartBeatAckPacketFlag();
	}

public:
	virtual ~SpaceWireRHeartBeatAckPacket() {
	}
};

class SpaceWireRFlowControlPacket: public SpaceWireRPacket {
public:
	SpaceWireRFlowControlPacket() {
		this->setFlowControlPacketFlag();
	}

public:
	virtual ~SpaceWireRFlowControlPacket() {
	}
};

class SpaceWireRControlAckPacket: public SpaceWireRPacket {
public:
	SpaceWireRControlAckPacket() {
		this->setControlAckPacketFlag();
	}

public:
	virtual ~SpaceWireRControlAckPacket() {
	}
};

#endif /* SPACEWIRERPACKET_HH_ */
