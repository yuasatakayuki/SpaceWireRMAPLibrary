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
#include "RMAPMemoryObject.hh"

class RMAPInitiatorException: public CxxUtilities::Exception {
public:
	enum {
		Timeout = 0x100,
		Aborted = 0x200,
		ReadReplyWithInsufficientData,
		ReadReplyWithTooMuchData,
		UnexpectedWriteReplyReceived,
		NoSuchRMAPMemoryObject,
		NoSuchRMAPTargetNode,
		RMAPTransactionCouldNotBeInitiated,
		SpecifiedRMAPMemoryObjectIsNotReadable,
		SpecifiedRMAPMemoryObjectIsNotWritable,
		SpecifiedRMAPMemoryObjectIsNotRMWable,
		RMAPTargetNodeDBIsNotRegistered,
		NonblockingTransactionHasNotBeenInitiated,
		NonblockingTransactionHasNotBeenCompleted
	};

public:
	RMAPInitiatorException(uint32_t status) :
			CxxUtilities::Exception(status) {
	}

public:
	virtual ~RMAPInitiatorException() {
	}

public:
	std::string toString() {
		std::string result;
		switch (status) {
		case Timeout:
			result = "Timeout";
			break;
		case Aborted:
			result = "Aborted";
			break;
		case ReadReplyWithInsufficientData:
			result = "ReadReplyWithInsufficientData";
			break;
		case ReadReplyWithTooMuchData:
			result = "ReadReplyWithTooMuchData";
			break;
		case UnexpectedWriteReplyReceived:
			result = "UnexpectedWriteReplyReceived";
			break;
		case NoSuchRMAPMemoryObject:
			result = "NoSuchRMAPMemoryObject";
			break;
		case NoSuchRMAPTargetNode:
			result = "NoSuchRMAPTargetNode";
			break;
		case RMAPTransactionCouldNotBeInitiated:
			result = "RMAPTransactionCouldNotBeInitiated";
			break;
		case SpecifiedRMAPMemoryObjectIsNotReadable:
			result = "SpecifiedRMAPMemoryObjectIsNotReadable";
			break;
		case SpecifiedRMAPMemoryObjectIsNotWritable:
			result = "SpecifiedRMAPMemoryObjectIsNotWritable";
			break;
		case SpecifiedRMAPMemoryObjectIsNotRMWable:
			result = "SpecifiedRMAPMemoryObjectIsNotRMWable";
			break;
		case RMAPTargetNodeDBIsNotRegistered:
			result = "RMAPTargetNodeDBIsNotRegistered";
			break;
		case NonblockingTransactionHasNotBeenInitiated:
			result = "NonblockingTransactionHasNotBeenInitiated";
			break;
		case NonblockingTransactionHasNotBeenCompleted:
			result = "NonblockingTransactionHasNotBeenCompleted";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

class RMAPInitiator {
public:
	static const uint16_t DefaultTransactionID = 0x00;
	static const bool DefaultIncrementMode = true;
	static const bool DefaultVerifyMode = true;
	static const bool DefaultReplyMode = true;

private:
	RMAPEngine* rmapEngine;
	RMAPPacket* commandPacket;
	RMAPPacket* replyPacket;
	CxxUtilities::Mutex mutex;

	CxxUtilities::Mutex deleteReplyPacketMutex;

private:
	//optional DB
	RMAPTargetNodeDB* targetNodeDB;

private:
	uint8_t initiatorLogicalAddress;
	bool isInitiatorLogicalAddressSet_;

private:
	RMAPTransaction transaction;

private:
	bool incrementMode;
	bool isIncrementModeSet_;
	bool verifyMode;
	bool isVerifyModeSet_;
	bool replyMode;
	bool isReplyModeSet_;
	uint16_t transactionID;
	bool isTransactionIDSet_;

private:
	bool useDraftECRC;

public:
	RMAPInitiator(RMAPEngine* rmapEngine) {
		this->rmapEngine = rmapEngine;
		commandPacket = new RMAPPacket();
		replyPacket = new RMAPPacket();
		isIncrementModeSet_ = false;
		isVerifyModeSet_ = false;
		isReplyModeSet_ = false;
		isTransactionIDSet_ = false;
		useDraftECRC = false;

		transactionID = DefaultTransactionID;
		incrementMode = DefaultIncrementMode;
		verifyMode = DefaultVerifyMode;
		replyMode = DefaultReplyMode;
	}

	~RMAPInitiator() {
		if (commandPacket != NULL) {
			delete commandPacket;
		}
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
	}

public:
	void deleteReplyPacket() {
		deleteReplyPacketMutex.lock();
		if (replyPacket == NULL) {
			deleteReplyPacketMutex.unlock();
			return;
		}
		delete replyPacket;
		replyPacket = NULL;
		deleteReplyPacketMutex.unlock();
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
	void read(std::string targetNodeID, uint32_t memoryAddress, uint32_t length, uint8_t* buffer, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPTargetNode);
		}
		read(targetNode, memoryAddress, length, buffer, timeoutDuration);
	}

	/** easy to use, but somewhat slow due to data copy. */
	std::vector<uint8_t>* readConstructingNewVecotrBuffer(std::string targetNodeID, std::string memoryObjectID,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
					RMAPReplyException) {
		using namespace std;
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		cerr << targetNodeDB->getSize() << endl;
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPTargetNode);
		}

		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = targetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}

