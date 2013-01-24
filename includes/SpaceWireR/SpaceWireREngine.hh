/*
 * SpaceWireREngine.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERENGINE_HH_
#define SPACEWIRERENGINE_HH_

#include "CxxUtilities/Thread.hh"
/*
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
*/

class SpaceWireREngineException: public CxxUtilities::Exception {
public:
	SpaceWireREngineException(int status) :
			CxxUtilities::Exception(status) {

	}

public:
	virtual ~SpaceWireREngineException() {
	}

public:
	enum {
		SpaceWireREngineIsNotRunning
	};

public:
	std::string toString() {
		std::string str;
		switch (status) {
		case SpaceWireREngineIsNotRunning:
			str = "SpaceWireREngineIsNotRunning";
			break;
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

private:
	std::map<uint8_t, std::list<SpaceWireRPacket*>*> receiveTEPPacketListMap;
	std::map<uint8_t, CxxUtilities::Condition*> receiveTEPNotificationMap;

private:
	std::map<uint8_t, std::list<SpaceWireRPacket*>*> transmitTEPPacketListMap;
	std::map<uint8_t, CxxUtilities::Condition*> transmitTEPNotificationMap;

private:
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
		nSentPackets = 0;
		nReceivedPackets = 0;
	}

public:
	static const double DefaultReceiveTimeoutDurationInMicroSec = 1000000;

public:
	void processReceivedSpaceWireRPacket(SpaceWireRPacket* packet) throw (SpaceWireREngineException) {
		uint8_t channel = packet->getChannelNumber();
		std::map<uint8_t, std::list<SpaceWireRPacket*>*>::iterator it_find = receiveTEPPacketListMap.find(channel);
		if (it_find != receiveTEPPacketListMap.end()) {
			//if there is ReceiveTEP corresponding to the channel number in the received packet.
			std::list<SpaceWireRPacket*>* packetList = it_find->second;
			packetList->push_back(packet); //pass the received packet to the ReceiveTEP
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
	void sendPacket(SpaceWireRPacket* packet) throw (SpaceWireREngineException) {
		if (this->isStopped()) {
			throw SpaceWireREngineException(SpaceWireREngineException::SpaceWireREngineIsNotRunning);
		}
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
	void registerReceiveTEP(uint8_t channel, std::list<SpaceWireRPacket*>* packetList,
			CxxUtilities::Condition* notifier) {
		receiveTEPPacketListMap[channel] = packetList;
		receiveTEPNotificationMap[channel] = notifier;
	}

public:
	void unregisterReceiveTEP(uint8_t channel) {
		if (receiveTEPNotificationMap.find(channel) != receiveTEPNotificationMap.end()) {
			receiveTEPNotificationMap.erase(receiveTEPNotificationMap.find(channel));
			receiveTEPPacketListMap.erase(receiveTEPPacketListMap.find(channel));
		}
	}

public:
	void registerTransmitTEP(uint8_t channel, std::list<SpaceWireRPacket*>* packetList,
			CxxUtilities::Condition* notifier) {
		transmitTEPPacketListMap[channel] = packetList;
		transmitTEPNotificationMap[channel] = notifier;
	}

public:
	void unregisterTransmitTEP(uint8_t channel) {
		if (transmitTEPNotificationMap.find(channel) != transmitTEPNotificationMap.end()) {
			transmitTEPNotificationMap.erase(transmitTEPNotificationMap.find(channel));
			transmitTEPPacketListMap.erase(receiveTEPPacketListMap.find(channel));
		}
	}

public:
	void run() {
		using namespace std;
		spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
		std::vector<uint8_t>* data;
		SpaceWireRPacket* packet;
		_SpaceWireREngine_run_loop: while (!stopped) {
			try {
				data = spwif->receive();
				nReceivedPackets++;
				packet = new SpaceWireRPacket();
				packet->interpretPacket(data);
				processReceivedSpaceWireRPacket(packet);
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
	size_t getNSentPackets() {
		return nSentPackets;
	}

public:
	size_t getNReceivedPackets() {
		return nReceivedPackets;
	}
};

#endif /* SPACEWIRERENGINE_HH_ */
