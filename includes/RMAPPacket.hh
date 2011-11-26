/*
 * RMAPPacket.hh
 *
 *  Created on: Jul 27, 2011
 *      Author: yuasa
 */

#ifndef RMAPPACKET_HH_
#define RMAPPACKET_HH_

#include <CxxUtilities/CommonHeader.hh>

#include "SpaceWireUtilities.hh"
#include "SpaceWireProtocol.hh"

#include "RMAPProtocol.hh"
#include "RMAPUtilities.hh"
#include "RMAPTargetNode.hh"

class RMAPPacketException: public CxxUtilities::Exception {
public:
	enum {
		ProtocolIDIsNotRMAP,
		PacketInterpretationFailed,
		InsufficientBufferSize,
		InvalidHeaderCRC,
		InvalidDataCRC,
		DataLengthMismatch
	};

public:
	RMAPPacketException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}

public:
	virtual std::string toString() {
		//return ClassInformation::demangle(typeid(*this).name());
		using namespace std;
		stringstream ss;
		switch (status) {
		case ProtocolIDIsNotRMAP:
			ss << "ProtocolIDIsNotRMAP";
			break;
		case PacketInterpretationFailed:
			ss << "PacketInterpretationFailed";
			break;
		case InsufficientBufferSize:
			ss << "InsufficientBufferSize";
			break;
		case InvalidHeaderCRC:
			ss << "InvalidHeaderCRC";
			break;
		case InvalidDataCRC:
			ss << "InvalidDataCRC";
			break;
		case DataLengthMismatch:
			ss << "DataLengthMismatch";
			break;
		default:
			ss << "Undefined exception";
		}
		return ss.str();
	}
};

class RMAPPacket {
private:
	std::vector<uint8_t> wholePacket;

private:
	uint8_t protocolID;
	std::vector<uint8_t> targetSpaceWireAddress;
	uint8_t targetLogicalAddress;
	uint8_t instruction;
	uint8_t key;
	std::vector<uint8_t> replyAddress;
	uint8_t initiatorLogicalAddress;
	uint8_t extendedAddress;
	uint32_t address;
	uint32_t dataLength;
	uint8_t status;
	uint8_t headerCRC;

private:
	uint16_t transactionID;

private:
	std::vector<uint8_t> header;

private:
	std::vector<uint8_t> data;
	uint8_t dataCRC;

private:
	uint32_t headerCRCMode;
	uint32_t dataCRCMode;

public:
	bool headerCRCIsChecked;
	bool dataCRCIsChecked;

public:
	enum {
		AutoCRC = 0x00, ManualCRC = 0x01
	};

public:
	static const uint8_t BitMaskForReserved = 0x80;
	static const uint8_t BitMaskForCommandReply = 0x40;
	static const uint8_t BitMaskForWriteRead = 0x20;
	static const uint8_t BitMaskForVerifyFlag = 0x10;
	static const uint8_t BitMaskForReplyFlag = 0x08;
	static const uint8_t BitMaskForIncrementFlag = 0x04;
	static const uint8_t BitMaskForReplyPathAddressLength = 0x3;
	bool getDataCRCIsChecked() const {
		return dataCRCIsChecked;
	}

	bool getHeaderCRCIsChecked() const {
		return headerCRCIsChecked;
	}

	void setDataCRCIsChecked(bool dataCRCIsChecked) {
		this->dataCRCIsChecked = dataCRCIsChecked;
	}