		//check if the memory is readable.
		if (!memoryObject->isReadable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotReadable);
		}

		std::vector<uint8_t>* buffer = new std::vector<uint8_t>(memoryObject->getLength());
		read(targetNode, memoryObject->getAddress(), memoryObject->getLength(), &(buffer->at(0)), timeoutDuration);
		return buffer;

		return new std::vector<uint8_t>(4);
	}

	void read(std::string targetNodeID, std::string memoryObjectID, uint8_t* buffer, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		read(targetNode, memoryObjectID, buffer, timeoutDuration);
	}

	void read(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, uint8_t *buffer, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = rmapTargetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		//check if the memory is readable.
		if (!memoryObject->isReadable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotReadable);
		}
		read(rmapTargetNode, memoryObject->getAddress(), memoryObject->getLength(), buffer, timeoutDuration);
	}

	/** Reads remote memory. This method blocks the current thread. For non-blocking access, use the nonblockingRead() method.
	 */
	void read(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint32_t length, uint8_t *buffer,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
					RMAPReplyException) {
		using namespace std;
		lock();
		transaction.isNonblockingMode = false;
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
		setCRCVersion();
		commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
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
		commandPacket->clearData();
		/** InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below */
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		transaction.commandPacket = this->commandPacket;
		//tid
		if (isTransactionIDSet_) {
			transaction.setTransactionID(transactionID);
		}
		try {
			rmapEngine->initiateTransaction(transaction);
		} catch (RMAPEngineException& e) {
			unlock();
			transaction.state = RMAPTransaction::NotInitiated;
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		} catch (...) {
			unlock();
			transaction.state = RMAPTransaction::NotInitiated;
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		}
		transaction.condition.wait(timeoutDuration);
		if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			transaction.replyPacket = NULL;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				uint8_t replyStatus = replyPacket->getStatus();
				unlock();
				transaction.state = RMAPTransaction::NotInitiated;
				deleteReplyPacket();
				throw RMAPReplyException(replyStatus);
			}
			if (length < replyPacket->getDataBuffer()->size()) {
				unlock();
				transaction.state = RMAPTransaction::NotInitiated;
				deleteReplyPacket();
				throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
			}
			replyPacket->getData(buffer, length);
			transaction.state = RMAPTransaction::NotInitiated;
			unlock();
			//when successful, replay packet is retained until next transaction for inspection by user application
			//deleteReplyPacket();
			return;
		} else {
			//cancel transaction (return transaction ID)
			rmapEngine->cancelTransaction(&transaction);
			transaction.state = RMAPTransaction::NotInitiated;
			unlock();
			deleteReplyPacket();
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

public:
	void nonblockingRead(std::string targetNodeID, uint32_t memoryAddress, uint32_t length) throw (RMAPEngineException,
			RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPTargetNode);
		}
		nonblockingRead(targetNode, memoryAddress, length);
	}

	void nonblockingRead(std::string targetNodeID, std::string memoryObjectID) throw (RMAPEngineException,
			RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		nonblockingRead(targetNode, memoryObjectID);
	}

	void nonblockingRead(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID) throw (RMAPEngineException,
			RMAPInitiatorException, RMAPReplyException) {
		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = rmapTargetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		//check if the memory is readable.
		if (!memoryObject->isReadable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotReadable);
		}
		nonblockingRead(rmapTargetNode, memoryObject->getAddress(), memoryObject->getLength());
	}

	/** Reads remote memory without blocking the current thread.
	 * This method returns immediately after initiating a transaction.
	 * A result of the transaction can be checked via isNonblockingReadCompleted(),
	 * and read data can be obtained via getNonblockingReadData().
	 * Do not invoke other read/write methods while a non-blocking
	 * read/write transaction is taking place, otherwise state information of
	 * the non-blocking access will be corrupted. A non-blocking transaction
	 * can be canceled via cancelNonblockingRead()/cancelNonblockingWrite().
	 */
	void nonblockingRead(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint32_t length)
			throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		using namespace std;
		lock();
		transaction.isNonblockingMode = true;
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
		setCRCVersion();
		commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
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
		commandPacket->clearData();
		/** InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below */
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		transaction.commandPacket = this->commandPacket;
		//tid
		if (isTransactionIDSet_) {
			transaction.setTransactionID(transactionID);
		}
		try {
			rmapEngine->initiateTransaction(transaction);
			unlock();
			return;
		} catch (RMAPEngineException& e) {
			transaction.state = RMAPTransaction::NotInitiated;
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		} catch (...) {
			transaction.state = RMAPTransaction::NotInitiated;
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		}
	}

	bool isNonblockingReadCompleted() {
		if (transaction.state == RMAPTransaction::ReplyReceived) {
			return true;
		} else {
			return false;
		}
	}

	void getNonblockingReadData(uint8_t *buffer, uint32_t length) throw (RMAPInitiatorException) {
		using namespace std;
		if (transaction.state == RMAPTransaction::NotInitiated) {
			throw RMAPInitiatorException(RMAPInitiatorException::NonblockingTransactionHasNotBeenInitiated);
		}
		if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				uint8_t replyStatus = replyPacket->getStatus();
				deleteReplyPacket();
				throw RMAPReplyException(replyStatus);
			}
			if (length < replyPacket->getDataBuffer()->size()) {
				deleteReplyPacket();
				throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
			}
			replyPacket->getData(buffer, length);
			//when successful, replay packet is retained until next transaction for inspection by user application
			//deleteReplyPacket();
			return;
		} else {
			throw RMAPInitiatorException(RMAPInitiatorException::NonblockingTransactionHasNotBeenCompleted);
		}
	}

	void cancelNonblockingRead() {
		try {
			//cancel transaction (return transaction ID)
			rmapEngine->cancelTransaction(&transaction);
		} catch (...) {
		}
	}

