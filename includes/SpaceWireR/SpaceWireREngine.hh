/*
 * SpaceWireREngine.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERENGINE_HH_
#define SPACEWIRERENGINE_HH_

#include "CxxUtilities/Thread.hh"
#include "SpaceWireR/SpaceWireRPacket.hh"
#include "SpaceWireR/SpaceWireRClassInterfaces.hh"

#undef SpaceWireREngineDumpPacket
#undef DebugSpaceWireREngine

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
		SpaceWireREngineIsNotRunning, SpaceWireIFIsNotWorking
	};

public:
	std::string toString() {
		std::string str;
		switch (status) {
		case SpaceWireREngineIsNotRunning:
			str = "SpaceWireREngineIsNotRunning";
			break;
		case SpaceWireIFIsNotWorking:
			str = "SpaceWireIFIsNotWorking";
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
	/*
	 std::map<uint8_t, std::list<SpaceWireRPacket*>*> receiveTEPPacketListMap;
	 std::map<uint8_t, CxxUtilities::Condition*> receiveTEPNotificationMap;
	 */
	std::map<uint8_t, SpaceWireRTEPInterface*> receiveTEPs;

private:
	/*
	 std::map<uint8_t, std::list<SpaceWireRPacket*>*> transmitTEPPacketListMap;
	 std::map<uint8_t, CxxUtilities::Condition*> transmitTEPNotificationMap;
	 */
	std::map<uint8_t, SpaceWireRTEPInterface*> transmitTEPs;

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
		using namespace std;
#ifdef DebugSpaceWireREngine
		cout << "SpaceWireREngine::processReceivedSpaceWireRPacket()" << endl;
#endif
		uint8_t channel = packet->getChannelNumber();
		if (!packet->isAckPacket()) { //command/data packet
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is Command/Data packet." << endl;
#endif
			std::map<uint8_t, SpaceWireRTEPInterface*>::iterator it_find = receiveTEPs.find(channel);
			if (it_find != receiveTEPs.end()) {
				//if there is ReceiveTEP corresponding to the channel number in the received packet.
				it_find->second->receivedPackets.push_back(packet); //pass the received packet to the ReceiveTEP
				//tell the ReceiveTEP that a packet has arrived.
				it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to " << "0x" << hex << right << setw(8)
						<< setfill('0') << (uint64_t) it_find->second << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
			} else {
				//if there is no ReceiveTEP to receive the packet.
				//discard the packet
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
				nDiscardedReceivedPackets++;
				delete packet;
			}
		} else { //ack packet
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is Ack packet." << endl;
#endif
			std::map<uint8_t, SpaceWireRTEPInterface*>::iterator it_find = transmitTEPs.find(channel);
			if (it_find != transmitTEPs.end()) {
				//if there is TransmitTEP corresponding to the channel number in the received packet.
				it_find->second->receivedPackets.push_back(packet); //pass the received packet to the ReceiveTEP
				//tell the ReceiveTEP that a packet has arrived.
				it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to " << "0x" << hex << right << setw(8)
						<< setfill('0') << (uint64_t) it_find->second << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
			} else {
				//if there is no ReceiveTEP to receive the packet.
				//discard the packet
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
				nDiscardedReceivedPackets++;
				delete packet;
			}
		}
	}

public:
	void sendPacket(SpaceWireRPacket* packet) throw (SpaceWireREngineException) {
		using namespace std;
		if (this->isStopped()) {
			throw SpaceWireREngineException(SpaceWireREngineException::SpaceWireREngineIsNotRunning);
		}
		sendMutex.lock();
		try {
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::sendPacket() sending packet." << endl;
#endif
			spwif->send(packet->getPacketBufferPointer());
#ifdef SpaceWireREngineDumpPacket
			SpaceWireUtilities::dumpPacket(packet->getPacketBufferPointer());
#endif
			nSentPackets++;
		} catch (...) {
			sendMutex.unlock();
			using namespace std;
			cerr << "SpaceWireREngine::sendPacket() fatal error with SpaceWireIF. SpaceWireREngine will stop." << endl;
			this->stop();
			throw SpaceWireREngineException(SpaceWireREngineException::SpaceWireIFIsNotWorking);
		}
		sendMutex.unlock();
	}

public:
	void registerReceiveTEP(SpaceWireRTEPInterface* instance) {
		receiveTEPs[instance->channel]=instance;
		using namespace std;
#ifdef DebugSpaceWireREngine
		cout << "SpaceWireREngine::registerReceiveTEP() channel=" << (uint32_t) instance->channel
				<< " receiveTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
	}

public:
	void unregisterReceiveTEP(uint8_t channel) {
		if (receiveTEPs.find(channel) != receiveTEPs.end()) {
			receiveTEPs.erase(receiveTEPs.find(channel));
		}
	}

public:
	void registerTransmitTEP(SpaceWireRTEPInterface* instance) {
		transmitTEPs[instance->channel] = instance;
#ifdef DebugSpaceWireREngine
		using namespace std;
		cout << "SpaceWireREngine::registerTransmitTEP() channel=" << (uint32_t) instance->channel
				<< " transmitTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
	}

public:
	void unregisterTransmitTEP(uint8_t channel) {
		if (transmitTEPs.find(channel) != transmitTEPs.end()) {
			transmitTEPs.erase(transmitTEPs.find(channel));
		}
	}

public:
	void tellDisconnectionToAllTEPs() {
		for (size_t i = 0; i < receiveTEPs.size(); i++) {
			receiveTEPs[i]->closeDueToSpaceWireIFFailure();
		}
		for (size_t i = 0; i < transmitTEPs.size(); i++) {
			transmitTEPs[i]->closeDueToSpaceWireIFFailure();
		}
	}

public:
	void run() {
		using namespace std;
		spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
		std::vector<uint8_t>* data;
		SpaceWireRPacket* packet;
		_SpaceWireREngine_run_loop: //
		while (!stopped) {
			try {
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::run() Waiting for a packet to be received." << endl;
#endif
				data = spwif->receive();
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::run() A packet was received." << endl;
#endif
				nReceivedPackets++;
				packet = new SpaceWireRPacket;
				packet->interpretPacket(data);
#ifdef DebugSpaceWireREngine
				cout << "SpaceWireREngine::run() Packet was successfully interpreted. ChannelID="
						<< (uint32_t) packet->getChannelNumber() << endl;
#endif
				processReceivedSpaceWireRPacket(packet);
				delete data;
			} catch (SpaceWireIFException& e) {
				//todo
#ifdef DebugSpaceWireREngine
				cerr << "SpaceWireREngine::run() got SpaceWireIF " << e.toString() << endl;
#endif
				if (e.getStatus() == SpaceWireIFException::Disconnected) {
					tellDisconnectionToAllTEPs();
					this->stop();
				}
				break;
			} catch (SpaceWireRPacketException& e) {
				//todo
				cerr << "SpaceWireREngine::run() got SpaceWireRPacketException " << e.toString() << endl;
				dumpReceivedPacket(data);
				this->stop();
				delete data;
				delete packet;
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

private:
	void dumpReceivedPacket(std::vector<uint8_t>* data) {
		using namespace std;
		cerr << "SpaceWireREngine got invalid SpaceWire-R packet." << endl;
		SpaceWireUtilities::dumpPacket(data);
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