	void setHeaderCRCIsChecked(bool headerCRCIsChecked) {
		this->headerCRCIsChecked = headerCRCIsChecked;
	}

public:
	RMAPPacket() {
		protocolID = RMAPProtocol::ProtocolIdentifier;
		targetLogicalAddress = SpaceWireProtocol::DefaultLogicalAddress;
		key = RMAPProtocol::DefaultKey;
		transactionID = RMAPProtocol::DefaultTID;
		status = RMAPProtocol::DefaultStatus;
		instruction = 0x00;
		headerCRCMode = RMAPPacket::AutoCRC;
		dataCRCMode = RMAPPacket::AutoCRC;
		headerCRCIsChecked = RMAPProtocol::DefaultCRCCheckMode;
		dataCRCIsChecked = RMAPProtocol::DefaultCRCCheckMode;
	}

public:
	void constructHeader() {
		using namespace std;

		header.clear();
		if (isCommand()) {
			header.push_back(targetLogicalAddress);
			header.push_back(protocolID);
			header.push_back(instruction);
			header.push_back(key);
			std::vector<uint8_t> tmpvector;
			uint8_t tmporaryCounter = replyAddress.size();
			while (tmporaryCounter % 4 != 0) {
				header.push_back(0x00);
				tmporaryCounter++;
			}
			for (size_t i = 0; i < replyAddress.size(); i++) {
				header.push_back(replyAddress.at(i));
			}

			header.push_back(initiatorLogicalAddress);
			header.push_back((uint8_t) (((((transactionID & 0xff00) >> 8)))));
			header.push_back((uint8_t) (((((transactionID & 0x00ff) >> 0)))));
			header.push_back(extendedAddress);
			header.push_back((uint8_t) (((((address & 0xff000000) >> 24)))));
			header.push_back((uint8_t) (((((address & 0x00ff0000) >> 16)))));
			header.push_back((uint8_t) (((((address & 0x0000ff00) >> 8)))));
			header.push_back((uint8_t) (((((address & 0x000000ff) >> 0)))));
			header.push_back((uint8_t) (((((dataLength & 0x00ff0000) >> 16)))));
			header.push_back((uint8_t) (((((dataLength & 0x0000ff00) >> 8)))));
			header.push_back((uint8_t) (((((dataLength & 0x000000ff) >> 0)))));
		} else {
			header.push_back(initiatorLogicalAddress);
			header.push_back(protocolID);
			header.push_back(instruction);
			header.push_back(status);
			header.push_back(targetLogicalAddress);
			header.push_back((uint8_t) (((((transactionID & 0xff00) >> 8)))));
			header.push_back((uint8_t) (((((transactionID & 0x00ff) >> 0)))));
			if (isRead()) {
				header.push_back(0);
				header.push_back((uint8_t) (((((dataLength & 0x00ff0000) >> 16)))));
				header.push_back((uint8_t) (((((dataLength & 0x0000ff00) >> 8)))));
				header.push_back((uint8_t) (((((dataLength & 0x000000ff) >> 0)))));
			}
		}

		if (headerCRCMode == RMAPPacket::AutoCRC) {
			headerCRC = RMAPUtilities::calculateCRC(header);
		}
		header.push_back(headerCRC);
	}

	inline void calculateDataCRC() {
		dataCRC = RMAPUtilities::calculateCRC(data);
	}

	void constructPacket() {
		using namespace std;
		constructHeader();
		if (dataCRCMode == RMAPPacket::AutoCRC) {
			calculateDataCRC();
		}
		wholePacket.clear();
		if (isCommand() == true) {
			SpaceWireUtilities::concatenateTo(wholePacket, targetSpaceWireAddress);
		} else {
			SpaceWireUtilities::concatenateTo(wholePacket, replyAddress);
		}
		SpaceWireUtilities::concatenateTo(wholePacket, header);
		SpaceWireUtilities::concatenateTo(wholePacket, data);
		if (hasData()) {
			wholePacket.push_back(dataCRC);
		}
	}

	std::vector<uint8_t> getPacket() {
		return wholePacket;
	}

