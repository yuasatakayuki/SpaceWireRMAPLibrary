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
 * SpaceWireRTEP.hh
 *
 *  Created on: Jan 14, 2013
 *      Author: yuasa
 */

#ifndef SPACEWIRERTEP_HH_
#define SPACEWIRERTEP_HH_

#include "SpaceWireR/SpaceWireRClassInterfaces.hh"
#include "SpaceWireR/SpaceWireREngine.hh"
#include "SpaceWireR/SpaceWireRPacket.hh"
#include "SpaceWireR/SpaceWireRTEPExceptions.hh"

//#define DebugSpaceWireRTEP

#undef DebugSpaceWireRTEP

class SpaceWireRTEPType {
public:
	enum Type {
		TransmitTEP, //
		ReceiveTEP
	};
};

class SpaceWireRTEPState {
public:
	enum State {
		Closed, Enabled, Open, Closing
	};

public:
	static std::string toString(State state) {
		std::string str;
		switch (state) {
		case Closed:
			str = "Closed";
			break;
		case Closing:
			str = "Closing";
			break;
		case Enabled:
			str = "Enabled";
			break;
		case Open:
			str = "Open";
			break;
		default:
			str = "Undefined status (malfunctioning!)";
			break;
		}
		return str;
	}
};

class SpaceWireRTEP: public SpaceWireRTEPInterface {
protected:
	SpaceWireRTEPType::Type tepType;

protected:
	SpaceWireRTEPState::State state;

protected:
	//routing information
	std::vector<uint8_t> destinationSpaceWireAddress;
	std::vector<uint8_t> sourceSpaceWireAddress;
	uint8_t destinationLogicalAddress;
	uint8_t sourceLogicalAddress;

protected:
	SpaceWireREngine* spwREngine;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;

protected:
	std::vector<SpaceWireRPacket*> slidingWindowBuffer;
	uint8_t slidingWindowFrom;
	uint8_t slidingWindowSize;

protected:
	CxxUtilities::Condition stateTransitionNotifier;

public:
	static const size_t DefaultSlidingWindowSize = 8;
	static const size_t MaxOfSlidingWindow = 256;

public:
	static const double WaitDurationInMsForClosedLoop = 100; //m1s
	static const double WaitDurationInMsForReceive = 1000; //ms
	static const double WaitDurationForPacketReceiveLoop = 100; //ms
	static const double WaitDurationForTransitionFromClosingToClosed = 1000; //ms

public:
	SpaceWireRTEP(SpaceWireRTEPType::Type tepType, SpaceWireREngine* spwREngine, uint16_t channel) {
		this->spwREngine = spwREngine;
		this->state = SpaceWireRTEPState::Closed;
		this->tepType = tepType;
		this->channel = channel;
		this->sourceLogicalAddress = DefaultLogicalAddress;
		this->destinationLogicalAddress = DefaultLogicalAddress;
		this->nReceivedHeartBeatPackets = 0;
		this->isHeartBeatEmissionEnabled = false;
		this->doNotRespondToReceivedHeartBeatPacket_ = false;
		this->heartBeatTimer = new HeartBeatTimer(this);
		this->heartBeatAckPacket = new SpaceWireRPacket();
		retryTimerUpdater = new RetryTimerUpdater(this);
		flowControlPacket = new SpaceWireRPacket();
		retryTimerUpdater->start();
		this->nOfOutstandingPackets = 0;
		this->initializeSlidingWindow();
		this->initializeSlidingWindowRelatedBuffers();
		this->initializeHeartBeatCounters();
		this->prepareSpaceWireRPacketInstances();
	}

public:
	virtual ~SpaceWireRTEP() {
		delete heartBeatAckPacket;
		this->finalizeSlidingWindowRelatedBuffers();
		retryTimerUpdater->stop();
		retryTimerUpdater->waitUntilRunMethodComplets();
		delete retryTimerUpdater;
		delete flowControlPacket;
	}

protected:
	void initializeSlidingWindow() {
		slidingWindowBuffer.clear();
		slidingWindowBuffer.resize(MaxOfSlidingWindow, NULL);
		slidingWindowSize = DefaultSlidingWindowSize;
		slidingWindowFrom = 0;
	}

protected:
	void prepareSpaceWireRPacketInstances() {
		for (size_t i = 0; i < this->MaxOfSlidingWindow; i++) {
			SpaceWireRPacket* packet = new SpaceWireRPacket;
			//set common parameters
			packet->setChannelNumber(channel);
			packet->setDestinationLogicalAddress(destinationLogicalAddress);
			packet->setDestinationSpaceWireAddress(destinationSpaceWireAddress);
			packet->setSecondaryHeaderFlag(SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed);
			packet->setPrefix(this->sourceSpaceWireAddress);
			packet->setSourceLogicalAddress(this->sourceLogicalAddress);
			slidingWindowBuffer[i] = packet;
			/* Note */
			/* The following fields should be set packet by packet. */
			//- sequence flag
			//- packet type
			//- payload length
			//- sequence number
		}
	}

private:
	void initializeHeartBeatCounters() {
		nReceivedHeartBeatPackets = 0;
		nReceivedHeartBeatAckPackets = 0;
		nTransmittedHeartBeatPackets = 0;
		nTransmittedHeartBeatAckPackets = 0;
	}

public:
	bool isTransmitTEP() {
		if (tepType == SpaceWireRTEPType::TransmitTEP) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isReceiveTEP() {
		if (tepType == SpaceWireRTEPType::ReceiveTEP) {
			return true;
		} else {
			return false;
		}
	}

public:
	SpaceWireRTEPState::State getState() {
		return state;
	}

public:
	bool isOpen() {
		if (state == SpaceWireRTEPState::Open) {
			return true;
		} else {
			return false;
		}
	}

public:
	void setEnabled() {
		state = SpaceWireRTEPState::Enabled;
	}

public:
	bool isEnabled() {
		if (state == SpaceWireRTEPState::Enabled) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosed() {
		if (state == SpaceWireRTEPState::Closed) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosing() {
		if (state == SpaceWireRTEPState::Closing) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline uint8_t getSlidingWindowFrom() const {
		return slidingWindowFrom;
	}

public:
	inline void setSlidingWindowFrom(uint8_t slidingWindowFrom) {
		this->slidingWindowFrom = slidingWindowFrom;
	}

public:
	inline uint8_t getSlidingWindowSize() const {
		return slidingWindowSize;
	}

public:
	inline void setSlidingWindowSize(uint8_t slidingWindowSize) {
		this->slidingWindowSize = slidingWindowSize;
	}

protected:
	inline void discardReceivedPackets() {
		while (receivedPackets.size() != 0) {
			delete receivedPackets.front();
			receivedPackets.pop_front();
		}
	}

	/*
	 private:
	 bool insideForwardSlidingWindow(uint8_t sequenceNumber) =0;

	 private:
	 bool insideBackwardSlidingWindow(uint8_t sequenceNumber) =0;
	 */

	/*
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
	 */

	/* ============================================
	 * sendPacket interface
	 * ============================================ */
protected:
	static const double DefaultTimeoutDurationInMs = 1000; //ms
	static const double DefaultWaitDurationInMsForCompletionCheck = 50; //ms
	static const double DefaultWaitDurationInMsForSendSegment = 500; //ms
	static const double DefaultTimeoutDurationInMsForRetryTimers = 50; //ms
	double timeoutDurationForRetryTimers = DefaultTimeoutDurationInMsForRetryTimers; //ms
	static const double WaitDurationInMsForPacketRetransmission = 2000; //ms

protected:
	size_t maximumSegmentSize;
	static const size_t DefaultMaximumSegmentSize = 256;
	static const size_t DefaultMaximumRetryCount = 4;
	size_t maxRetryCount = DefaultMaximumRetryCount;

public:
	//statistics counters
	size_t nRetriedSegments;
	size_t nSentSegments;
	size_t nSentUserData;
	size_t nSentUserDataInBytes;
	size_t nLostAckPackets;
	size_t nOfOutstandingPackets;

protected:
	CxxUtilities::Mutex sendMutex;

protected:
	uint8_t sequenceNumber;
	double sendTimeoutCounter;
	size_t segmentIndex;
	CxxUtilities::Condition conditionForSendWait;
	CxxUtilities::Mutex mutexForNOfOutstandingPackets;
	CxxUtilities::Mutex mutexForRetryTimeoutCounters;

protected:
	// Sliding window related arrays
	double* retryTimeoutCounters;
	bool* packetHasBeenSent;
	bool* packetWasAcknowledged;
	size_t* retryCountsForSequenceNumber;

protected:
	class RetryTimerUpdater: public CxxUtilities::StoppableThread {
	private:
		SpaceWireRTEP* parent;

	public:
		RetryTimerUpdater(SpaceWireRTEP* parent) {
			this->parent = parent;
		}

	public:
		void run() {
			CxxUtilities::Condition c;
			while (!stopped) {
				if (parent->state == SpaceWireRTEPState::Open) {
					c.wait(parent->timeoutDurationForRetryTimers);
					parent->updateRetryTimers();
				}
			}
		}
	};

protected:
	RetryTimerUpdater* retryTimerUpdater;

private:
	/** Initializes the memory buffers used in sliding window control.
	 */
	void initializeSlidingWindowRelatedBuffers() {
		retryTimeoutCounters = new double[SpaceWireRProtocol::SizeOfSlidingWindow];
		packetHasBeenSent = new bool[SpaceWireRProtocol::SizeOfSlidingWindow];
		packetWasAcknowledged = new bool[SpaceWireRProtocol::SizeOfSlidingWindow];
		retryCountsForSequenceNumber = new size_t[SpaceWireRProtocol::SizeOfSlidingWindow];
	}

private:
	/** Finalizes (deletes) the memory buffers used in sliding window control.
	 */
	void finalizeSlidingWindowRelatedBuffers() {
		delete retryTimeoutCounters;
		delete packetHasBeenSent;
		delete packetWasAcknowledged;
		delete retryCountsForSequenceNumber;
	}

private:
	/*
	 void sendPacket(SpaceWireRPacket* packet, double timeoutDuration = DefaultTimeoutDurationInMs) {
	 sendMutex.lock();

	 //reset HeartBeat Timer
	 this->heartBeatTimer->resetHeartBeatTimer();

	 //configure SpaceWireRPacket instance
	 packet->setSequenceNumber(sequenceNumber);

	 using namespace std;
	 #ifdef DebugSpaceWireRTEP
	 cout << "SpaceWireRTEP::sendPacket() enetered." << endl;
	 cout << "SpaceWireRTEP::sendPacket() PacketType=" << packet->getPacketTypeAsString() << " SequenceNumber="
	 << packet->getSequenceNumberAs32bitInteger() << endl;
	 #endif

	 sendTimeoutCounter = 0;
	 nOfOutstandingPackets = 0;

	 bool thisPacketIsAcknowledged = false;

	 size_t index = 0;
	 //check timeout
	 if (sendTimeoutCounter > timeoutDuration) {
	 cout << "sendTimeoutCounter = " << dec << sendTimeoutCounter << "  timeoutDuration=" << timeoutDuration << endl;
	 //timeout occurs
	 malfunctioningTransportChannel();
	 sendMutex.unlock();
	 throw SpaceWireRTEPException(SpaceWireRTEPException::Timeout);
	 }
	 checkRetryTimerThenRetry();

	 //update counters
	 retryTimeoutCounters[sequenceNumber] = 0;
	 sequenceNumber++;
	 incrementNOfOutstandingPackets();

	 //send segment
	 try {
	 #ifdef DebugSpaceWireRTEP
	 cout << "SpaceWireRTEP::sendPacket() sending a segment sequence number=" << (uint32_t) packet->getSequenceNumber()
	 << " " << endl;
	 #endif
	 spwREngine->sendPacket(packet);
	 nSentSegments++;
	 packetHasBeenSent[packet->getSequenceNumber()] = true;
	 } catch (...) {
	 sendMutex.unlock();
	 this->malfunctioningSpaceWireIF();
	 throw SpaceWireRTEPException(SpaceWireRTEPException::SpaceWireIFIsNotWorking);
	 }

	 //check all sent packets were acknowledged
	 #ifdef DebugSpaceWireRTEP
	 cout << "SpaceWireRTEP::sendPacket() all segments were sent. Wait until acknowledged." << endl;
	 #endif
	 while (!allOngoingPacketesWereAcknowledged()) {
	 checkRetryTimerThenRetry();
	 conditionForSendWait.wait(DefaultWaitDurationInMsForCompletionCheck);
	 }
	 #ifdef DebugSpaceWireRTEP
	 cout << "SpaceWireRTEP::sendPacket() Completed." << endl;
	 #endif
	 sendMutex.unlock();
	 }
	 */

protected:
	virtual void sendPacket(SpaceWireRPacket* packet, double timeoutDuration = DefaultTimeoutDurationInMs/*ms*/)
			throw (SpaceWireRTEPException) =0;

protected:
	void sendPacketWithSpecifiedSequenceNumber(SpaceWireRPacket* packet, uint8_t specifiedSequenceNumber,
			double timeoutDuration = DefaultTimeoutDurationInMs/*ms*/) throw (SpaceWireRTEPException) {
		sendMutex.lock();

		//reset HeartBeat Timer
		this->heartBeatTimer->resetHeartBeatTimer();

		//configure SpaceWireRPacket instance
		packet->setSequenceNumber(sequenceNumber);

		using namespace std;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::sendPacket() enetered." << endl;
		cout << "SpaceWireRTEP::sendPacket() PacketType=" << packet->getPacketTypeAsString() << " SequenceNumber="
		<< packet->getSequenceNumberAs32bitInteger() << endl;
#endif

		sendTimeoutCounter = 0;
		nOfOutstandingPackets = 0;

		bool thisPacketIsAcknowledged = false;

		size_t index = 0;
		//check timeout
		if (sendTimeoutCounter > timeoutDuration) {
			cout << "sendTimeoutCounter = " << dec << sendTimeoutCounter << "  timeoutDuration=" << timeoutDuration << endl;
			//timeout occurs
			malfunctioningTransportChannel();
			sendMutex.unlock();
			throw SpaceWireRTEPException(SpaceWireRTEPException::Timeout);
		}
		checkRetryTimerThenRetry();

		//update counters
		retryTimeoutCounters[specifiedSequenceNumber] = 0;
		specifiedSequenceNumber++;
		incrementNOfOutstandingPackets();

		//send segment
		try {
#ifdef DebugSpaceWireRTEP
			cout << "SpaceWireRTEP::sendPacket() sending a segment sequence number=" << (uint32_t) packet->getSequenceNumber()
			<< " " << endl;
			cout << packet->toString() << endl;
#endif
			spwREngine->sendPacket(packet);
			nSentSegments++;
			packetHasBeenSent[packet->getSequenceNumber()] = true;
		} catch (...) {
			sendMutex.unlock();
			this->malfunctioningSpaceWireIF();
			throw SpaceWireRTEPException(SpaceWireRTEPException::SpaceWireIFIsNotWorking);
		}

		//check all sent packets were acknowledged
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::sendPacket() all segments were sent. Wait until acknowledged." << endl;
#endif
		while (!allOngoingPacketesWereAcknowledged()) {
			checkRetryTimerThenRetry();
			conditionForSendWait.wait(DefaultWaitDurationInMsForCompletionCheck);
		}
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::sendPacket() Completed." << endl;
#endif
		sendMutex.unlock();
	}

protected:
	void checkRetryTimerThenRetry() throw (SpaceWireRTEPException) {
		using namespace std;
		mutexForRetryTimeoutCounters.lock();
		for (size_t i = 0; i < this->slidingWindowSize; i++) {
			uint8_t index = (uint8_t) (this->slidingWindowFrom + i);
#ifdef DebugSpaceWireRTEP
			cout << "SpaceWireRTEP::checkRetryTimerThenRetry() Window=" << (size_t) index << " TimeoutCounter="
			<< retryTimeoutCounters[index] << endl;
#endif
			if (packetHasBeenSent[index] == true && packetWasAcknowledged[index] == false
					&& retryTimeoutCounters[index] > WaitDurationInMsForPacketRetransmission) {
#ifdef DebugSpaceWireRTEPDumpCriticalIncidents
				std::stringstream ss;
				ss << "SpaceWireRTEP::checkRetryTimerThenRetry() Timer expired for sequence number = " << dec << right
				<< (uint32_t) index << " !!!" << endl;
				CxxUtilities::TerminalControl::displayInRed(ss.str());
#endif
				nLostAckPackets++;
				retryTimeoutCounters[index] = 0;
				retryCountsForSequenceNumber[index]++;
				//check if retry is necessary
				if (retryCountsForSequenceNumber[index] > maxRetryCount) {
					mutexForRetryTimeoutCounters.unlock();
					this->malfunctioningTransportChannel();
					throw SpaceWireRTEPException(SpaceWireRTEPException::TooManyRetryFailures);
				}

				slidingWindowBuffer[index]->setSequenceNumber(index);
#ifdef DebugSpaceWireRTEP
				cout << "SpaceWireRTEP::checkRetryTimerThenRetry() Retry for sequence number=" << (uint32_t) index << " "
				<< slidingWindowBuffer[index]->getPacketTypeAsString() << " "
				<< (uint32_t) slidingWindowBuffer[index]->getSequenceNumber() << " "
				<< slidingWindowBuffer[index]->getSequenceFlagsAsString() << endl;
#endif
				//do retry
				spwREngine->sendPacket(slidingWindowBuffer[index]);
				nRetriedSegments++;
			}
		}
		mutexForRetryTimeoutCounters.unlock();
	}

private:
	void updateRetryTimers() {
		using namespace std;
		mutexForRetryTimeoutCounters.lock();
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::updateRetryTimers()" << endl;
#endif
		for (size_t i = 0; i < this->slidingWindowSize; i++) {
			uint8_t index = (uint8_t) (this->slidingWindowFrom + i);
			if (packetHasBeenSent[index] == true && packetWasAcknowledged[index] == false) {
#ifdef DebugSpaceWireRTEP
				cout << "SpaceWireRTEP::updateRetryTimers() TimeoutCounter[" << dec << (uint32_t) index << "]="
				<< retryTimeoutCounters[index] << endl;
#endif
				retryTimeoutCounters[index] += timeoutDurationForRetryTimers;
			}
		}
		mutexForRetryTimeoutCounters.unlock();
	}

protected:
	void incrementNOfOutstandingPackets() {
		mutexForNOfOutstandingPackets.lock();
		if (nOfOutstandingPackets < this->slidingWindowSize) {
			nOfOutstandingPackets++;
		} else {
			this->malfunctioningTransportChannel();
		}
		mutexForNOfOutstandingPackets.unlock();
	}

protected:
	void decrementNOfOutstandingPackets() {
		mutexForNOfOutstandingPackets.lock();
		if (nOfOutstandingPackets != 0) {
			nOfOutstandingPackets--;
		} else {
			this->malfunctioningTransportChannel();
		}
		mutexForNOfOutstandingPackets.unlock();
		conditionForSendWait.signal();
	}

protected:
	bool allOngoingPacketesWereAcknowledged() {
		if (nOfOutstandingPackets == 0) {
			return true;
		} else {
			return false;
		}
	}

protected:
	inline SpaceWireRPacket* getAvailablePacketInstance() {
		using namespace std;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::getAvailablePacketInstance() nOfOutstandingPackets=" << dec << nOfOutstandingPackets
		<< endl;
#endif
		if (nOfOutstandingPackets < this->slidingWindowSize) {
			SpaceWireRPacket* packet = slidingWindowBuffer[this->sequenceNumber];
			return packet;
		} else {
#ifdef DebugSpaceWireRTEP
			cout << "SpaceWireRTEP::getAvailablePacketInstance() Returns NULL!!!" << endl;
#endif
			return NULL;
		}
	}

protected:
	/** Slides the sliding window by 1 packet.
	 */
	void slideSlidingWindow() {
		using namespace std;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::slideSlidingWindow()" << endl;
#endif
		uint8_t n = this->slidingWindowFrom;
		while (packetHasBeenSent[n] == true && packetWasAcknowledged[n] == true) {
			packetHasBeenSent[n] = false;
			packetWasAcknowledged[n] = false;
			retryTimeoutCounters[n] = 0;
			retryCountsForSequenceNumber[n] = 0;
			n = (uint8_t) (n + 1);
		}
		this->slidingWindowFrom = n;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::slideSlidingWindow() slidingWindowFrom=" << (uint32_t) this->slidingWindowFrom << endl;
#endif
	}

	/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	 * sendPacket interface
	 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

private:
	virtual void malfunctioningTransportChannel() =0;

private:
	virtual void malfunctioningSpaceWireIF() =0;

public:
	virtual std::string toString() = 0;

public:
	void closeDueToSpaceWireIFFailure() {
		this->state = SpaceWireRTEPState::Closed;
	}

public:
	/** Internal class used for HeartBeat emission.
	 */
	class HeartBeatTimer: public CxxUtilities::Condition, public CxxUtilities::StoppableThread {
	private:
		SpaceWireRTEP* parent;
		double timerConstantInMilliSec; //ms
		static const size_t timerDivider = 10;
		double timerCounter;
	public:
		/** Constructs a HeartBeatTimer instance.
		 * @param[in] parent parent SpaceWire-R TEP instance (either of SpaceWireRTransmitTEP or SpaceWireRReceiveTEP)
		 * @param[in] timerConstantInMilliSec timer expiration constant in milli second
		 */
		HeartBeatTimer(SpaceWireRTEP* parent, double timerConstantInMilliSec = 1000) {
			this->parent = parent;
			this->timerConstantInMilliSec = timerConstantInMilliSec;
		}

	public:
		void setHeartBeatTimerConstant(double timerConstantInMilliSec) {
			this->timerConstantInMilliSec = timerConstantInMilliSec;
		}

	public:
		void run() {
			double waitDuration = this->timerConstantInMilliSec / this->timerDivider;
			while (!stopped) {
				this->CxxUtilities::Condition::wait(waitDuration);
				timerCounter += waitDuration;
				if (timerCounter > timerConstantInMilliSec) {
					timerCounter = 0;
					if (parent->isOpen()) {
						parent->emitHeartBeatPacket();
					}
				}
			}
		}

	public:
		/** Resets the HeartBeat Transmit timer.
		 * TransmitTEP and ReceiveTEP should call this method whenever
		 * a packet (either Data, Control, or Ack) is transmitted (TransmitTEP)
		 * or received (ReceiveTEP).
		 */
		void resetHeartBeatTimer() {
			timerCounter = 0;
		}
	};

protected:
	size_t nReceivedHeartBeatPackets;
	size_t nReceivedHeartBeatAckPackets;
	size_t nTransmittedHeartBeatPackets;
	size_t nTransmittedHeartBeatAckPackets;

private:
	bool isHeartBeatEmissionEnabled;
	bool doNotRespondToReceivedHeartBeatPacket_;

protected:
	HeartBeatTimer* heartBeatTimer;
	SpaceWireRPacket* heartBeatAckPacket;

public:
	void enableHeartBeat() {
		using namespace std;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::enableHeartBeat()" << endl;
#endif
		isHeartBeatEmissionEnabled = true;
		if (heartBeatTimer->isStopped()) {
#ifdef DebugSpaceWireRTEP
			cout << "SpaceWireRTEP::enableHeartBeat() starting child thread" << endl;
#endif
			heartBeatTimer->start();
		}
	}

public:
	void disableHeartBeat() {
		isHeartBeatEmissionEnabled = false;
		heartBeatTimer->stop();
	}

private:
	/** Emits a HeartBeat packet.
	 */
	void emitHeartBeatPacket() throw (SpaceWireRTEPException) {
		sendMutex.lock();
		using namespace std;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::emitHeartBeatPacket() entered." << endl;
#endif
		SpaceWireRPacket* heartBeatPacket;
		size_t nTrials = 0;
		heartBeatPacket = this->getAvailablePacketInstance();
		if (heartBeatPacket == NULL) { //if no room
			sendMutex.unlock();
			throw SpaceWireRTEPException(SpaceWireRTEPException::NoRoomInSlidingWindow);
		}
		heartBeatPacket->setPacketType(SpaceWireRPacketType::HeartBeatPacket);
		heartBeatPacket->clearPayload();
		heartBeatPacket->setCompleteSegmentFlag();
		sendPacket(heartBeatPacket);
		nTransmittedHeartBeatPackets++;
#ifdef DebugSpaceWireRTEP
		cout << "SpaceWireRTEP::emitHeartBeatPacket() HeartBeat packet has been transmitted and acknowledged." << endl;
#endif
		sendMutex.unlock();
	}

public:
	void setHeartBeatTimerConstant(double timerConstantInMilliSec) {
		this->heartBeatTimer->setHeartBeatTimerConstant(timerConstantInMilliSec);
	}

public:
	void doNotRespondToReceivedHeartBeatPacket() {
		doNotRespondToReceivedHeartBeatPacket_ = true;
	}

public:
	void respondToReceivedHeartBeatPacket() {
		doNotRespondToReceivedHeartBeatPacket_ = false;
	}

public:
	bool shouldRespondToReceivedHeartBeatPacket() {
		return !doNotRespondToReceivedHeartBeatPacket_;
	}

private:
	bool isFlowControlEnabled_;


protected:
	SpaceWireRPacket* flowControlPacket;
	size_t nTransmittedFlowControlPackets=0;
	size_t nReceivedFlowControlPackets=0;

public:
	void enableFlowControl() {
		isFlowControlEnabled_ = true;
	}

public:
	void disableFlowControl() {
		isFlowControlEnabled_ = false;
	}

public:
	bool isFlowControlEnabled() {
		return isFlowControlEnabled_;
	}

};

#endif /* SPACEWIRERTEP_HH_ */
