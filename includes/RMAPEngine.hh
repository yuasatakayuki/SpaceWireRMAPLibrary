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

#include "RMAPTransaction.hh"
#include "SpaceWireIF.hh"

class RMAPEngineException: public CxxUtilities::Exception {
public:
	enum {
		TransactionIDIsNotAvailable,
		TooManyConcurrentTransactions,
		SpecifiedTransactionIDIsAlreadyInUse,
		CommandPacketWasNotSentCorrectly,
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
private:
	std::map<uint16_t, RMAPTransaction*> transactions;
	CxxUtilities::Mutex transactionIDMutex;
	std::list<uint16_t> availableTransactionIDList;
	uint16_t latestAssignedTransactionID;

public:
	static const size_t MaximumTIDNumber = 65536;

private:
	SpaceWireIF* spwif;

public:
	bool stopped;

private:
	size_t nDiscardedReceivedPackets;
	size_t nErrorneousReplyPackets;

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

	void initializeCounters(){
		nDiscardedReceivedPackets=0;
		nErrorneousReplyPackets=0;
	}

public:
	void run() {
		using namespace std;
		stopped = false;
		while (!stopped) {
			RMAPPacket* rmapPacket = receivePacket();
			if (rmapPacket == NULL) {
				//do nothing
			} else if (rmapPacket->isCommand()) {
				rmapCommandPacketReceived(rmapPacket);
			} else {
				rmapReplyPacketReceived(rmapPacket);
			}
		}
		stopped = true;
	}

public:
	void stop() {
		stopped = true;
	}

	bool isStopped(){
		return stopped;
	}

private:
	void rmapCommandPacketReceived(RMAPPacket* packet) {
		throw 1;
	}

	void rmapReplyPacketReceived(RMAPPacket* packet) {
		//find a corresponding command packet
		RMAPTransaction* transaction;
		try{
			transaction=this->resolveTransaction(packet);
		}catch(RMAPEngineException e){
			//if not found, increment error counter
			nErrorneousReplyPackets++;
			return;
		}
		transaction->replyPacket=packet;
		transaction->setState(RMAPTransaction::ReplyReceived);
		transaction->getCondition()->signal();
	}

	void receivedPacketDiscarded(){
		nDiscardedReceivedPackets++;
	}

	RMAPPacket* receivePacket() throw (RMAPEngineException) {
		using namespace std;
		std::vector<uint8_t>* buffer=new std::vector<uint8_t>;
		try {
			spwif->receive(buffer);
		} catch (SpaceWireIFException e) {
			if (e.status == SpaceWireIFException::Disconnected) {
				//stop RMAPEngine
				this->stop();
				//tell user application that SpaceWireIF is disconnected
				throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
			} else {
				if(e.status == SpaceWireIFException::Timeout) {
					//cout << "#receive timeout" << endl;
				}else{
					//cout << "#receive failed" << endl;
				}
				return NULL;
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
			transaction->state=RMAPTransaction::ReplyReceived;
			transaction->getCondition()->signal();
			return transaction;
		}
		transactionIDMutex.unlock();
	}

public:
	inline void initiateTransaction(RMAPTransaction& transaction) {
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
		try {
			spwif->send(commandPacket->getPacketBufferPointer());
			transaction->state=RMAPTransaction::Initiated;
//			cout << "# sent" << endl;
//			cout << "#" << commandPacket->toString() << endl;
		} catch (...) {
			throw RMAPEngineException(RMAPEngineException::CommandPacketWasNotSentCorrectly);
		}
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
};

#endif /* RMAPENGINE_HH_ */