	std::vector<uint8_t> *getPacketBufferPointer() {
		return &wholePacket;
	}

public:
	void interpretAsAnRMAPPacket(uint8_t *packet, uint32_t length) throw (RMAPPacketException) {
		using namespace std;

		std::vector<uint8_t> temporaryPathAddress;
		try {
			int i = 0;
			int rmapIndex = 0;
			int rmapIndexAfterSourcePathAddress = 0;
			int dataIndex = 0;
			temporaryPathAddress.clear();
			while (packet[i] < 0x20) {
				temporaryPathAddress.push_back(packet[i]);
				i++;
			}
			rmapIndex = i;
			if (packet[rmapIndex + 1] != RMAPProtocol::ProtocolIdentifier) {
				throw(RMAPPacketException(RMAPPacketException::ProtocolIDIsNotRMAP));
			}
			using namespace std;
			instruction = packet[rmapIndex + 2];
			uint8_t replyPathAddressLength = getReplyPathAddressLength();
			if (isCommand()) {
				setTargetSpaceWireAddress(temporaryPathAddress);
				setTargetLogicalAddress(packet[rmapIndex]);
				setKey(packet[rmapIndex + 3]);
				vector<uint8_t> temporaryReplyAddress;
				for (uint8_t i = 0; i < replyPathAddressLength * 4; i++) {
					temporaryReplyAddress.push_back(packet[rmapIndex + 4 + i]);
				}
				setReplyAddress(temporaryReplyAddress);
				rmapIndexAfterSourcePathAddress = rmapIndex + 4 + replyPathAddressLength * 4;
				setInitiatorLogicalAddress(packet[rmapIndexAfterSourcePathAddress + 0]);
				uint8_t uppertid, lowertid;
				uppertid = packet[rmapIndexAfterSourcePathAddress + 1];
				lowertid = packet[rmapIndexAfterSourcePathAddress + 2];
				setTransactionID((uint32_t) ((uppertid * 0x100 + lowertid)));
				setExtendedAddress(packet[rmapIndexAfterSourcePathAddress + 3]);
				uint8_t address_3, address_2, address_1, address_0;
				address_3 = packet[rmapIndexAfterSourcePathAddress + 4];
				address_2 = packet[rmapIndexAfterSourcePathAddress + 5];
				address_1 = packet[rmapIndexAfterSourcePathAddress + 6];
				address_0 = packet[rmapIndexAfterSourcePathAddress + 7];
				setAddress(address_3 * 0x01000000 + address_2 * 0x00010000 + address_1 * 0x00000100 + address_0
						* 0x00000001);
				uint8_t length_2, length_1, length_0;
				length_2 = packet[rmapIndexAfterSourcePathAddress + 8];
				length_1 = packet[rmapIndexAfterSourcePathAddress + 9];
				length_0 = packet[rmapIndexAfterSourcePathAddress + 10];
				setDataLength(length_2 * 0x010000 + length_1 * 0x000100 + length_0 * 0x000001);
				uint8_t temporaryHeaderCRC = packet[rmapIndexAfterSourcePathAddress + 11];
				if (headerCRCIsChecked == true) {
					uint32_t headerCRCMode_original = headerCRCMode;
					headerCRCMode = RMAPPacket::AutoCRC;
					constructHeader();
					headerCRCMode = headerCRCMode_original;
					if (headerCRC != temporaryHeaderCRC) {
						throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
					}
				} else {
					headerCRC = temporaryHeaderCRC;
				}
				dataIndex = rmapIndexAfterSourcePathAddress + 12;
				data.clear();
				if (isWrite()) {
					for (uint32_t i = 0; i < getDataLength(); i++) {
						if ((dataIndex + i) < (length - 1)) {
							data.push_back(packet[dataIndex + i]);
						} else {
							throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
						}
					}
					//length check for DataCRC
					uint8_t temporaryDataCRC = 0x00;
					if ((dataIndex + getDataLength()) == (length - 1)) {
						temporaryDataCRC = packet[dataIndex + getDataLength()];
					} else {
						throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
					}
					calculateDataCRC();
					if (dataCRCIsChecked == true) {
						if (dataCRC != temporaryDataCRC) {
							throw(RMAPPacketException(RMAPPacketException::InvalidDataCRC));
						}
					} else {
						dataCRC = temporaryDataCRC;
					}
				}
			} else {
				temporaryPathAddress.clear();
				uint32_t replyPathAddressLength = getReplyPathAddressLength();
				for (int i = 0; i < replyPathAddressLength * 4; i++) {
					temporaryPathAddress.push_back(0);
				}
				setReplyAddress(temporaryPathAddress);
				setInitiatorLogicalAddress(packet[rmapIndex]);
				setStatus(packet[rmapIndex + 3]);
				setTargetLogicalAddress(packet[rmapIndex + 4]);
				uint8_t uppertid, lowertid;
				uppertid = packet[rmapIndex + 5];
				lowertid = packet[rmapIndex + 6];
				setTransactionID((uint32_t) ((uppertid * 0x100 + lowertid)));
				if (isWrite()) {
					uint8_t temporaryHeaderCRC = packet[rmapIndex + 7];
					uint32_t headerCRCMode_original = headerCRCMode;
					headerCRCMode = RMAPPacket::AutoCRC;
					constructHeader();
					headerCRCMode = headerCRCMode_original;
					if (headerCRCIsChecked == true) {
						if (headerCRC != temporaryHeaderCRC) {
							throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
						}
					} else {
						headerCRC = temporaryHeaderCRC;
					}
				} else {
					uint8_t length_2, length_1, length_0;
					length_2 = packet[rmapIndex + 8];
					length_1 = packet[rmapIndex + 9];
					length_0 = packet[rmapIndex + 10];
					setDataLength(length_2 * 0x010000 + length_1 * 0x000100 + length_0 * 0x000001);
					uint8_t temporaryHeaderCRC = packet[rmapIndex + 11];
					constructHeader();
					if (headerCRCIsChecked == true) {
						if (headerCRC != temporaryHeaderCRC) {
							throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
						}
					} else {
						headerCRC = temporaryHeaderCRC;
					}
					dataIndex = rmapIndex + 12;
					data.clear();
					for (uint32_t i = 0; i < getDataLength(); i++) {
						try {
							data.push_back(packet[dataIndex + i]);
						} catch (...) {
							dataCRC = 0x00;//initialized
							throw RMAPPacketException(RMAPPacketException::DataLengthMismatch);
						}
					}
					uint8_t temporaryDataCRC = packet[dataIndex + getDataLength()];
					calculateDataCRC();
					if (dataCRCIsChecked == true) {
						if (dataCRC != temporaryDataCRC) {
							throw(RMAPPacketException(RMAPPacketException::InvalidDataCRC));
						}
					}
				}
			}
		} catch (exception e) {
			throw(RMAPPacketException(RMAPPacketException::PacketInterpretationFailed));
		}
		uint32_t previousHeaderCRCMode = headerCRCMode;
		uint32_t previousDataCRCMode = dataCRCMode;
		headerCRCMode = RMAPPacket::ManualCRC;
		dataCRCMode = RMAPPacket::ManualCRC;
		constructPacket();
		headerCRCMode = previousHeaderCRCMode;
		dataCRCMode = previousDataCRCMode;
	}

