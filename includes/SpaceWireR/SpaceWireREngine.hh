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

//#define SpaceWireREngineDumpPacket
#define DebugSpaceWireREngine

#undef SpaceWireREngineDumpPacket
//#undef DebugSpaceWireREngine


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
	std::map<uint16_t, SpaceWireRTEPInterface*> receiveTEPs;
	std::map<uint16_t, SpaceWireRTEPInterface*> transmitTEPs;
	std::map<uint16_t, SpaceWireRTEPInterface*> allTEPs;

private:
	size_t nDiscardedReceivedPackets;
	size_t nSentPackets;
	size_t nReceivedPackets;
	CxxUtilities::Mutex sendMutex;

private:
	static constexpr double TimeoutDurationForStopCondition = 1000;

public:
	SpaceWireREngine(SpaceWireIF* spwif) {
		this->spwif = spwif;
		nDiscardedReceivedPackets = 0;
		nSentPackets = 0;
		nReceivedPackets = 0;
	}

public:
	static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 1000000;

public:
	void processReceivedSpaceWireRPacket(SpaceWireRPacket* packet) throw (SpaceWireREngineException) {
		using namespace std;
#ifdef DebugSpaceWireREngine
		cout << "SpaceWireREngine::processReceivedSpaceWireRPacket()" << endl;
#endif
		uint16_t channel = packet->getChannelNumber();
		if(packet->isHeartBeatPacketType() || packet->isHeartBeatAckPacketType()){
			// for HeartBeat/HeartBeatAck packets, all TEPs can be a potential destiantion TEP,
			// and therefore, search in the allTEPs map.
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is HeartBeat/HeartBeatAck packet." << endl;
#endif
			std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = allTEPs.find(channel);
			if (it_find != allTEPs.end()) {
				//if there is TransmitTEP/ReceiveTEP corresponding to the channel number in the received packet.
				it_find->second->pushReceivedSpaceWireRPacket(packet); //pass the received packet to the ReceiveTEP
				//tell the TEP that a packet has arrived.
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
		}else if(packet->isFlowControlPacket()){
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is FlowControl packet (sequence number=" << packet->getSequenceNumberAs32bitInteger() << ")." << endl;
#endif
			std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = allTEPs.find(channel);
			if (it_find != allTEPs.end()) {
				//if there is TransmitTEP/ReceiveTEP corresponding to the channel number in the received packet.
				it_find->second->pushReceivedSpaceWireRPacket(packet); //pass the received packet to the ReceiveTEP
				//tell the TEP that a packet has arrived.
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
		} else if (!packet->isAckPacket()) { //command/data packet
#ifdef DebugSpaceWireREngine
			cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is Command/Data packet." << endl;
#endif
			std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = receiveTEPs.find(channel);
			if (it_find != receiveTEPs.end()) {
				//if there is ReceiveTEP corresponding to the channel number in the received packet.
				it_find->second->pushReceivedSpaceWireRPacket(packet); //pass the received packet to the ReceiveTEP
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
			std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = transmitTEPs.find(channel);
			if (it_find != transmitTEPs.end()) {
				//if there is TransmitTEP corresponding to the channel number in the received packet.
				it_find->second->pushReceivedSpaceWireRPacket(packet); //pass the received packet to the ReceiveTEP
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
			std::vector<uint8_t>* packetBuffer=packet->getPacketBufferPointer();
			spwif->send(packetBuffer);
			delete packetBuffer;
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
		allTEPs[instance->channel]=instance;
		using namespace std;
#ifdef DebugSpaceWireREngine
		cout << "SpaceWireREngine::registerReceiveTEP() channel=" << (uint32_t) instance->channel
				<< " receiveTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
	}

public:
	void unregisterReceiveTEP(uint16_t channel) {
		if (receiveTEPs.find(channel) != receiveTEPs.end()) {
			receiveTEPs.erase(receiveTEPs.find(channel));
		}
		if (allTEPs.find(channel) != allTEPs.end()) {
			allTEPs.erase(allTEPs.find(channel));
		}
	}

public:
	void registerTransmitTEP(SpaceWireRTEPInterface* instance) {
		transmitTEPs[instance->channel] = instance;
		allTEPs[instance->channel]=instance;
#ifdef DebugSpaceWireREngine
		using namespace std;
		cout << "SpaceWireREngine::registerTransmitTEP() channel=" << (uint32_t) instance->channel
				<< " transmitTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
	}

public:
	void unregisterTransmitTEP(uint16_t channel) {
		if (transmitTEPs.find(channel) != transmitTEPs.end()) {
			transmitTEPs.erase(transmitTEPs.find(channel));
		}
		if (allTEPs.find(channel) != allTEPs.end()) {
			allTEPs.erase(allTEPs.find(channel));
		}
	}

public:
	void tellDisconnectionToAllTEPs() {
		std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it1=receiveTEPs.begin();
		while(it1!=receiveTEPs.end()){
			it1->second->closeDueToSpaceWireIFFailure();
		}
		std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it=transmitTEPs.begin();
		while(it!=transmitTEPs.end()){
			it->second->closeDueToSpaceWireIFFailure();
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
#ifdef SpaceWireREngineDumpPacket
			SpaceWireUtilities::dumpPacket(data);
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
				if(e.getStatus()==SpaceWireIFException::Timeout){
					goto _SpaceWireREngine_run_loop;
				}else
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
				std::stringstream ss;
				ss << "SpaceWireREngine::run() got an exception." << endl;
				CxxUtilities::TerminalControl::displayInRed(ss.str());
				break;
			}

		}
#ifdef DebugSpaceWireREngine
		cerr << "SpaceWireREngine::run() Stops." << endl;
#endif
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
