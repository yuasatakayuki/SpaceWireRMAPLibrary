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
 * SpaceWireRReceiveTEP.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERRECEIVETEP_HH_
#define SPACEWIRERRECEIVETEP_HH_

#include "SpaceWireR/SpaceWireRTEP.hh"

#undef DebugSpaceWireRReceiveTEP
#undef DebugSpaceWireRReceiveTEPDumpCriticalIncidents

class SpaceWireRReceiveTEP: public SpaceWireRTEP, public CxxUtilities::StoppableThread {

public:
	SpaceWireRReceiveTEP(SpaceWireREngine* spwREngine, uint16_t channel) :
			SpaceWireRTEP(SpaceWireRTEPType::ReceiveTEP, spwREngine, channel) {
		registerMeToSpaceWireREngine();
		initializeCounters();
		isReceivingSegmentedApplicationData = false;
		ackPacket = new SpaceWireRPacket();
		hasReceivedDataPacket = false;
		randomMT = new CxxUtilities::RandomMT();
		this->start();
	}

public:
	virtual ~SpaceWireRReceiveTEP() {
		unregisterMeToSpaceWireREngine();
		this->stop();
		this->waitUntilRunMethodComplets();

		while (receivedApplicationData.size() != 0) {
			delete receivedApplicationData.front();
			receivedApplicationData.pop_front();
		}
		if (currentApplicationData != NULL) {
			delete currentApplicationData;
		}
	}

// ---------------------------------------------

private:
	SpaceWireRPacket* ackPacket;
	CxxUtilities::RandomMT* randomMT;

private:
	std::vector<uint8_t>* currentApplicationData;
	bool isReceivingSegmentedApplicationData;
	std::list<std::vector<uint8_t>*> receivedApplicationData;
	CxxUtilities::Condition receiveWaitCondition; //used for receive of application data

public:
	static const size_t MaxReceivedApplicationData = 1000;

public:
	size_t nDiscardedApplicationData;
	size_t nDiscardedApplicationDataBytes;
	size_t nDiscardedControlPackets;
	size_t nDiscardedDataPackets;
	size_t nDiscardedKeepAlivePackets;
	size_t nErrorInjectionNoReply;
	size_t nReceivedDataBytes;
	size_t nReceivedSegments;
	size_t nReceivedApplicationData;

private:
	bool hasReceivedDataPacket;
//---------------------------------------------

public:
	void initializeCounters() {
		this->nDiscardedApplicationData = 0;
		this->nDiscardedApplicationDataBytes = 0;
		this->nDiscardedControlPackets = 0;
		this->nDiscardedDataPackets = 0;
		this->nDiscardedKeepAlivePackets = 0;
		this->nErrorInjectionNoReply = 0;
		this->nReceivedDataBytes=0;
		this->nReceivedApplicationData=0;
		this->nReceivedSegments=0;
	}

private:
	void registerMeToSpaceWireREngine() {
		spwREngine->registerReceiveTEP(this);
	}

private:
	void unregisterMeToSpaceWireREngine() {
		spwREngine->unregisterReceiveTEP(channel);
	}

private:
	std::vector<uint8_t>* popApplicationData() {
		std::vector<uint8_t>* result = *(receivedApplicationData.begin());
		receivedApplicationData.pop_front();
		return result;
	}

public:
	std::vector<uint8_t>* receive(double timeoutDuration = WaitDurationInMsForReceive) throw (SpaceWireRTEPException) {
		if (state != SpaceWireRTEPState::Open) {
			throw SpaceWireRTEPException(SpaceWireRTEPException::NotInTheOpenState);
		}
		if (receivedApplicationData.size() != 0) {
			return popApplicationData();
		}
		receiveWaitCondition.wait(timeoutDuration);
		if (receivedApplicationData.size() != 0) {
			return popApplicationData();
		} else {
			throw SpaceWireRTEPException(SpaceWireRTEPException::Timeout);
		}
	}

private:
	void applicationDataWasDiscarded(size_t length) {
		nDiscardedApplicationData++;
		nDiscardedApplicationDataBytes += length;
	}

private:
	void controlPacketWasDiscarded() {
		nDiscardedControlPackets++;
	}

private:
	void sendNack() {

	}

public:
	static const double WaitDurationInMsForOpenWaitLoop = 100; //ms

public:
	void open() throw (SpaceWireRTEPException) {
		state = SpaceWireRTEPState::Enabled;
		stateTransitionNotifier.signal();
		CxxUtilities::Condition c;
		while (this->state != SpaceWireRTEPState::Open) {
			c.wait(WaitDurationInMsForOpenWaitLoop);
		}
	}

private:
	void closed() {
		hasReceivedDataPacket = false;
		this->initializeSlidingWindow();
		while (receivedApplicationData.size() != 0) {
			nDiscardedApplicationData++;
			delete receivedApplicationData.front();
			receivedApplicationData.pop_front();
		}
		this->stop();
		unregisterMeToSpaceWireREngine();
	}

public:
	size_t getNDiscardedApplicationData() const {
		return nDiscardedApplicationData;
	}

public:
	size_t getNDiscardedApplicationDataBytes() const {
		return nDiscardedApplicationDataBytes;
	}

public:
	size_t getNDiscardedCommandPackets() const {
		return nDiscardedControlPackets;
	}

public:
	size_t getNDiscardedKeepAlivePackets() const {
		return nDiscardedKeepAlivePackets;
	}

public:
	size_t getNDiscardedDataPackets() const {
		return nDiscardedDataPackets;
	}

private:
	static const double ProbabilityOfErrorInjectionNoReply = 0;
	static const double ProbabilityOfErrorInjectionCRCError = 0;

private:
	bool errorInjectionNoReply(uint8_t sequenceNumber) {
		using namespace std;
		if (randomMT->generateRandomDoubleFrom0To1() < ProbabilityOfErrorInjectionNoReply) {
#ifdef DebugSpaceWireRReceiveTEPDumpCriticalIncidents
			std::stringstream ss;
			ss << "SpaceWireRReceiveTEP::errorInjectionNoReply() for sequence number = " << dec << right << (uint32_t) sequenceNumber << " !!!"
			<< endl;
			CxxUtilities::TerminalControl::displayInRed(ss.str());
			nErrorInjectionNoReply++;
#endif
			return true;
		} else {
			return false;
		}
	}

private:
	void replyAckForPacket(SpaceWireRPacket* packet) {
		if (!errorInjectionNoReply(packet->getSequenceNumber())) {
			using namespace std;
			ackPacket->constructAckForPacket(packet);
			try {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::replyAckForPacket() replying ack for sequence number = "
				<< (uint32_t) ackPacket->getSequenceNumber() << endl;
#endif

				//todo: inject CRC error

				spwREngine->sendPacket(ackPacket);
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::replyAckForPacket() ack for sequence number = "
				<< (uint32_t) ackPacket->getSequenceNumber() << " has been sent." << endl;
#endif
			} catch (...) {
				malfunctioningSpaceWireIF();
			}
		}
	}

private:
	CxxUtilities::Mutex mutexForConsumeReceivedPacketes;
	bool isConsumingReceivedPacketes;

private:
	void consumeReceivedPackets() {
		using namespace std;
		mutexForConsumeReceivedPacketes.lock();
		if (isConsumingReceivedPacketes) {
			mutexForConsumeReceivedPacketes.unlock();
			//another thread is in this method
			return;
		}
		mutexForConsumeReceivedPacketes.unlock();

		mutexForConsumeReceivedPacketes.lock();
		isConsumingReceivedPacketes = true;
		mutexForConsumeReceivedPacketes.unlock();

		using namespace std;
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::consumeReceivedPackets()" << endl;
#endif

		size_t loopSize = receivedPackets.size();

		for (size_t i = 0; i < loopSize; i++) {
#ifdef DebugSpaceWireRReceiveTEP
			cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() consuming one packet." << endl;
#endif
			SpaceWireRPacket* packet = this->popReceivedSpaceWireRPacket();

			if (packet->isDataPacket()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Data Packet." << endl;
#endif
				processDataPacket(packet);
				continue;
			}

			if (packet->isControlPacketOpenCommand()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Open command." << endl;
#endif
				if (this->hasReceivedDataPacket == false && this->state == SpaceWireRTEPState::Enabled) {
					processOpenComand(packet);
				} else {
					//Open packet was already received, and one or more Data packet have been received.
					//Therefore this Open packet is invalid.
					//Moves to the Closing state.
#ifdef DebugSpaceWireRReceiveTEP
					cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() invalid Open command." << endl;
#endif
					malfunctioningTransportChannel();
				}
				continue;
			}

			if (packet->isControlPacketCloseCommand()) {
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Close command." << endl;
				this->state = SpaceWireRTEPState::Closing;
				replyAckForPacket(packet);
				delete packet;
				continue;
			}

			if (packet->isKeepAlivePacketType()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Keep Alive command." << endl;
#endif
				processKeepAlivePacket(packet);
				delete packet;
				continue;
			}
		}
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() completed." << endl;
#endif

		mutexForConsumeReceivedPacketes.lock();
		isConsumingReceivedPacketes = false;
		mutexForConsumeReceivedPacketes.unlock();
	}

private:
	void processOpenComand(SpaceWireRPacket* packet) {
		this->state = SpaceWireRTEPState::Open;
		replyAckForPacket(packet);
		this->slidingWindowBuffer[packet->getSequenceNumber()] = packet;
		slideSlidingWindow();
	}

private:
	void processDataPacket(SpaceWireRPacket* packet) {
		using namespace std;
		uint8_t sequenceNumber = packet->getSequenceNumber();
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::processDataPacket() sequenceNumber=" <<(uint32_t)sequenceNumber << endl;
#endif
		nReceivedDataBytes+=packet->getPayloadLength();
		nReceivedSegments++;
		if (insideForwardSlidingWindow(sequenceNumber)) {
#ifdef DebugSpaceWireRReceiveTEP
			cout << "SpaceWireRReceiveTEP::processDataPacket() insideForwardSlidingWindow" << endl;
#endif
			if (this->slidingWindowBuffer[sequenceNumber] == NULL) {
				replyAckForPacket(packet);
				this->slidingWindowBuffer[sequenceNumber] = packet;
				slideSlidingWindow();
			} else {
				replyAckForPacket(packet);
				delete packet;
			}
		} else if (insideBackwardSlidingWindow(sequenceNumber)) {
#ifdef DebugSpaceWireRReceiveTEP
			cout << "SpaceWireRReceiveTEP::processDataPacket() insideBackwardSlidingWindow" << endl;
#endif
			//debug
			CxxUtilities::TerminalControl::displayInCyan("SpaceWireRReceiveTEP::processDataPacket() insideBackwardSlidingWindow");
			std::stringstream ss;
			ss << "sequenceNumber:" << "0x" << hex << right << setw(2) << setfill('0')  << (uint32_t)packet->getSequenceNumber() << endl;
			ss << "packetType:" << ((packet->isDataPacket())? "Data":"Other than data") << endl;
			ss << "segment   :" << ((packet->isFirstSegment())? "First": ((packet->isContinuedSegment())? "Continued":"Last") ) << endl;
			CxxUtilities::TerminalControl::displayInCyan(ss.str());
			replyAckForPacket(packet);
			delete packet;
		} else {
			malfunctioningTransportChannel();
			nDiscardedDataPackets++;
		}
	}

private:
	void slideSlidingWindow() {
		using namespace std;
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::slideSlidingWindow()" << endl;
#endif
		uint8_t n = this->slidingWindowFrom;
		while (this->slidingWindowBuffer[n] != NULL) {
#ifdef DebugSpaceWireRReceiveTEP
			cout << "SpaceWireRReceiveTEP::slideSlidingWindow() n=" << (uint32_t)n << endl;
#endif
			reconstructApplicationData(this->slidingWindowBuffer[n]);
			delete this->slidingWindowBuffer[n];
			this->slidingWindowBuffer[n] = NULL;
			n = (uint8_t) (n + 1);
		}
		this->slidingWindowFrom = n;
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::slideSlidingWindow() slidingWindowFrom=" << (uint32_t)this->slidingWindowFrom << endl;
#endif
	}

private:
	void reconstructApplicationData(SpaceWireRPacket* packet) {
		using namespace std;
		this->hasReceivedDataPacket = true;
#ifdef DebugSpaceWireRReceiveTEP
		cout << "SpaceWireRReceiveTEP::reconstructApplicationData() isReceivingSegmentedApplicationData=";
		if(isReceivingSegmentedApplicationData) {
			cout << "true" << endl;
		} else {
			cout << "false"
			<< endl;
		}
#endif

		if (isReceivingSegmentedApplicationData) {
			//normal cases
			if (packet->isContinuedSegment()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received continuous segment. " << endl;
#endif
				appendDataToCurrentApplicationDataInstance(packet);
				nReceivedApplicationData++;
				isReceivingSegmentedApplicationData = true;
				return;
			} else if (packet->isLastSegment()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received the last segment. " << endl;
#endif
				appendDataToCurrentApplicationDataInstance(packet);
				completeServiceDataUnitHasBeenReceived();
				isReceivingSegmentedApplicationData = false;
				return;
			}
			//error cases
			if (packet->isCompleteSegment() || packet->isFirstSegment()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received invalid segment: ";
				if (packet->isCompleteSegment()) {
					cout << "Complete Segment." << endl;
				} else {
					cout << "The First Segment." << endl;
				}
#endif
				//something is wrong with segmented data
				malfunctioningTransportChannel();
				return;
			}
		} else {
			//normal case
			if (packet->isCompleteSegment()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received complete segment. " << endl;
#endif
				completeServiceDataUnitHasBeenReceived(packet);
				isReceivingSegmentedApplicationData = false;
				return;
			}
			if (packet->isFirstSegment()) {
#ifdef DebugSpaceWireRReceiveTEP
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received the first segment. " << endl;
#endif
				//start new segmented application data
				currentApplicationData = new std::vector<uint8_t>;
				appendDataToCurrentApplicationDataInstance(packet);
				isReceivingSegmentedApplicationData = true;
				return;
			}
			//error cases
			if (packet->isContinuedSegment() || packet->isLastSegment()) {
				//something is wrong with segmented data
				malfunctioningTransportChannel();
				return;
			}
		}
	}

private:
	void appendDataToCurrentApplicationDataInstance(SpaceWireRPacket* packet) {
		std::vector<uint8_t>* payload = packet->getPayload();
		size_t size = packet->getPayloadLength();
		for (size_t i = 0; i < size; i++) {
			currentApplicationData->push_back(payload->at(i));
		}
	}

private:
	void completeServiceDataUnitHasBeenReceived() {
		receivedApplicationData.push_back(currentApplicationData);
		currentApplicationData = NULL;
	}

private:
	void completeServiceDataUnitHasBeenReceived(SpaceWireRPacket* packet) {
		std::vector<uint8_t>* applicationData = new std::vector<uint8_t>(packet->getPayloadLength());
		applicationData = packet->getPayload();
		receivedApplicationData.push_back(applicationData);
		currentApplicationData = NULL;
	}

private:
	void processKeepAlivePacket(SpaceWireRPacket* packet) {
		uint8_t sequenceNumber = packet->getSequenceNumber();
		if (insideForwardSlidingWindow(sequenceNumber)) {
			replyAckForPacket(packet);
		} else if (insideBackwardSlidingWindow(sequenceNumber)) {
			replyAckForPacket(packet);
		} else {
			malfunctioningTransportChannel();
			nDiscardedKeepAlivePackets++;
		}
	}

private:
	void malfunctioningTransportChannel() {
		this->state = SpaceWireRTEPState::Closing;
		//todo
		//add message which should be sent to user
		using namespace std;
		cout << "SpaceWireRReceiveTEP::malfunctioningTransportChannel() !!! MALFUNCTIONING TEP !!!" << endl;
		cout << ((SpaceWireRTEPInterface*)(NULL))->channel << endl;
	}

private:
	void malfunctioningSpaceWireIF() {
		this->state = SpaceWireRTEPState::Closed;
		//todo
		//send message to user that SpaceWire IF is failing
		//probably, Action mechanism is suitable.
		using namespace std;
		cout << "SpaceWireRReceiveTEP::malfunctioningSpaceWireIF() !!! MALFUNCTIONING SpaceWire IF !!!" << endl;
	}

public:
	void run() {
		using namespace std;
		static CxxUtilities::Condition waitTimer;
		while (!stopped) {
			switch (this->state) {
			case SpaceWireRTEPState::Closed:
				while (this->state == SpaceWireRTEPState::Closed && !stopped) {
					stateTransitionNotifier.wait(WaitDurationInMsForClosedLoop);
					if (this->state == SpaceWireRTEPState::Closed) {
						discardReceivedPackets();
					}
				}
				break;

			case SpaceWireRTEPState::Enabled:
				while (this->state == SpaceWireRTEPState::Enabled && !stopped) {
					packetArrivalNotifier.wait(WaitDurationForPacketReceiveLoop);
					consumeReceivedPackets();
					/*
					 cout << "SpaceWireRReceiveTEP::run() Enabled state. receivedPackets.size()=" << receivedPackets.size()
					 << endl;
					 if (receivedPackets.size() == 0) {
					 packetArrivalNotifier.wait(WaitDurationForPacketReceiveLoop);
					 } else {
					 SpaceWireRPacket* packet = receivedPackets.front();
					 receivedPackets.pop_front();
					 if (packet->isControlPacketOpenCommand()) {
					 this->state = SpaceWireRTEPState::Open;
					 replyAckForPacket(packet);
					 delete packet;
					 } else if (packet->isControlPacketCloseCommand()) {
					 this->state = SpaceWireRTEPState::Closing;
					 delete packet;
					 }
					 }*/
				}
				break;

			case SpaceWireRTEPState::Open:
				while (this->state == SpaceWireRTEPState::Open && !stopped) {
					if (receivedPackets.size() != 0) {
						consumeReceivedPackets();
					}
					if (SpaceWireRTEPState::Open) {
						packetArrivalNotifier.wait(WaitDurationForPacketReceiveLoop);
					}
				}
				break;

			case SpaceWireRTEPState::Closing:
				while (receivedPackets.size() != 0) {
					SpaceWireRPacket* packet = this->popReceivedSpaceWireRPacket();
					if (packet->isControlPacketCloseCommand()) {
						replyAckForPacket(packet);
					}
					delete packet;
				}
				waitTimer.wait(WaitDurationForTransitionFromClosingToClosed);
				this->state = SpaceWireRTEPState::Closed;
				closed();
				break;

			default:
				this->stop();
				break;
			}
		}
	}

public:
	std::string toString() {
		using namespace std;
		std::stringstream ss;
		ss << "---------------------------------------------" << endl;
		ss << "SpaceWireRReceiveTEP" << endl;
		ss << "State             : " << SpaceWireRTEPState::toString(this->state) << endl;
		ss << "slidingWindowFrom : (dec)" << dec << (uint32_t) this->slidingWindowFrom << endl;
		ss << "slidingWindowSize : (dec)" << dec << (uint32_t) this->slidingWindowSize << endl;
		ss << "receivedPackets.size() : (dec)" << dec << receivedPackets.size() << endl;
		ss << "ProbabilityOfErrorInjectionNoReply : " << ProbabilityOfErrorInjectionNoReply << endl;
		ss << "nErrorInjectionNoReply : (dec)" << dec << nErrorInjectionNoReply << endl;
		ss << "remaining received ApplicationData : " << dec << receivedApplicationData.size() << endl;
		ss << "nDiscardedApplicationData : " << dec << nDiscardedApplicationData << endl;
		ss << "nDiscardedApplicationDataBytes : " << dec << nDiscardedApplicationDataBytes << endl;
		ss << "nDiscardedControlPackets : " << dec << nDiscardedControlPackets << endl;
		ss << "nDiscardedDataPackets : " << dec << nDiscardedControlPackets << endl;
		ss << "nDiscardedKeepAlivePackets : " << dec << nDiscardedKeepAlivePackets << endl;
		ss << "nReceivedBytes : " << nReceivedDataBytes/1024 << "kB" << endl;
		ss << "nReceivedSegments : " << nReceivedSegments << endl;
		ss << "nReceivedApplicationData : " << nReceivedApplicationData << endl;
		ss << "---------------------------------------------" << endl;
		return ss.str();
	}
};
#endif /* SPACEWIRERRECEIVETEP_HH_ */
