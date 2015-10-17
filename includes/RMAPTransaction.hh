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
 * RMAPTransaction.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPTRANSACTION_HH_
#define RMAPTRANSACTION_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "RMAPPacket.hh"

class RMAPTransaction {
public:
	uint8_t targetLogicalAddress;
	uint8_t initiatorLogicalAddress;
	uint16_t transactionID;
	uint32_t transactionIDMode;
	CxxUtilities::Condition condition;
	double timeoutDuration;
	uint32_t state;

public:
	enum {
		AutoTransactionID = 0x00, ManualTransactionID = 0x01
	};

public:
	bool isNonblockingMode;

public:
	RMAPPacket* commandPacket;
	RMAPPacket* replyPacket;

public:
	RMAPTransaction() {
		timeoutDuration = DefaultTimeoutDuration;
		transactionIDMode = AutoTransactionID;
		replyPacket = NULL;
		commandPacket = NULL;
		isNonblockingMode = false;
	}

public:
	enum {
		//for RMAPInitiator-related transaction
		NotInitiated = 0x00,
		Initiated = 0x01,
		CommandSent = 0x02,
		ReplyReceived = 0x03,
		Timeout = 0x04,
		//for RMAPTarget-related transaction
		CommandPacketReceived = 0x10,
		ReplySet = 0x11,
		ReplySent = 0x12,
		Aborted = 0x13,
		ReplyCompleted = 0x14
	};

public:
	static constexpr double DefaultTimeoutDuration = 1000;

public:
	RMAPPacket* getCommandPacket() const {
		return commandPacket;
	}

	CxxUtilities::Condition* getCondition() {
		return &condition;
	}

	double getDefaultTimeoutDuration() {
		return DefaultTimeoutDuration;
	}

	uint8_t getInitiatorLogicalAddress() const {
		return initiatorLogicalAddress;
	}

	RMAPPacket* getReplyPacket() const {
		return replyPacket;
	}

	uint32_t getState() const {
		return state;
	}

	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

	double getTimeoutDuration() const {
		return timeoutDuration;
	}

	uint16_t getTransactionID() const {
		return transactionID;
	}

	void setCommandPacket(RMAPPacket* commandPacket) {
		this->commandPacket = commandPacket;
	}

	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

	void setReplyPacket(RMAPPacket* replyPacket) {
		this->replyPacket = replyPacket;
	}

	void setState(uint32_t state) {
		this->state = state;
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTimeoutDuration(double timeoutDuration) {
		this->timeoutDuration = timeoutDuration;
	}

	void setTransactionID(uint16_t transactionID) {
		this->transactionID = transactionID;
		this->transactionIDMode = ManualTransactionID;
	}

	uint32_t getTransactionIDMode() {
		return transactionIDMode;
	}

	bool isAutoTransactionIDMode() {
		if (transactionIDMode == AutoTransactionID) {
			return true;
		} else {
			return false;
		}
	}

	bool isManualTransactionIDMode() {
		if (transactionIDMode == AutoTransactionID) {
			return false;
		} else {
			return true;
		}
	}

	void setTransactionIDMode(uint32_t transactionIDMode) {
		this->transactionIDMode = transactionIDMode;
	}
};

#endif /* RMAPTRANSACTION_HH_ */