	inline void interpretAsAnRMAPPacket(std::vector<uint8_t>& data) throw (RMAPPacketException) {
		if (data.size() == 0) {
			throw RMAPPacketException(RMAPPacketException::PacketInterpretationFailed);
		}
		interpretAsAnRMAPPacket(&(data[0]), data.size());
	}

	inline void interpretAsAnRMAPPacket(std::vector<uint8_t>* data) throw (RMAPPacketException) {
		if (data->size() == 0) {
			throw RMAPPacketException(RMAPPacketException::PacketInterpretationFailed);
		}
		interpretAsAnRMAPPacket(&(data->at(0)), data->size());
	}

public:
	void setRMAPTargetInformation(RMAPTargetNode* rmapTargetNode) {
		setTargetLogicalAddress(rmapTargetNode->getTargetLogicalAddress());
		setReplyAddress(rmapTargetNode->getReplyAddress());
		setTargetSpaceWireAddress(rmapTargetNode->getTargetSpaceWireAddress());
		setKey(rmapTargetNode->getDefaultKey());
		if (rmapTargetNode->isInitiatorLogicalAddressSet()) {
			setInitiatorLogicalAddress(rmapTargetNode->getInitiatorLogicalAddress());
		}
	}

	inline void setRMAPTargetInformation(RMAPTargetNode& rmapTargetNode) {
		setRMAPTargetInformation(&rmapTargetNode);
	}

public:
	inline bool isCommand() {
		if ((instruction & RMAPPacket::BitMaskForCommandReply) > 0) {
			return true;
		} else {
			return false;
		}
	}

	inline void setCommand() {
		instruction = instruction | RMAPPacket::BitMaskForCommandReply;
	}

	inline bool isReply() {
		if ((instruction & RMAPPacket::BitMaskForCommandReply) > 0) {
			return false;
		} else {
			return true;
		}
	}

	inline bool setReply() {
		instruction = instruction & (~RMAPPacket::BitMaskForCommandReply);
	}

	inline bool isWrite() {
		if ((instruction & RMAPPacket::BitMaskForWriteRead) > 0) {
			return true;
		} else {
			return false;
		}
	}

	inline void setWrite() {
		instruction = instruction | RMAPPacket::BitMaskForWriteRead;
	}

