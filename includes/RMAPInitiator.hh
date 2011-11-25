/*
 * RMAPInitiator.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPINITIATOR_HH_
#define RMAPINITIATOR_HH_

#include "RMAPPacket.hh"
#include "RMAPEngine.hh"
#include "RMAPTransaction.hh"
#include "RMAPTargetNode.hh"
#include "RMAPReplyStatus.hh"
#include "RMAPReplyException.hh"
#include "RMAPProtocol.hh"

class RMAPInitiatorException: public CxxUtilities::Exception {
public:
	enum {
		Timeout = 0x100, Aborted = 0x200, ReadReplyWithInsufficientData, ReadReplyWithTooMuchData, UnexpectedWriteReplyReceived
	};
public:
	RMAPInitiatorException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}
};

class RMAPInitiator {
private:
	RMAPEngine* rmapEngine;
	RMAPPacket* commandPacket;
	RMAPPacket* replyPacket;
	CxxUtilities::Mutex mutex;

private:
	uint8_t initiatorLogicalAddress;

private:
	bool incrementMode;
	bool verifyMode;
	bool replyMode;

public:
	RMAPInitiator(RMAPEngine *rmapEngine) {
		this->rmapEngine = rmapEngine;
		commandPacket = new RMAPPacket();
		replyPacket = new RMAPPacket();
	}

	~RMAPInitiator() {
		delete commandPacket;
		if (replyPacket != NULL) {
			delete replyPacket;
		}
	}

public:
	void lock() {
		using namespace std;
		//		cout << "RMAPInitiator Locking a mutex." << endl;
		mutex.lock();
	}

	void unlock() {
		using namespace std;
		//		cout << "RMAPInitiator UnLocking a mutex." << endl;
		mutex.unlock();
	}

public:
	void read(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, uint32_t length, uint8_t *buffer,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPInitiatorException, RMAPReplyException) {
		using namespace std;
		lock();
		if (replyPacket != NULL) {
			delete replyPacket;
			replyPacket = NULL;
		}
		commandPacket->setInitiatorLogicalAddress(this->initiatorLogicalAddress);
		commandPacket->setRead();
		commandPacket->setCommand();
		if (incrementMode) {
			commandPacket->setIncrementMode();
		} else {
			commandPacket->setNoIncrementMode();
		}
		commandPacket->setNoVerifyMode();
		commandPacket->setReplyMode();
		commandPacket->setExtendedAddress(0x00);
		commandPacket->setAddress(memoryAddress);
		commandPacket->setDataLength(length);
		/** InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below */
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		RMAPTransaction transaction;
		transaction.commandPacket = this->commandPacket;
		rmapEngine->initiateTransaction(transaction);
		transaction.condition.wait(timeoutDuration);
		if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				unlock();
				throw RMAPReplyException(replyPacket->getStatus());
			}
			if (replyPacket->getDataBuffer()->size() != length) {
				unlock();
				throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
			}
			replyPacket->getData(buffer, length);
			unlock();
			return;
		} else {
			transaction.state = RMAPTransaction::Timeout;
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

public:
	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, uint8_t *data, uint32_t length,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPInitiatorException, RMAPReplyException) {
		lock();
		if (replyPacket != NULL) {
			delete replyPacket;
			replyPacket = NULL;
		}
		commandPacket->setInitiatorLogicalAddress(this->initiatorLogicalAddress);
		commandPacket->setWrite();
		commandPacket->setCommand();
		if (incrementMode) {
			commandPacket->setIncrementMode();
		} else {
			commandPacket->setNoIncrementMode();
		}
		if (verifyMode) {
			commandPacket->setVerifyMode();
		} else {
			commandPacket->setNoVerifyMode();
		}
		if (replyMode) {
			commandPacket->setReplyMode();
		} else {
			commandPacket->setNoReplyMode();
		}
		commandPacket->setExtendedAddress(0x00);
		commandPacket->setAddress(memoryAddress);
		commandPacket->setDataLength(length);
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		RMAPTransaction transaction;
		transaction.commandPacket = this->commandPacket;
		rmapEngine->initiateTransaction(transaction);
		transaction.condition.wait(timeoutDuration);
		replyPacket = transaction.replyPacket;
		if (transaction.state == RMAPTransaction::CommandSent) {
			if (replyMode) {
				throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
			} else {
				unlock();
				return;
			}
		} else if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			if (replyMode) {
				if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
					unlock();
					throw RMAPReplyException(replyPacket->getStatus());
				}
				if (replyPacket->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
					unlock();
					return;
				} else {
					unlock();
					throw RMAPReplyException(replyPacket->getStatus());
				}
			} else {//no reply was expected
				unlock();
				throw RMAPInitiatorException(RMAPInitiatorException::UnexpectedWriteReplyReceived);
			}
		} else if (transaction.state == RMAPTransaction::Timeout) {
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

public:
	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

	uint8_t getInitiatorLogicalAddress() {
		return initiatorLogicalAddress;
	}

public:
	static const double DefaultTimeoutDuration = 1000;

public:
	bool getReplyMode() const {
		return replyMode;
	}

	void setReplyMode(bool replyMode) {
		this->replyMode = replyMode;
	}

	bool getIncrementMode() const {
		return incrementMode;
	}

	bool getVerifyMode() const {
		return verifyMode;
	}

	void setIncrementMode(bool incrementMode) {
		this->incrementMode = incrementMode;
	}

	void setVerifyMode(bool verifyMode) {
		this->verifyMode = verifyMode;
	}

};
#endif /* RMAPINITIATOR_HH_ */
