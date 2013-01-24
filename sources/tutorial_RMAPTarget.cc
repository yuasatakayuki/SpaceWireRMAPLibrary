/*
 * tutorial_RMAPTarget.cc
 *
 *  Created on: Jan 24, 2013
 *      Author: yuasa
 */

#ifndef TUTORIAL_RMAPTARGET_CC_
#define TUTORIAL_RMAPTARGET_CC_

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWire.hh"
#include "RMAP.hh"

class RMAPTargetAccessActionImplementation: public RMAPTargetAccessAction {
private:
	uint8_t logicalAddress;
	uint8_t key;
	std::vector<uint8_t> replyData;

public:
	RMAPTargetAccessActionImplementation(uint8_t logicalAddress, uint8_t key, std::vector<uint8_t> replyData) {
		this->logicalAddress = logicalAddress;
		this->key = key;
		this->replyData = replyData;
	}

	virtual ~RMAPTargetAccessActionImplementation() {
	}

public:
	void processTransaction(RMAPTransaction* rmapTransaction) throw (RMAPTargetAccessActionException) {
		using namespace std;
		uint8_t logicalAddress = rmapTransaction->getCommandPacket()->getTargetLogicalAddress();
		uint8_t key = rmapTransaction->getCommandPacket()->getKey();
		uint16_t length = rmapTransaction->getCommandPacket()->getLength();

		//check logical address
		if (logicalAddress != this->logicalAddress) {
			cout << "Invalid Logical Address" << endl;
			this->setReplyWithStatus(rmapTransaction, RMAPReplyStatus::InvalidTargetLogicalAddress);
			return;
		}

		//check key
		if (key != this->key) {
			cout << "Invalid Key!" << endl;
			this->setReplyWithStatus(rmapTransaction, RMAPReplyStatus::InvalidDestinationKey);
			return;
		}

		//check length
		if (length != replyData.size()) {
			cout << "Invalid access Length!" << endl;
			this->setReplyWithStatus(rmapTransaction, RMAPReplyStatus::CommandNotImplementedOrNotAuthorized);
			return;
		}

		//if logical address and key are valid,
		//set reply data.
		cout << "Transaction successfully completed." << endl;
		this->setReplyWithDataWithStatus(rmapTransaction, &replyData, RMAPReplyStatus::CommandExcecutedSuccessfully);
		return;
	}
};

