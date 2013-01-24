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
	}

public:
	virtual ~SpaceWireRReceiveTEP() {
		while(receivedApplicationData.size!=0){
			delete receivedApplicationData.front();
			receivedApplicationData.pop_front();
		}
		if(currentApplicationData!=NULL){
			delete currentApplicationData;
		}
	}

// ---------------------------------------------
public:
	static const double WaitDurationInMsForClosedLoop = 100; //m1s
	static const double WaitDurationInMsForReceive = 1000; //ms
	static const double WaitDurationForPacketReceiveLoop = 100; //ms
	static const double WaitDurationForTransitionFromClosingToClosed = 1000; //ms

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
		spwREngine->registerReceiveTEP(channel, &receivedPackets, &packetArrivalNotifier);
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
	void open() throw (SpaceWireRTEPException) {
		state = SpaceWireRTEPState::Enabled;
		stateTransitionNotifier.signal();
		if(stopped){
			this->start();
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
		ackPacket->constructAckForPacket(controlPacket);
		try {
			spwREngine->sendPacket(ackPacket);
		} catch (...) {
			malfunctioningSpaceWireIF();
		}
	}


private:
	void consumeReceivedPackets() {
		while (receivedPackets.size() != 0) {
			SpaceWireRPacket* packet = receivedPackets.front();
			receivedPackets.pop_front();

			if (packet->isDataPacket()) {
				replyAckForPacket(packet);
				processDataPacket(packet);
				delete packet;
				continue;
			}

			if (packet->isControlPacketOpenCommand()) {
				if (this->hasReceivedDataPacket == false) {
					replyAckForPacket(packet);
				} else {
					//Open packet was already received, and one or more Data packet have been received.
					//Therefore this Open packet is invalid.
					//Moves to the Closing state.
					malfunctioningTransportChannel();
				}
				delete packet;
				continue;
			}

			if (packet->isControlPacketCloseCommand()) {
				this->state = SpaceWireRTEPState::Closing;
				replyAckForPacket(packet);
				delete packet;
				continue;
			}

			if (packet->isKeepAlivePacketType()) {
				processKeepAlivePacket(packet);
				delete packet;
				continue;
			}
		}
	}

private:
	void processDataPacket(SpaceWireRPacket* packet) {
		uint8_t sequenceNumber = packet->getSequenceNumber();
		if (insideForwardSlidingWindow(sequenceNumber)) {
			if (this->slidingWindowBuffer[sequenceNumber] == NULL) {
				replyAckForPacket(packet);
				this->slidingWindowBuffer[sequenceNumber] = packet;
				slideSlidingWindow();
			} else {
				replyAckForPacket(packet);
			}
		} else if (insideBackwardSlidingWindow(sequenceNumber)) {
			replyAckForPacket(packet);
		} else {
			malfunctioningTransportChannel();
			nDiscardedDataPackets++;
		}
	}

private:
	void slideSlidingWindow() {
		uint8_t n = this->slidingWindowFrom;
		while (this->slidingWindowBuffer[n] != NULL) {
			reconstructApplicationData(this->slidingWindowBuffer[n]);
			delete this->slidingWindowBuffer[n];
			this->slidingWindowBuffer[n] = NULL;
			n = (uint8_t) (n + 1);
		}
		this->slidingWindowFrom = n;
	}

private:
	void reconstructApplicationData(SpaceWireRPacket* packet) {
		this->hasReceivedDataPacket = true;
		if (isReceivingSegmentedApplicationData) {
			//normal cases
			if (packet->isContinuedSegment()) {
				appendDataToCurrentApplicationDataInstance(packet);
				isReceivingSegmentedApplicationData=true;
				return;
			} else if (packet->isLastSegment()) {
				appendDataToCurrentApplicationDataInstance(packet);
				completeServiceDataUnitHasBeenReceived();
				isReceivingSegmentedApplicationData=false;
				return;
			}
			//error cases
			if (packet->isCompleteSegment() || packet->isFirstSegment()) {
				//something is wrong with segmented data
				malfunctioningTransportChannel();
				return;
			}
		} else {
			//normal case
			if(packet->isCompleteSegment()){
				completeServiceDataUnitHasBeenReceived(packet);
				isReceivingSegmentedApplicationData=false;
				return;
			}
			if(packet->isFirstSegment()){
				//start new segmented application data
				currentApplicationData=new std::vector<uint8_t>;
				appendDataToCurrentApplicationDataInstance(packet);
				isReceivingSegmentedApplicationData=true;
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
	void completeServiceDataUnitHasBeenReceived(){
		receivedApplicationData.push_back(currentApplicationData);
		currentApplicationData=NULL;
	}

private:
	void completeServiceDataUnitHasBeenReceived(SpaceWireRPacket* packet){
		std::vector<uint8_t>* applicationData=new std::vector<uint8_t>(packet->getPayloadLength());
		applicationData=*(packet->getPayload());
		receivedApplicationData.push_back(applicationData);
		currentApplicationData=NULL;
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
	bool insideForwardSlidingWindow(uint8_t sequenceNumber) {
		uint8_t n = this->slidingWindowFrom;
		uint8_t k = this->slidingWindowSize;
		if ((uint8_t) (n) < (uint8_t) (n + k - 1)) {
			// n -- n + k - 1
			if (n <= sequenceNumber && sequenceNumber <= (uint8_t) (n + k - 1)) {
				return true;
			} else {
				return false;
			}
		} else {
			// n -- 255, 0 -- n + k - 1
			if ((n <= sequenceNumber && sequenceNumber <= 255)
					|| (0 <= sequenceNumber && sequenceNumber <= (uint8_t) (n + k - 1))) {
				return true;
			} else {
				return false;
			}
		}
	}

private:
	bool insideBackwardSlidingWindow(uint8_t sequenceNumber) {
		uint8_t n = this->slidingWindowFrom;
		uint8_t k = this->slidingWindowSize;
		if ((uint8_t) (n - k) < (uint8_t) (n - 1)) {
			//n-k -- n-1
			if ((uint8_t) (n - k) <= sequenceNumber && sequenceNumber <= (uint8_t) (n - 1)) {
				return true;
			} else {
				return false;
			}
		} else {
			//n-k -- 255, 0 -- n - 1
			if (((uint8_t) (n - k) <= sequenceNumber && sequenceNumber <= 255)
					|| (0 <= sequenceNumber && sequenceNumber <= (uint8_t) (n - 1))) {
				return true;
			} else {
				return false;
			}
		}
	}

private:
	void malfunctioningTransportChannel() {
		this->state = SpaceWireRTEPState::Closing;
		//todo
		//add message which should be sent to user
	}

private:
	void malfunctioningSpaceWireIF(){
		this->state = SpaceWireRTEPState::Closed;
		//todo
		//send message to user that SpaceWire IF is failing
		//probably, Action mechanism is suitable.
	}

public:
	void run() {
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
					}
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
}
;
#endif /* SPACEWIRERRECEIVETEP_HH_ */
