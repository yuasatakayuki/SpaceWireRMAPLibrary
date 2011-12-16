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

class RMAPEngineStoppedAction : public CxxUtilities::Action {
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
	RMAPEngineException(uint32_t status, RMAPPacket* packetCausedThisException) :
		CxxUtilities::Exception(status) {
		this->rmapPacketCausedThisException = packetCausedThisException;
		causeIsRegistered = true;
	}

public:
	RMAPPacket* getRMAPPacketCausedThisException() {
		return rmapPacketCausedThisException;
	}

	bool isRMAPPacketCausedThisExceptionRegistered() {
		return causeIsRegistered;
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
				SpaceWireUtilities::dumpPacket(&cout,rmapTransaction.replyPacket->getPacketBufferPointer());
				rmapTransaction.replyPacket->constructPacket();
				cout << "$2 size=" << rmapTransaction.replyPacket->getPacketBufferPointer()->size() << endl;
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

private:
	SpaceWireIF* spwif;
	CxxUtilities::Mutex spwSendMutex;

public:
	bool stopped;
	CxxUtilities::Actions rmapEngineStoppedActions;

private:
	size_t nDiscardedReceivedPackets;
	size_t nErrorneousReplyPackets;
	size_t nErrorneousCommandPackets;
	size_t nTransactionsAbortedWhenReplying;

public:
	RMAPEngine() {
		spwif = NULL;
		initialize();
	}

	RMAPEngine(SpaceWireIF* spwif) {
		this->spwif = spwif;
		initialize();
	}

	~RMAPEngine() {

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
		//initialize counters
		initializeCounters();
	}

	void initializeCounters() {
		nDiscardedReceivedPackets = 0;
		nErrorneousReplyPackets = 0;
		nErrorneousCommandPackets = 0;
		nTransactionsAbortedWhenReplying = 0;
	}

public:
	void run() {
		using namespace std;
		stopped = false;
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
			} catch (...) {
				break;
			}
		}
		stopped = true;
		invokeRegisteredStopActions();
	}

public:
	void stop() {
		stopped = true;
	}

	bool isStopped() {
		return stopped;
	}

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
				RMAPTargetProcessThread* aThread = new RMAPTargetProcessThread(this, rmapTransaction,
						rmapTargetAcessAction);
				aThread->start();
				rmapTargetProcessThreads.push_back(aThread);
				return;
			}
		}
		delete commandPacket;
		receivedCommandPacketDiscarded();
	}

	void rmapReplyPacketReceived(RMAPPacket* packet) throw (RMAPEngineException) {
		//find a corresponding command packet
		RMAPTransaction* transaction;
		try {
			transaction = this->resolveTransaction(packet);
		} catch (RMAPEngineException e) {
			//if not found, increment error counter
			nErrorneousReplyPackets++;
			return;
		}
		transaction->replyPacket = packet;
		transaction->setState(RMAPTransaction::ReplyReceived);
		transaction->getCondition()->signal();
	}

	void receivedPacketDiscarded() {
		nDiscardedReceivedPackets++;
	}

protected:
	void receivedCommandPacketDiscarded() {
		nErrorneousCommandPackets++;
	}

	void replyToReceivedCommandPacketCouldNotBeSent() {
		nTransactionsAbortedWhenReplying++;
	}

private:
	RMAPPacket* receivePacket() throw (RMAPEngineException) {
		using namespace std;
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>;
		try {
			spwif->receive(buffer);
		} catch (SpaceWireIFException e) {
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
		try {
			packet->interpretAsAnRMAPPacket(buffer);
		} catch (RMAPPacketException e) {
			delete packet;
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
		if (isTransactionIDAvailable(transactionID) == true) {
			throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived, packet);
		} else {
			transactionIDMutex.unlock();
			RMAPTransaction* transaction = transactions[transactionID];
			transaction->setReplyPacket(packet);
			transaction->state = RMAPTransaction::ReplyReceived;
			transaction->getCondition()->signal();
			return transaction;
		}
		transactionIDMutex.unlock();
	}

public:
	inline void initiateTransaction(RMAPTransaction& transaction) throw (RMAPEngineException) {
		initiateTransaction(&transaction);
	}

	void initiateTransaction(RMAPTransaction* transaction) throw (RMAPEngineException) {
		using namespace std;
		uint16_t transactionID;
		RMAPPacket* commandPacket;
		if (transaction->getTransactionIDMode() == RMAPTransaction::AutoTransactionID) {
			transactionID = getNextAvailableTransactionID();
			commandPacket = transaction->getCommandPacket();
			commandPacket->setTransactionID(transactionID);
		} else {
			//check if the TID specified in RMAPCommandPacket is
			//available or already used by another transaction
			transactionID = transaction->getCommandPacket()->getTransactionID();
			if (isTransactionIDAvailable(transactionID) == false) {
				throw RMAPEngineException(RMAPEngineException::SpecifiedTransactionIDIsAlreadyInUse);
			}
		}
		//register the transaction to management list
		transactionIDMutex.lock();
		transactions[transactionID] = transaction;
		transactionIDMutex.unlock();
		//send a command packet
		commandPacket->constructPacket();
		if (isStarted()) {
			sendPacket(commandPacket->getPacketBufferPointer());
			transaction->state = RMAPTransaction::Initiated;
		} else {
			throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
		}
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
		this->spwif = spwif;
	}

	SpaceWireIF* getSpaceWireIF() {
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
	void addRMAPEngineStoppedAction(RMAPEngineStoppedAction* rmapEngineStoppedAction){
		rmapEngineStoppedActions.addAction(rmapEngineStoppedAction);
	}

	void removeRMAPEngineStoppedAction(RMAPEngineStoppedAction* rmapEngineStoppedAction){
		rmapEngineStoppedActions.removeAction(rmapEngineStoppedAction);
	}

	CxxUtilities::Actions* getRMAPEngineStoppedActions(){
		return &rmapEngineStoppedActions;
	}

private:
	void invokeRegisteredStopActions(){
		rmapEngineStoppedActions.doEachAction(this);
	}

};


#endif /* RMAPENGINE_HH_ */
