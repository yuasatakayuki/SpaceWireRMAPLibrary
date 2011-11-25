/*
 * RMAPInitiatorOptions.hh
 *
 *  Created on: Aug 9, 2011
 *      Author: yuasa
 */

#ifndef RMAPINITIATOROPTIONS_HH_
#define RMAPINITIATOROPTIONS_HH_

#include "CxxUtilities/CxxUtilities.hh"

class RMAPInitiatorOptions {
private:
	std::map<std::string,void*> options;

private:
	uint8_t protocolID;
	std::vector<uint8_t> targetSpaceWireAddress;
	uint8_t targetLogicalAddress;
	uint8_t instruction;
	uint8_t key;
	std::vector<uint8_t> replyAddress;
	uint8_t initiatorLogicalAddress;
	uint16_t transactionID;
	uint8_t extendedAddress;
	uint32_t address;
	uint32_t dataLength;
	uint8_t status;
	uint8_t headerCRC;
	uint8_t dataCRC;

public:
	uint32_t getAddress() const {
		return address;
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

	uint8_t getStatus() const {
		return status;
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

public:
	void setAddress(uint32_t address) {
		this->address = address;

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
	}

	void setStatus(uint8_t status) {
		this->status = status;
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

};
#endif /* RMAPINITIATOROPTIONS_HH_ */