	inline bool isRead() {
		if ((instruction & RMAPPacket::BitMaskForWriteRead) > 0) {
			return false;
		} else {
			return true;
		}
	}

	inline void setRead() {
		instruction = instruction & (~RMAPPacket::BitMaskForWriteRead);
	}

	inline bool isVerifyFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForVerifyFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

	inline void setVerifyFlag() {
		instruction = instruction | RMAPPacket::BitMaskForVerifyFlag;
	}

	inline void unsetVerifyFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForVerifyFlag);
	}

	inline void setVerifyMode() {
		setVerifyFlag();
	}

	inline void setNoVerifyMode() {
		unsetVerifyFlag();
	}

	inline bool isReplyFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForReplyFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

	inline void setReplyFlag() {
		instruction = instruction | RMAPPacket::BitMaskForReplyFlag;
	}

	inline void unsetReplyFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForReplyFlag);
	}

	inline void setReplyMode() {
		setReplyFlag();
	}

	inline void setNoReplyMode() {
		unsetReplyFlag();
	}

	inline bool isIncrementFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForIncrementFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

	inline void setIncrementFlag() {
		instruction = instruction | RMAPPacket::BitMaskForIncrementFlag;
	}

	inline void unsetIncrementFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForIncrementFlag);
	}

	inline void setIncrementMode() {
		setIncrementFlag();
	}

	inline void setNoIncrementMode() {
		unsetIncrementFlag();
	}

	inline uint8_t getReplyPathAddressLength() {
		return instruction & RMAPPacket::BitMaskForReplyPathAddressLength;
	}

	inline void setReplyPathAddressLength(uint8_t pathAddressLength) {
		instruction = (instruction & ~(RMAPPacket::BitMaskForIncrementFlag)) + pathAddressLength
				& RMAPPacket::BitMaskForIncrementFlag;
	}

public:
	uint32_t getAddress() const {
		return address;
	}

