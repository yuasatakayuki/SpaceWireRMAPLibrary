/*
 * SpaceWireREngine.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERENGINE_HH_
#define SPACEWIRERENGINE_HH_

#include "CxxUtilities/Thread.hh"

class SpaceWireRTEP {
private:
	uint8_t channelNumber;

public:
	uint8_t getChannelNumber() {
		return channelNumber;
	}

public:
	void setChannelNumber(uint8_t channelNumber) {
		this->channelNumber = channelNumber;
	}
};

class SpaceWireREngineException: public CxxUtilities::Exception {
public:
	SpaceWireREngineException(int status) :
			CxxUtilities::Exception(status) {

	}

public:
	~SpaceWireREngineException() {
	}

public:
	enum {

	};

public:
	std::string toString() {
		std::string str;
		switch (status) {
		default:
			str = "";
			break;
		}
		return str;
	}
};

class SpaceWireREngine: public CxxUtilities::StoppableThread {
private:
	SpaceWireIF* spwif;
	CxxUtilities::Condition stopCondition;
	std::map<uint8_t, SpaceWireRPacket**> receiveTEPPacketPointerMap;
	std::map<uint8_t, CxxUtilities::Condition*> receiveTEPNotificationMap;
	size_t nDiscardedReceivedPackets;
	size_t nSentPackets;
	size_t nReceivedPackets;
	CxxUtilities::Mutex sendMutex;

private:
	static const double TimeoutDurationForStopCondition = 1000;

public:
	SpaceWireREngine(SpaceWireIF* spwif) {
		this->spwif = spwif;
		nDiscardedReceivedPackets = 0;
		nSentPackets=0;
		nReceivedPackets=0;
	}

public:
	static const double DefaultReceiveTimeoutDurationInMicroSec = 1000000;

public:
	void processReceivedSpaceWireRPacket(SpaceWireRPacket* packet) throw (SpaceWireREngineException) {
		uint8_t channel = packet->getChannelNumber();
		std::map<uint8_t, SpaceWireRPacket**>::iterator it_find = receiveTEPPacketPointerMap.find(channel);
		if (it_find != receiveTEPPacketPointerMap.end()) {
			//if there is ReceiveTEP corresponding to the channel number in the received packet.
			SpaceWireRPacket** packetPointer = it_find->second;
			(*packetPointer) = packet; //pass the received packet to the ReceiveTEP
			//tell the ReceiveTEP that a packet has arrived.
			receiveTEPNotificationMap[channel]->signal();
		} else {
			//if there is no ReceiveTEP to receive the packet.
			//discard the packet
			nDiscardedReceivedPackets++;
			delete packet;
		}
	}

public:
	void sendPacket(SpaceWireRPacket* packet) {
		sendMutex.lock();
		try {
			spwif->send(packet->getPacketBufferPointer());
			nSentPackets++;
		} catch (...) {
			sendMutex.unlock();
			//todo
			using namespace std;
			cerr << "SpaceWireREngine::sendPacket() fatal error with SpaceWireIF. SpaceWireREngine will stop." << endl;
			this->stop();
		}
		sendMutex.unlock();
	}

public:
	void registerReceiveTEP(uint8_t channel, SpaceWireRPacket** packetPointer, CxxUtilities::Condition* notifier) {
		receiveTEPPacketPointerMap[channel] = packetPointer;
		receiveTEPNotificationMap[channel] = notifier;
	}

public:
	void run() {
		using namespace std;
		spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
		std::vector<uint8_t>* data;
		SpaceWireRPacket* packet;
		_SpaceWireREngine_run_loop:
		while (!stopped) {
			try {
				data = spwif->receive();
				nReceivedPackets++;
				packet = new SpaceWireRPacket();
				packet->interpretPacket(data);
				delete data;
			} catch (SpaceWireIFException& e) {
				//todo
				cerr << "SpaceWireREngine::run() got SpaceWireIF " << e.toString() << endl;
				break;
			} catch (SpaceWireRPacketException& e) {
				//todo
				delete data;
				delete packet;
				cerr << "SpaceWireREngine::run() got SpaceWireRPacketException" << e.toString() << endl;
				goto _SpaceWireREngine_run_loop;
			} catch (...) {
				//todo
				break;
			}

		}
		stopped = true;
		//todo
		//invokeRegisteredStopActions();
	}

public:
	void stop() {
		//stop this thread
		stopCondition.signal();
	}

public:
	SpaceWireIF* getSpaceWireIF() {
		return spwif;
	}

public:
	size_t getNDiscardedReceivedPackets() {
		return nDiscardedReceivedPackets;
	}

public:
	size_t getNSentPackets(){
		return nSentPackets;
	}

public:
	size_t getNReceivedPackets(){
		return nReceivedPackets;
	}
};

#endif /* SPACEWIRERENGINE_HH_ */