public:
	void write(std::string targetNodeID, uint32_t memoryAddress, uint8_t *data, uint32_t length, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode *targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		write(targetNode, memoryAddress, data, length, timeoutDuration);
	}

public:
	void write(std::string targetNodeID, uint32_t memoryAddress, std::vector<uint8_t>* data, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode *targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		if (data->size() != 0) {
			write(targetNode, memoryAddress, &(data->at(0)), data->size(), timeoutDuration);
		}else{
			uint8_t dummyValue;
			write(targetNode, memoryAddress, &dummyValue, 0, timeoutDuration);
		}
	}

	void write(std::string targetNodeID, std::string memoryObjectID, uint8_t* data, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode *targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		write(targetNode, memoryObjectID, data, timeoutDuration);
	}

	void write(RMAPTargetNode *rmapTargetNode, std::string memoryObjectID, uint8_t* data, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = rmapTargetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException& e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		//check if the memory is readable.
		if (!memoryObject->isWritable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotWritable);
		}
		write(rmapTargetNode, memoryObject->getAddress(), data, memoryObject->getLength(), timeoutDuration);
	}

	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, std::vector<uint8_t>* data,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
					RMAPReplyException) {
		uint8_t* pointer = NULL;
		if (data->size() != 0) {
			pointer = &(data->at(0));
		}
		write(rmapTargetNode, memoryAddress, pointer, data->size(), timeoutDuration);
	}

	/** Writes remote memory. This method blocks the current thread. For non-blocking access, use the writeAsynchronouly() method.
	 */
	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, uint8_t *data, uint32_t length,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
					RMAPReplyException) {
		lock();
		transaction.isNonblockingMode = false;
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
		setCRCVersion();
		commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
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
		commandPacket->setData(data, length);
		transaction.commandPacket = this->commandPacket;
		setRMAPTransactionOptions(transaction);
		rmapEngine->initiateTransaction(transaction);

		if (!replyMode) { //if reply is not expected
			if (transaction.state == RMAPTransaction::Initiated) {
				transaction.state = RMAPTransaction::Initiated;
				unlock();
				return;
			} else {
				transaction.state = RMAPTransaction::NotInitiated;
				unlock();
				//command was not sent successfully
				throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
			}
		}
		transaction.state = RMAPTransaction::CommandSent;

		//if reply is expected
		transaction.condition.wait(timeoutDuration);
		if (transaction.state == RMAPTransaction::CommandSent) {
			if (replyMode) {
				unlock();
				//cancel transaction (return transaction ID)
				rmapEngine->cancelTransaction(&transaction);
				//reply packet is not created, and therefore the line below is not necessary
				//deleteReplyPacket();
				transaction.state = RMAPTransaction::Initiated;
				throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
			} else {
				transaction.state = RMAPTransaction::NotInitiated;
				unlock();
				return;
			}
		} else if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			transaction.replyPacket = NULL;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				uint8_t replyStatus = replyPacket->getStatus();
				unlock();
				deleteReplyPacket();
				transaction.state = RMAPTransaction::NotInitiated;
				throw RMAPReplyException(replyStatus);
			}
			if (replyPacket->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
				unlock();
				//When successful, replay packet is retained until next transaction for inspection by user application
				//deleteReplyPacket();
				transaction.state = RMAPTransaction::NotInitiated;
				return;
			} else {
				uint8_t replyStatus = replyPacket->getStatus();
				unlock();
				deleteReplyPacket();
				transaction.state = RMAPTransaction::NotInitiated;
				throw RMAPReplyException(replyStatus);
			}
		} else if (transaction.state == RMAPTransaction::Timeout) {
			unlock();
			//cancel transaction (return transaction ID)
			rmapEngine->cancelTransaction(&transaction);
			deleteReplyPacket();
			transaction.state = RMAPTransaction::NotInitiated;
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

private:
	void setRMAPTransactionOptions(RMAPTransaction& transaction) {
		//increment mode
		if (isIncrementModeSet_) {
			if (incrementMode) {
				transaction.commandPacket->setIncrementFlag();
			} else {
				transaction.commandPacket->unsetIncrementFlag();
			}
		}
		//verify mode
		if (isVerifyModeSet_) {
			if (verifyMode) {
				transaction.commandPacket->setVerifyFlag();
			} else {
				transaction.commandPacket->setNoVerifyMode();
			}
		}
		//reply mode
		if (transaction.commandPacket->isRead() && transaction.commandPacket->isCommand()) {
			transaction.commandPacket->setReplyMode();
		} else if (isReplyModeSet_) {
			if (replyMode) {
				transaction.commandPacket->setReplyMode();
			} else {
				transaction.commandPacket->setNoReplyMode();
			}
		}
		//tid
		if (isTransactionIDSet_) {
			transaction.setTransactionID(transactionID);
		}
	}

public:
	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
		isInitiatorLogicalAddressSet_ = true;
	}

	uint8_t getInitiatorLogicalAddress() {
		if (isInitiatorLogicalAddressSet_ == true) {
			return initiatorLogicalAddress;
		} else {
			return RMAPProtocol::DefaultLogicalAddress;
		}
	}

	bool isInitiatorLogicalAddressSet() {
		return isInitiatorLogicalAddressSet_;
	}

	void unsetInitiatorLogicalAddress() {
		isInitiatorLogicalAddressSet_ = false;
	}

