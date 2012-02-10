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
	RMAPPacket* commandPacket;
	RMAPPacket* replyPacket;

public:
	RMAPTransaction() {
		timeoutDuration = DefaultTimeoutDuration;
		transactionIDMode=AutoTransactionID;
		replyPacket=NULL;
		commandPacket=NULL;
	}

public:
	enum {
		//for RMAPInitiator-related transaction
		NotInitiated, Initiated, CommandSent, ReplyReceived, Timeout,
		//for RMAPTarget-related transaction
		CommandPacketReceived, ReplySet, ReplySent, Aborted, ReplyCompleted
	};

public:
	static const double DefaultTimeoutDuration = 1000;

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

	bool isAutoTransactionIDMode(){
		if(transactionIDMode==AutoTransactionID){
			return true;
		}else{
			return false;
		}
	}

	bool isManualTransactionIDMode(){
		if(transactionIDMode==AutoTransactionID){
			return false;
		}else{
			return true;
		}
	}

	void setTransactionIDMode(uint32_t transactionIDMode) {
		this->transactionIDMode = transactionIDMode;
	}
};

#endif /* RMAPTRANSACTION_HH_ */