public:
	bool hasData() {
		if (data.size() != 0) {
			return true;
		} else {
			if ((isCommand() && isWrite()) || (isReply() && isRead())) {
				return true;
			} else {
				return false;
			}
		}
	}

	std::vector<uint8_t> getData() const {
		return data;
	}

	void getData(uint8_t* buffer, uint32_t maxLength) throw (RMAPPacketException) {
		uint32_t length = data.size();
		if (maxLength < length) {
			throw RMAPPacketException(RMAPPacketException::InsufficientBufferSize);
		}
		for (uint32_t i = 0; i < length; i++) {
			buffer[i] = data[i];
		}
	}

	void getData(std::vector<uint8_t>& buffer) {
		uint32_t length = data.size();
		buffer.resize(length);
		getData(&(buffer[0]), length);
	}

	void getData(std::vector<uint8_t>* buffer) {
		uint32_t length = data.size();
		buffer->resize(length);
		getData(&(buffer->at(0)), length);
	}

	std::vector<uint8_t>* getDataBuffer() {
		return &data;
	}

	uint8_t getDataCRC() const {
		return dataCRC;
	}

	uint32_t getDataLength() const {
		return dataLength;
	}

	uint8_t getExtendedAddress() const {
		return extendedAddress;
	}

	uint8_t getHeaderCRC() const {
		return headerCRC;
	}

	uint8_t getInitiatorLogicalAddress() const {
		return initiatorLogicalAddress;
	}

	uint8_t getInstruction() const {
		return instruction;
	}

	uint8_t getKey() const {
		return key;
	}

	uint8_t getProtocolID() const {
		return protocolID;
	}

	std::vector<uint8_t> getReplyAddress() const {
		return replyAddress;
	}

	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

	std::vector<uint8_t> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

	uint16_t getTransactionID() const {
		return transactionID;
	}

	void setAddress(uint32_t address) {
		this->address = address;
	}

	void setData(std::vector<uint8_t>& data) {
		this->data = data;
	}

	void setDataCRC(uint8_t dataCRC) {
		this->dataCRC = dataCRC;
	}

	void setDataLength(uint32_t dataLength) {
		this->dataLength = dataLength;
	}

	void setExtendedAddress(uint8_t extendedAddress) {
		this->extendedAddress = extendedAddress;
	}

	void setHeaderCRC(uint8_t headerCRC) {
		this->headerCRC = headerCRC;
	}

	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

	void setInstruction(uint8_t instruction) {
		this->instruction = instruction;
	}

	void setKey(uint8_t key) {
		this->key = key;
	}

	void setProtocolID(uint8_t protocolID) {
		this->protocolID = protocolID;
	}

	void setReplyAddress(std::vector<uint8_t> replyAddress) {
		this->replyAddress = replyAddress;
		if (replyAddress.size() % 4 == 0) {
			instruction = (instruction & (~BitMaskForReplyPathAddressLength)) + replyAddress.size() / 4;
		} else {
			instruction = (instruction & (~BitMaskForReplyPathAddressLength)) + (replyAddress.size() + 4) / 4;
		}
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTargetSpaceWireAddress(std::vector<uint8_t> targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

	void setTransactionID(uint16_t transactionID) {
		this->transactionID = transactionID;
	}

	uint8_t getStatus() const {
		return status;
	}

	void setStatus(uint8_t status) {
		this->status = status;
	}

	uint32_t getHeaderCRCMode() const {
		return headerCRCMode;
	}

	void setHeaderCRCMode(uint32_t headerCRCMode) {
		this->headerCRCMode = headerCRCMode;
	}

	uint32_t getDataCRCMode() const {
		return dataCRCMode;
	}

	void setDataCRCMode(uint32_t dataCRCMode) {
		this->dataCRCMode = dataCRCMode;
	}

	inline void addData(uint8_t oneByte) {
		this->data.push_back(oneByte);
	}

	inline void clearData() {
		data.clear();
	}

	inline void addData(std::vector<uint8_t> array) {
		size_t size = array.size();
		for (size_t i = 0; i < size; i++) {
			data.push_back(array[i]);
		}
	}

public:
	std::string toString() {
		if (isCommand()) {
			return toStringCommandPacket();
		} else {
			return toStringReplyPacket();
		}
	}

private:
	std::string toStringCommandPacket() {
		using namespace std;
		stringstream ss;
		///////////////////////////////
		//Command
		///////////////////////////////
		//Target SpaceWire Address
		if (targetSpaceWireAddress.size() != 0) {
			ss << "--------- Target SpaceWire Address ---------" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &targetSpaceWireAddress, 1, 128);
		}

		//Header
		ss << "--------- RMAP Header Part ---------" << endl;
		//Initiator Logical Address
		ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex
				<< (uint32_t) initiatorLogicalAddress << endl;
		//Target Logical Address
		ss << "Target Logic. Address     : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) targetLogicalAddress << endl;
		//Protocol Identifier
		ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) 1 << endl;
		//Instruction
		ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) instruction
				<< endl;
		toStringInstructionField(ss);
		//Key
		ss << "Key                       : 0x" << setw(2) << setfill('0') << hex << (unsigned int) key << endl;
		//Reply Address
		if (replyAddress.size() != 0) {
			ss << "Reply Address             : ";
			SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
		} else {
			ss << "Reply Address         : --none--" << endl;
		}
		ss << "Transaction Identifier    : 0x" << right << setw(2) << setfill('0') << hex
				<< ((unsigned int) transactionID) / 0x100 << " 0x" << right << setw(2) << setfill('0') << hex
				<< ((unsigned int) transactionID) % 0x100 << endl;
		ss << "Extended Address          : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) extendedAddress << endl;
		ss << "Address                   : 0x" << right << setw(8) << setfill('0') << hex << (unsigned int) address
				<< endl;
		ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int) dataLength
				<< endl;
		ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) headerCRC
				<< endl;

		//Data Part
		ss << "---------  RMAP Data Part  ---------" << endl;
		if (isWrite()) {
			ss << "[data size = " << dec << dataLength << "bytes]" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &data, 1, 16);
			ss << "Data CRC                  : " << right << setw(2) << setfill('0') << hex << (unsigned int) dataCRC
					<< endl;
		} else {
			ss << "--- none ---" << endl;
		}
		ss << endl;
		this->constructPacket();
		ss << "Total data (bytes)        : " << dec << this->getPacketBufferPointer()->size() << endl;
		ss << dec << endl;

		return ss.str();
	}

	std::string toStringReplyPacket() {
		using namespace std;
		stringstream ss;

		///////////////////////////////
		//Reply
		///////////////////////////////
		ss << "RMAP Packet Dump" << endl;
		//Reply Address
		if (replyAddress.size() != 0) {
			ss << "--------- Reply Address ---------" << endl;
			ss << "Reply Address       : ";
			SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
		}

		//Header
		ss << "--------- RMAP Header Part ---------" << endl;
		//Initiator Logical Address
		ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex
				<< (uint32_t) initiatorLogicalAddress << endl;
		//Target Logical Address
		ss << "Target Logical Address    : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) targetLogicalAddress << endl;
		//Protocol Identifier
		ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) 1 << endl;
		//Instruction
		ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) instruction
				<< endl;
		toStringInstructionField(ss);
		//Status
		string statusstring;
		switch (status) {
		case 0x00:
			statusstring = "Successfully Executed";
			break;
		case 0x01:
			statusstring = "General Error";
			break;
		case 0x02:
			statusstring = "Unused RMAP Packet Type or Command Code";
			break;
		case 0x03:
			statusstring = "Invalid Target Key";
			break;
		case 0x04:
			statusstring = "Invalid Data CRC";
			break;
		case 0x05:
			statusstring = "Early EOP";
			break;
		case 0x06:
			statusstring = "Cargo Too Large";
			break;
		case 0x07:
			statusstring = "EEP";
			break;
		case 0x08:
			statusstring = "Reserved";
			break;
		case 0x09:
			statusstring = "Verify Buffer Overrun";
			break;
		case 0x0a:
			statusstring = "RMAP Command Not Implemented or Not Authorized";
			break;
		case 0x0b:
			statusstring = "Invalid Target Logical Address";
			break;
		default:
			statusstring = "Reserved";
			break;
		}
		ss << "Status                    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) status
				<< " (" << statusstring << ")" << endl;
		ss << "Transaction Identifier    : 0x" << right << setw(2) << setfill('0') << hex
				<< ((unsigned int) transactionID) / 0x100 << " 0x" << setw(2) << setfill('0') << hex
				<< ((unsigned int) transactionID) % 0x100 << endl;
		if (isRead()) {
			ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex
					<< (unsigned int) dataLength << endl;
		}
		ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) headerCRC
				<< endl;

		//Data Part
		ss << "---------  RMAP Data Part  ---------" << endl;
		if (isRead()) {
			ss << "[data size = " << dec << dataLength << "bytes]" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &data, 1, 128);
			ss << "Data CRC    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) dataCRC << endl;
		} else {
			ss << "--- none ---" << endl;
		}
		ss << endl;
		this->constructPacket();
		ss << "Total data (bytes)        : " << dec << this->getPacketBufferPointer()->size() << endl;
		ss << dec << endl;

		return ss.str();
	}

