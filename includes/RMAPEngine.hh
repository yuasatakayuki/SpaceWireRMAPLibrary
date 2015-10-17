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
 * RMAPEngine.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPENGINE_HH_
#define RMAPENGINE_HH_

#include "CxxUtilities/CommonHeader.hh"
#include "CxxUtilities/Mutex.hh"
#include "CxxUtilities/Action.hh"

#include "RMAPTransaction.hh"
#include "RMAPTarget.hh"
#include "SpaceWireIF.hh"
#include "SpaceWireUtilities.hh"

class RMAPEngineStoppedAction: public CxxUtilities::Action<void> {
public:
	virtual ~RMAPEngineStoppedAction() {
	}

public:
	virtual void doAction(void* rmapEngine) = 0;
};

class RMAPEngineException: public CxxUtilities::Exception {
public:
	enum {
		RMAPEngineIsNotStarted,
		TransactionIDIsNotAvailable,
		TooManyConcurrentTransactions,
		SpecifiedTransactionIDIsAlreadyInUse,
		PacketWasNotSentCorrectly,
		SpaceWireIFDisconnected,
		UnexpectedRMAPReplyPacketWasReceived
	};

private:
	RMAPPacket* rmapPacketCausedThisException;
	bool causeIsRegistered;

public:
	RMAPEngineException(uint32_t status) :
			CxxUtilities::Exception(status) {
		rmapPacketCausedThisException = NULL;
		causeIsRegistered = false;
	}

public:
	virtual ~RMAPEngineException() {
	}

public:
	RMAPEngineException(uint32_t status, RMAPPacket* packetCausedThisException) :
			CxxUtilities::Exception(status) {
		this->rmapPacketCausedThisException = packetCausedThisException;
		causeIsRegistered = true;
	}

public:
	RMAPPacket* getRMAPPacketCausedThisException() {
		return rmapPacketCausedThisException;
	}

public:
	bool isRMAPPacketCausedThisExceptionRegistered() {
		return causeIsRegistered;
	}

public:
	std::string toString() {
		std::string result;
		switch (status) {
		case RMAPEngineIsNotStarted:
			result = "RMAPEngineIsNotStarted";
			break;
		case TransactionIDIsNotAvailable:
			result = "TransactionIDIsNotAvailable";
			break;
		case TooManyConcurrentTransactions:
			result = "TooManyConcurrentTransactions";
			break;
		case SpecifiedTransactionIDIsAlreadyInUse:
			result = "SpecifiedTransactionIDIsAlreadyInUse";
			break;
		case PacketWasNotSentCorrectly:
			result = "PacketWasNotSentCorrectly";
			break;
		case SpaceWireIFDisconnected:
			result = "SpaceWireIFDisconnected";
			break;
		case UnexpectedRMAPReplyPacketWasReceived:
			result = "UnexpectedRMAPReplyPacketWasReceived";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

class RMAPEngine: public CxxUtilities::Thread {
public:
	class RMAPTargetProcessThread: public CxxUtilities::Thread {
	private:
		RMAPTargetAccessAction* rmapTargetAcessAction;
		RMAPTransaction rmapTransaction;
		RMAPEngine* rmapEngine;

	private:
		bool isCompleted_;

	public:
		RMAPTargetProcessThread(RMAPEngine* rmapEngine, RMAPTransaction rmapTransaction,
				RMAPTargetAccessAction* rmapTargetAcessAction) :
				CxxUtilities::Thread() {
			this->rmapEngine = rmapEngine;
			this->rmapTargetAcessAction = rmapTargetAcessAction;
			this->rmapTransaction = rmapTransaction;
			isCompleted_ = false;
		}

	public:
		void run() {
			using namespace std;
			isCompleted_ = false;
			try {
				rmapTargetAcessAction->processTransaction(&rmapTransaction);
				rmapTransaction.setState(RMAPTransaction::ReplySet);
			} catch (...) {
				delete rmapTransaction.commandPacket;
				rmapEngine->receivedCommandPacketDiscarded();
				isCompleted_ = true;
				return;
			}
			try {
				rmapTransaction.replyPacket->constructPacket();
				rmapEngine->sendPacket(rmapTransaction.replyPacket->getPacketBufferPointer());
				rmapTransaction.setState(RMAPTransaction::ReplySent);
			} catch (...) {
				rmapTargetAcessAction->transactionReplyCouldNotBeSent(&rmapTransaction);
				rmapEngine->replyToReceivedCommandPacketCouldNotBeSent();
				delete rmapTransaction.commandPacket;
				isCompleted_ = true;
				return;
			}
			rmapTargetAcessAction->transactionWillComplete(&rmapTransaction);
			rmapTransaction.setState(RMAPTransaction::ReplyCompleted);
			delete rmapTransaction.commandPacket;
			isCompleted_ = true;
		}

	public:
		bool isCompleted() {
			return isCompleted_;
		}
	};

public:
	class RMAPEngineSpaceWireIFActionCloseAction: public SpaceWireIFActionCloseAction {
	private:
		RMAPEngine* rmapEngine;

	public:
		RMAPEngineSpaceWireIFActionCloseAction(RMAPEngine* rmapEngine) {
			this->rmapEngine = rmapEngine;
		}

	public:
		void doAction(SpaceWireIF* spwif) {
			rmapEngine->stop();
		}
	};

private:
	std::map<uint16_t, RMAPTransaction*> transactions;
	CxxUtilities::Mutex transactionIDMutex;
	std::list<uint16_t> availableTransactionIDList;
	uint16_t latestAssignedTransactionID;

private:
	std::vector<RMAPTarget*> rmapTargets;
	std::vector<RMAPTargetProcessThread*> rmapTargetProcessThreads;

public:
	static const size_t MaximumTIDNumber = 65536;
	static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 200000; //200ms

private:
	SpaceWireIF* spwif;
	CxxUtilities::Mutex spwSendMutex;

private:
	RMAPEngineSpaceWireIFActionCloseAction* spacewireIFActionCloseAction;

public:
	bool stopped;
	bool hasStopped;
	CxxUtilities::Actions<void> rmapEngineStoppedActions;

public:
	size_t nDiscardedReceivedPackets;
	size_t nErrorneousReplyPackets;
	size_t nErrorneousCommandPackets;
	size_t nTransactionsAbortedWhenReplying;
	size_t nErrorInRMAPReplyPacketProcessing;

private:
	bool stopActionsHasBeenExecuted;

public:
	RMAPEngine() {
		spwif = NULL;
		initialize();
	}

public:
	RMAPEngine(SpaceWireIF* spwif) {
		initialize();
		this->setSpaceWireIF(spwif);
	}

public:
	~RMAPEngine() {
		if(spwif!=NULL){
			spwif->cancelReceive();
		}
	}

private:
	void initialize() {
		transactionIDMutex.lock();
		latestAssignedTransactionID = 0;
		for (size_t i = 0; i < MaximumTIDNumber; i++) {
			availableTransactionIDList.push_back(i);
		}
		transactionIDMutex.unlock();
		stopped = true;
		spacewireIFActionCloseAction = NULL;
		stopActionsHasBeenExecuted = false;
		useDraftECRC = false;
		//initialize counters
		initializeCounters();
	}

private:
	void initializeCounters() {
		nDiscardedReceivedPackets = 0;
		nErrorneousReplyPackets = 0;
		nErrorneousCommandPackets = 0;
		nTransactionsAbortedWhenReplying = 0;
		nErrorInRMAPReplyPacketProcessing = 0;
	}

public:
	void run() {
		using namespace std;
		stopped = false;
		hasStopped = false;
		stopActionsHasBeenExecuted = false;
		spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
		while (!stopped) {
			try {
				RMAPPacket* rmapPacket = receivePacket();
				if (rmapPacket == NULL) {
					//do nothing
				} else if (rmapPacket->isCommand()) {
					rmapCommandPacketReceived(rmapPacket);
				} else {
					rmapReplyPacketReceived(rmapPacket);
				}
			} catch (RMAPPacketException& e) {
				cerr << "RMAPEngine::run() got RMAPPacketException " << e.toString() << endl;
				break;
			} catch (RMAPEngineException& e) {
				cerr << "RMAPEngine::run() got RMAPEngineException " << e.toString() << endl;
				break;
			}
		}
		stopped = true;
		invokeRegisteredStopActions();
		hasStopped = true;
	}

public:
	void stop() {
		using namespace std;
		if (stopped == false) {
			stopped = true;
			spwif->cancelReceive();
			do {
				CxxUtilities::Condition c;
				c.wait(spwif->getTimeoutDurationInMicroSec() / 1000.0 /* in milli sec */);
			} while (hasStopped != true);
		}
	}

public:
	bool isStopped() {
		return stopped;
	}

public:
	bool isStarted() {
		if (stopped == false) {
			return true;
		} else {
			return false;
		}
	}

private:
	void rmapCommandPacketReceived(RMAPPacket* commandPacket) throw (RMAPEngineException) {
		using namespace std;
		//first cleanup completed threads
		std::vector<RMAPTargetProcessThread*> newRMAPTargetProcessThreads;
		for (size_t i = 0; i < rmapTargetProcessThreads.size(); i++) {
			if (rmapTargetProcessThreads[i]->isCompleted()) {
				delete rmapTargetProcessThreads[i];
			} else {
				newRMAPTargetProcessThreads.push_back(rmapTargetProcessThreads[i]);
			}
		}
		rmapTargetProcessThreads = newRMAPTargetProcessThreads;

		//find an RMAPTarget instance which can accept the accessed address range
		RMAPTransaction rmapTransaction;
		rmapTransaction.commandPacket = commandPacket;
		rmapTransaction.setState(RMAPTransaction::CommandPacketReceived);
		for (size_t i = 0; i < rmapTargets.size(); i++) {
			RMAPTargetAccessAction* rmapTargetAcessAction = rmapTargets[i]->getCorrespondingRMAPTargetAccessAction(
					&rmapTransaction);
			if (rmapTargetAcessAction != NULL) {
				RMAPTargetProcessThread* aThread = new RMAPTargetProcessThread(this, rmapTransaction, rmapTargetAcessAction);
				aThread->start();
				rmapTargetProcessThreads.push_back(aThread);
				return;
			}
		}
		delete commandPacket;
		receivedCommandPacketDiscarded();
	}

private:
	std::vector<RMAPPacket*> discardedRMAPReplyPackets;

private:
	size_t getNDiscardedRMAPReplyPackets(){
		return discardedRMAPReplyPackets.size();
	}

public:
	std::vector<RMAPPacket*>* getDiscardedRMAPReplyPackets(){
		return &discardedRMAPReplyPackets;
	}

private:
	CxxUtilities::Condition c;

private:
	void rmapReplyPacketReceived(RMAPPacket* packet) throw (RMAPEngineException) {
		using namespace std;
		try {
			//find a corresponding command packet
			RMAPTransaction* transaction;
			try {
				transaction = this->resolveTransaction(packet);
			} catch (RMAPEngineException& e) {
				//if not found, increment error counter
				nErrorneousReplyPackets++;
				discardedRMAPReplyPackets.push_back(packet);
				std::cerr << "RMAP Reply packet (dataLength="<< packet->getLength() <<  "bytes) was received but no corresponding transaction was found. The size of discardedRMAPReplyPackets = " << discardedRMAPReplyPackets.size() << std::endl;
				return;
			}
			//register reply packet to the resolved transaction
			transaction->replyPacket = packet;
			//update transaction state
			/*while(transaction->getState()!=RMAPTransaction::CommandSent and transaction->getState()!=RMAPTransaction::Initiated){
				cout << "RMAPEngine::rmapReplyPacketReceived(): Waiting" << endl;
				cout << transaction->getState() << endl;
				c.wait(100);
			}*/
			transaction->setState(RMAPTransaction::ReplyReceived);
			if (!transaction->isNonblockingMode) {
				transaction->getCondition()->signal();
			}
		} catch (CxxUtilities::MutexException& e) {
			std::cerr << "Fatal error in RMAPEngine::rmapReplyPacketReceived()... :-(" << std::endl;
			std::cerr << "RMAPEngine tries to recover normal operation, but may fail continuously." << std::endl;
			nErrorInRMAPReplyPacketProcessing++;
		} catch (...) {
			std::cerr << "Fatal error in RMAPEngine::rmapReplyPacketReceived()... :-(" << std::endl;
			std::cerr << "RMAPEngine tries to recover normal operation, but may fail continuously." << std::endl;
		}
	}

private:
	void receivedPacketDiscarded() {
		nDiscardedReceivedPackets++;
	}

protected:
	void receivedCommandPacketDiscarded() {
		nErrorneousCommandPackets++;
	}

protected:
	void replyToReceivedCommandPacketCouldNotBeSent() {
		nTransactionsAbortedWhenReplying++;
	}

private:
	bool useDraftECRC;

private:
	RMAPPacket* receivePacket() throw (RMAPEngineException) {
		using namespace std;
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>;
		try {
			spwif->receive(buffer);
		} catch (SpaceWireIFException& e) {
			delete buffer;
			//cout << e.toString() << endl;
			if (e.status == SpaceWireIFException::Disconnected) {
				//tell run() that SpaceWireIF is disconnected
				throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
			} else {
				if (e.status == SpaceWireIFException::Timeout) {
					//cout << "#receive timeout" << endl;
					return NULL;
				} else {
					//tell run() that SpaceWireIF is disconnected
					throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
				}
			}
		}
		RMAPPacket* packet = new RMAPPacket();
		if (!useDraftECRC) {
			packet->setUseDraftECRC(false);
		} else {
			packet->setUseDraftECRC(true);
		}
		try {
			packet->interpretAsAnRMAPPacket(buffer);
		} catch (RMAPPacketException& e) {
			delete packet;
			delete buffer;
			receivedPacketDiscarded();
			return NULL;
		}
		delete buffer;
		return packet;
	}

private:
	RMAPTransaction* resolveTransaction(RMAPPacket* packet) throw (RMAPEngineException) {
		using namespace std;
		transactionIDMutex.lock();
		uint16_t transactionID = packet->getTransactionID();
		if (isTransactionIDAvailable(transactionID) == true) { //if tid is not in use
			transactionIDMutex.unlock();
			throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived, packet);
		} else { //if tid is registered to tid db
			//resolve transaction
			RMAPTransaction* transaction = transactions[transactionID];
			//delete registered tid
			deleteTransactionIDFromDB(transactionID);
			//unlock mutex
			transactionIDMutex.unlock();
			//return resolved transaction
			return transaction;
		}
	}

public:
	inline void initiateTransaction(RMAPTransaction& transaction) throw (RMAPEngineException) {
		initiateTransaction(&transaction);
	}

public:
	void initiateTransaction(RMAPTransaction* transaction) throw (RMAPEngineException) {
		using namespace std;
		transaction->state = RMAPTransaction::NotInitiated;
		uint16_t transactionID;
		RMAPPacket* commandPacket = transaction->getCommandPacket();
		if (transaction->getTransactionIDMode() == RMAPTransaction::AutoTransactionID) {
			transactionID = getNextAvailableTransactionID();
		} else {
			//check if the TID specified in RMAPCommandPacket is
			//available or already used by another transaction
			transactionID = transaction->getTransactionID();
			if (isTransactionIDAvailable(transactionID) == false) {
				throw RMAPEngineException(RMAPEngineException::SpecifiedTransactionIDIsAlreadyInUse);
			}
		}
		//register the transaction to management list
		//if Reply is required
		if (transaction->commandPacket->isReplyFlagSet()) {
			transactionIDMutex.lock();
			transactions[transactionID] = transaction;
			transactionIDMutex.unlock();
		} else {
			//otherwise put back transaction Id to available id list
			pushBackUtilizedTransactionID(transactionID);
		}
		//send a command packet
		commandPacket->setTransactionID(transactionID);
		commandPacket->constructPacket();
		if (isStarted()) {
			sendPacket(commandPacket->getPacketBufferPointer());
			transaction->state = RMAPTransaction::Initiated;
		} else {
			throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
		}
	}

public:
	inline void deleteTransactionIDFromDB(uint16_t transactionID) {
		//remove tid from management list
		transactionIDMutex.lock();
		std::map<uint16_t, RMAPTransaction*>::iterator it = transactions.find(transactionID);
		if (it != transactions.end()) { //found
			transactions.erase(it);
			//put back the transaction id to the available list
			pushBackUtilizedTransactionID(transactionID);
		}
		transactionIDMutex.unlock();
	}

public:
	void cancelTransaction(RMAPTransaction* transaction) throw (RMAPEngineException) {
		using namespace std;
		RMAPPacket* commandPacket = transaction->getCommandPacket();
		uint16_t transactionID = commandPacket->getTransactionID();
		deleteTransactionIDFromDB(transactionID);
	}

public:
	void sendPacket(std::vector<uint8_t>* bytes) {
		using namespace std;
		spwSendMutex.lock();
		try {
			spwif->send(bytes);
		} catch (...) {
			spwSendMutex.unlock();
			throw RMAPEngineException(RMAPEngineException::PacketWasNotSentCorrectly);
		}
		spwSendMutex.unlock();

	}

public:
	void setSpaceWireIF(SpaceWireIF* spwif) {
		using namespace std;
		this->spwif = spwif;
		if (spacewireIFActionCloseAction == NULL) {
			spacewireIFActionCloseAction = new RMAPEngineSpaceWireIFActionCloseAction(this);
		}
		this->spwif->addSpaceWireIFCloseAction(spacewireIFActionCloseAction);
	}

	SpaceWireIF * getSpaceWireIF() {
		return spwif;
	}

private:
	uint16_t getNextAvailableTransactionID() throw (RMAPEngineException) {
		transactionIDMutex.lock();
		if (availableTransactionIDList.size() != 0) {
			unsigned int tid = *(availableTransactionIDList.begin());
			availableTransactionIDList.pop_front();
			transactionIDMutex.unlock();
			return tid;
		} else {
			transactionIDMutex.unlock();
			throw RMAPEngineException(RMAPEngineException::TooManyConcurrentTransactions);
		}
	}

private:
	void pushBackUtilizedTransactionID(uint16_t transactionID) {
		transactionIDMutex.lock();
		availableTransactionIDList.push_back(transactionID);
		transactionIDMutex.unlock();
	}

public:
	bool isTransactionIDAvailable(uint16_t transactionID) {
		transactionIDMutex.lock();
		if (transactions.find(transactionID) != transactions.end()) {
			transactionIDMutex.unlock();
			//already in use
			return false;
		} else {
			transactionIDMutex.unlock();
			//not used
			return true;
		}
	}

public:
	void addRMAPTarget(RMAPTarget* rmapTarget) {
		rmapTargets.push_back(rmapTarget);
	}

public:
	void removeRMAPTarget(RMAPTarget* rmapTarget) {
		std::vector<RMAPTarget*> aVector;
		for (size_t i = 0; i < rmapTargets.size(); i++) {
			if (rmapTargets[i] != rmapTarget) {
				aVector.push_back(rmapTargets[i]);
			}
		}
		rmapTargets = aVector;
	}

public:
	void addRMAPEngineStoppedAction(RMAPEngineStoppedAction* rmapEngineStoppedAction) {
		rmapEngineStoppedActions.addAction(rmapEngineStoppedAction);
	}

public:
	void removeRMAPEngineStoppedAction(RMAPEngineStoppedAction* rmapEngineStoppedAction) {
		rmapEngineStoppedActions.removeAction(rmapEngineStoppedAction);
	}

public:
	CxxUtilities::Actions<void>* getRMAPEngineStoppedActions() {
		return &rmapEngineStoppedActions;
	}

private:
	void invokeRegisteredStopActions() {
		using namespace std;
		if (stopActionsHasBeenExecuted == false) {
			rmapEngineStoppedActions.doEachAction((void*) this);
			stopActionsHasBeenExecuted = true;
		}
	}

public:
	bool isUseDraftECRC() const {
		return useDraftECRC;
	}

public:
	void setUseDraftECRC(bool useDraftEcrc = true) {
		this->useDraftECRC = useDraftEcrc;
	}

public:
	size_t getNTransactions() {
		return transactions.size();
	}

public:
	size_t getNAvailableTransactionIDs() {
		return availableTransactionIDList.size();
	}

};

#endif /* RMAPENGINE_HH_ */