class TargetSimulator: public CxxUtilities::StoppableThread,
		public RMAPTarget,
		public SpaceWireIFActionCloseAction,
		public RMAPEngineStoppedAction,
		public SpaceWireIFActionTimecodeScynchronizedAction {

private:
	SpaceWireIFOverTCP* spwif;
	RMAPEngine* rmapEngine;

private:
	static const size_t nRMAPTargets = 3;
	RMAPTargetAccessActionImplementation* rmapTargetAccessAction0;
	RMAPTargetAccessActionImplementation* rmapTargetAccessAction1;
	RMAPTargetAccessActionImplementation* rmapTargetAccessAction2;
	RMAPAddressRange* addressRange0;
	RMAPAddressRange* addressRange1;
	RMAPAddressRange* addressRange2;

private:
	static const uint32_t tcpPort = 10030;

public:
	TargetSimulator() {
		spwif = new SpaceWireIFOverTCP(tcpPort);
		spwif->addSpaceWireIFCloseAction(this);
		spwif->addTimecodeAction(this);
		rmapEngine = new RMAPEngine(spwif);

		//RMAP Target parameters

		uint8_t logicalAddresses[] = { 0xFE, 0x30, 0xAB };
		uint8_t keys[] = { 0x00, 0x12, 0x55 };
		uint32_t addresses[] = { 0x00000000, 0x12345678, 0xaBadCafe };

		//prepare data for RMAPTargetNode 0
		std::vector<uint8_t> data0;
		for (size_t i = 0; i < 16; i++) {
			data0.push_back(i);
		}

		//prepare data for RMAPTargetNode 1
		std::vector<uint8_t> data1;
		data1.push_back(0xAB);
		data1.push_back(0xAD);
		data1.push_back(0xCA);
		data1.push_back(0xFE);

		//prepare data for RMAPTargetNode 2
		std::vector<uint8_t> data2;
		data2.push_back(0xDE);
		data2.push_back(0xAD);

		//Construct instances
		rmapTargetAccessAction0 = new RMAPTargetAccessActionImplementation(logicalAddresses[0], keys[0], data0);
		addressRange0 = new RMAPAddressRange(addresses[0], addresses[0]+data0.size());
		rmapTargetAccessAction1 = new RMAPTargetAccessActionImplementation(logicalAddresses[1], keys[1], data1);
		addressRange1 = new RMAPAddressRange(addresses[1], addresses[1]+data1.size());
		rmapTargetAccessAction2 = new RMAPTargetAccessActionImplementation(logicalAddresses[2], keys[2], data2);
		addressRange2 = new RMAPAddressRange(addresses[2], addresses[2]+data2.size());

		//register instances to this (via the RMAPTarget::addAddressRangeAndAssociatedAction() method)
		this->addAddressRangeAndAssociatedAction(addressRange0, rmapTargetAccessAction0);
		this->addAddressRangeAndAssociatedAction(addressRange1, rmapTargetAccessAction1);
		this->addAddressRangeAndAssociatedAction(addressRange2, rmapTargetAccessAction2);

		//add "this" (as an RMAPTarget instance) to RMAPEngine
		rmapEngine->addRMAPTarget(this);

		//add "this" (as an RMAPEngineStoppedAction instance) to RMAPEngine
		rmapEngine->addRMAPEngineStoppedAction((RMAPEngineStoppedAction*) this);
	}

public:
	virtual ~TargetSimulator() {
		rmapEngine->stop();
		delete rmapEngine;
		spwif->close();
		delete spwif;

		delete rmapTargetAccessAction0;
		delete rmapTargetAccessAction1;
		delete rmapTargetAccessAction2;
		delete addressRange0;
		delete addressRange1;
		delete addressRange2;
	}

public:
	CxxUtilities::Condition condition;

private:
	bool isConnected;

public:
	//invoked when SpaceWireIF is disconnected.
	void doAction(SpaceWireIF* spacewireIF) {
		isConnected = false;
		condition.signal();
		using namespace std;
		cout << "SpaceWireIF closed" << endl;
	}

public:
	//invoked when RMAPEngine is stopped.
	virtual void doAction(void* rmapEngine) {
		using namespace std;
		cout << "RMAPEngine stopped" << endl;
		spwif->close();
	}

public:
	//invoked when a timecode is received
	void doAction(unsigned char timecodeValue) {
		using namespace std;
		if (timecodeValue == 0x00) {
			cout << CxxUtilities::Time::getCurrentTimeAsString() << " Timecode 0 received" << endl;
		}
	}

public:
	void run() {
		using namespace CxxUtilities;
		using namespace std;
		while (!stopped) {
			if (!waitForAConnection()) {
				return;
			}
			condition.wait();
		}
	}

public:
	bool waitForAConnection() {
		isConnected = false;
		using namespace CxxUtilities;
		using namespace std;
		cout << "Waiting for a connection." << endl;
		_waitForAConnection_loop: //
		try {
			spwif->open();
			rmapEngine->start();
			cout << "Connected" << endl;
			isConnected = true;
			return true;
		} catch (SpaceWireIFException& e) {
			if (e.getStatus() == SpaceWireIFException::Timeout) {
				goto _waitForAConnection_loop;
			} else {
				this->stop();
				return false;
			}
		}
		return false;
	}

};

int main(int argc, char* argv[]) {
	TargetSimulator* targetSimulator = new TargetSimulator();
	targetSimulator->start();

	CxxUtilities::Condition c;
	c.wait();
}

#endif /* TUTORIAL_RMAPTARGET_CC_ */