public:
	void toStringInstructionField(std::stringstream& ss){
		using namespace std;
		//packet type (Command or Reply)
		ss << " ------------------------------" << endl;
		ss << " |Reserved    : 0" << endl;
		ss << " |Packet Type : " << (isCommand() ? 1 : 0);
		ss << " " << (isCommand() ? "(Command)" : "(Reply)") << endl;
		//Write or Read
		ss << " |Write/Read  : " << (isWrite() ? 1 : 0);
		ss << " " << (isWrite() ? "(Write)" : "(Read)") << endl;
		//Verify mode
		ss << " |Verify Mode : " << (isVerifyFlagSet() ? 1 : 0);
		ss << " " << (isVerifyFlagSet() ? "(Verify)" : "(No Verify)") << endl;
		//Ack mode
		ss << " |Reply Mode  : " << (isReplyFlagSet() ? 1 : 0);
		ss << " " << (isReplyFlagSet() ? "(Reply)" : "(No Reply)") << endl;
		//Increment
		ss << " |Increment   : " << (isIncrementFlagSet() ? 1 : 0);
		ss << " " << (isIncrementFlagSet() ? "(Increment)" : "(No Increment)") << endl;
		//SPAL
		ss << " |R.A.L.      : " << setw(1) << setfill('0') << hex << (uint32_t) (instruction
				& BitMaskForReplyPathAddressLength) << endl;
		ss << " |(R.A.L. = Reply Address Length)" << endl;
		ss << " ------------------------------" << endl;
	}
};
#endif /* RMAPPACKET_HH_ */
