/*
 * SpaceWireRReceiveTEP.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERRECEIVETEP_HH_
#define SPACEWIRERRECEIVETEP_HH_

#include "SpaceWireR/SpaceWireRTEP.hh"

class SpaceWireRReceiveTEP: public SpaceWireRTEP, public CxxUtilities::StoppableThread {

public:
	SpaceWireRReceiveTEP(SpaceWireREngine* spwREngine, uint8_t channel) :
			SpaceWireRTEP(SpaceWireRTEPType::ReceiveTEP, spwREngine, channel) {
		registerMeToSpaceWireREngine();
		initializeCounters();
		isReceivingSegmentedApplicationData = false;
		ackPacket = new SpaceWireRPacket();
		hasReceivedDataPacket = false;
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

private:
	std::vector<uint8_t>* currentApplicationData;
	bool isReceivingSegmentedApplicationData;
	std::list<std::vector<uint8_t>*> receivedApplicationData;
	CxxUtilities::Condition receiveWaitCondition; //used for receive of application data

public:
	static const size_t MaxReceivedApplicationData = 1000;

private:
	size_t nDiscardedApplicationData;
	size_t nDiscardedApplicationDataBytes;
	size_t nDiscardedControlPackets;
	size_t nDiscardedDataPackets;
	size_t nDiscardedKeepAlivePackets;

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

private:
	SpaceWireRPacket* popReceivedSpaceWireRPacket() {
		SpaceWireRPacket* result = *(receivedPackets.begin());
		receivedPackets.pop_front();
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
		if (stopped) {
			this->start();
		}
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
	void replyAckForPacket(SpaceWireRPacket* controlPacket) {
		using namespace std;
		ackPacket->constructAckForPacket(controlPacket);
		try {
			cout << "SpaceWireRReceiveTEP::replyAckForPacket() replying ack for sequence number = "
					<< (uint32_t) ackPacket->getSequenceNumber() << endl;
			spwREngine->sendPacket(ackPacket);
			cout << "SpaceWireRReceiveTEP::replyAckForPacket() ack for sequence number = "
					<< (uint32_t) ackPacket->getSequenceNumber() << " has been sent." << endl;
		} catch (...) {
			malfunctioningSpaceWireIF();
		}
	}

private:
	CxxUtilities::Mutex mutexForConsumeReceivedPacketes;
	bool isConsumingReceivedPacketes;

private:
	void consumeReceivedPackets() {
		mutexForConsumeReceivedPacketes.lock();
		if(isConsumingReceivedPacketes){
			mutexForConsumeReceivedPacketes.unlock();
			//another thread is in this method
			return;
		}
		mutexForConsumeReceivedPacketes.unlock();

		mutexForConsumeReceivedPacketes.lock();
		isConsumingReceivedPacketes=true;
		mutexForConsumeReceivedPacketes.unlock();

		using namespace std;
		cout << "SpaceWireRReceiveTEP::consumeReceivedPackets()" << endl;
		while (receivedPackets.size() != 0) {
			cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() consuming one packet." << endl;
			SpaceWireRPacket* packet = receivedPackets.front();
			receivedPackets.pop_front();

			if (packet->isDataPacket()) {
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Data Packet." << endl;
				processDataPacket(packet);
				continue;
			}

			if (packet->isControlPacketOpenCommand()) {
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Open command." << endl;
				if (this->hasReceivedDataPacket == false && this->state == SpaceWireRTEPState::Enabled) {
					processOpenComand(packet);
				} else {
					//Open packet was already received, and one or more Data packet have been received.
					//Therefore this Open packet is invalid.
					//Moves to the Closing state.
					cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() invalid Open command." << endl;
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
				cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() processing Keep Alive command." << endl;
				processKeepAlivePacket(packet);
				delete packet;
				continue;
			}
		}
		cout << "SpaceWireRReceiveTEP::consumeReceivedPackets() completed." << endl;

		mutexForConsumeReceivedPacketes.lock();
		isConsumingReceivedPacketes=false;
		mutexForConsumeReceivedPacketes.unlock();
	}

private:
	void processOpenComand(SpaceWireRPacket* packet){
		this->state = SpaceWireRTEPState::Open;
		replyAckForPacket(packet);
		this->slidingWindowBuffer[packet->getSequenceNumber()] = packet;
		slideSlidingWindow();
	}

private:
	void processDataPacket(SpaceWireRPacket* packet) {
		using namespace std;
		uint8_t sequenceNumber = packet->getSequenceNumber();
		cout << "SpaceWireRReceiveTEP::processDataPacket() sequenceNumber=" <<(uint32_t)sequenceNumber << endl;
		if (insideForwardSlidingWindow(sequenceNumber)) {
			cout << "SpaceWireRReceiveTEP::processDataPacket() insideForwardSlidingWindow" << endl;
			if (this->slidingWindowBuffer[sequenceNumber] == NULL) {
				replyAckForPacket(packet);
				this->slidingWindowBuffer[sequenceNumber] = packet;
				slideSlidingWindow();
			} else {
				replyAckForPacket(packet);
				delete packet;
			}
		} else if (insideBackwardSlidingWindow(sequenceNumber)) {
			cout << "SpaceWireRReceiveTEP::processDataPacket() insideBackwardSlidingWindow" << endl;
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
		cout << "SpaceWireRReceiveTEP::slideSlidingWindow()" << endl;
		uint8_t n = this->slidingWindowFrom;
		while (this->slidingWindowBuffer[n] != NULL) {
			cout << "SpaceWireRReceiveTEP::slideSlidingWindow() n=" << (uint32_t)n << endl;
			reconstructApplicationData(this->slidingWindowBuffer[n]);
			delete this->slidingWindowBuffer[n];
			this->slidingWindowBuffer[n] = NULL;
			n = (uint8_t) (n + 1);
		}
		this->slidingWindowFrom = n;
		cout << "SpaceWireRReceiveTEP::slideSlidingWindow() slidingWindowFrom=" << (uint32_t)this->slidingWindowFrom << endl;
	}

private:
void reconstructApplicationData(SpaceWireRPacket* packet) {
	using namespace std;
		this->hasReceivedDataPacket = true;
		cout << "SpaceWireRReceiveTEP::reconstructApplicationData() isReceivingSegmentedApplicationData=";
		if(isReceivingSegmentedApplicationData){
			cout << "true" << endl;
		}else{
			cout << "false"
					<< endl;
		}

		if (isReceivingSegmentedApplicationData) {
			//normal cases
			if (packet->isContinuedSegment()) {
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received continuous segment. " << endl;
				appendDataToCurrentApplicationDataInstance(packet);
				isReceivingSegmentedApplicationData = true;
				return;
			} else if (packet->isLastSegment()) {
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received the last segment. " << endl;
				appendDataToCurrentApplicationDataInstance(packet);
				completeServiceDataUnitHasBeenReceived();
				isReceivingSegmentedApplicationData = false;
				return;
			}
			//error cases
			if (packet->isCompleteSegment() || packet->isFirstSegment()) {
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received invalid segment: ";
				if (packet->isCompleteSegment()){
					cout << "Complete Segment." << endl;
				}else{
					cout << "The First Segment." << endl;
				}

				//something is wrong with segmented data
				malfunctioningTransportChannel();
				return;
			}
		} else {
			//normal case
			if (packet->isCompleteSegment()) {
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received complete segment. " << endl;
				completeServiceDataUnitHasBeenReceived(packet);
				isReceivingSegmentedApplicationData = false;
				return;
			}
			if (packet->isFirstSegment()) {
				cout << "SpaceWireRReceiveTEP::reconstructApplicationData() received the first segment. " << endl;
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
					SpaceWireRPacket* packet = receivedPackets.front();
					if (packet->isControlPacketCloseCommand()) {
						replyAckForPacket(packet);
					}
					receivedPackets.pop_front();
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
		ss << "---------------------------------------------" << endl;
		return ss.str();
	}
};
#endif /* SPACEWIRERRECEIVETEP_HH_ */