public:
	static constexpr double DefaultTimeoutDuration = 1000.0;

public:
	bool getReplyMode() const {
		return replyMode;
	}

	void setReplyMode(bool replyMode) {
		this->replyMode = replyMode;
		this->isReplyModeSet_ = true;
	}

	void setNoReplyMode() {
		this->replyMode = false;
		this->isReplyModeSet_ = true;
	}

	void unsetReplyMode() {
		isReplyModeSet_ = false;
	}

	bool isReplyModeSet() {
		return isReplyModeSet_;
	}

public:
	bool getIncrementMode() const {
		return incrementMode;
	}

	void setIncrementMode(bool incrementMode) {
		this->incrementMode = incrementMode;
		this->isIncrementModeSet_ = true;
	}

	void unsetIncrementMode() {
		this->isIncrementModeSet_ = false;
	}

	bool isIncrementModeSet() {
		return isIncrementModeSet_;
	}

public:
	bool getVerifyMode() const {
		return verifyMode;
	}

	void setVerifyMode(bool verifyMode) {
		this->verifyMode = verifyMode;
		this->isVerifyModeSet_ = true;
	}

	void unsetVerifyMode() {
		this->isVerifyModeSet_ = false;
	}

	bool isVerifyModeSet() {
		return isVerifyModeSet_;
	}

public:
	void setTransactionID(uint16_t transactionID) {
		this->transactionID = transactionID;
		this->isTransactionIDSet_ = true;
	}

	void unsetTransactionID() {
		this->isTransactionIDSet_ = false;
	}

	uint16_t getTransactionID() {
		return transactionID;
	}

	bool isTransactionIDSet() {
		return isTransactionIDSet_;
	}

public:
	RMAPPacket* getCommandPacketPointer() {
		return commandPacket;
	}

	RMAPPacket* getReplyPacketPointer() {
		return replyPacket;
	}

public:
	void setRMAPTargetNodeDB(RMAPTargetNodeDB* targetNodeDB) {
		this->targetNodeDB = targetNodeDB;
	}

	RMAPTargetNodeDB* getRMAPTargetNodeDB() {
		return targetNodeDB;
	}

public:
	bool isUseDraftECRC() const {
		return useDraftECRC;
	}

	void setUseDraftECRC(bool useDraftEcrc = true) {
		this->useDraftECRC = useDraftEcrc;
	}

private:
	inline void setCRCVersion() {
		if (!useDraftECRC) {
			commandPacket->setUseDraftECRC(false);
		} else {
			commandPacket->setUseDraftECRC(true);
		}
	}

};
#endif /* RMAPINITIATOR_HH_ */
