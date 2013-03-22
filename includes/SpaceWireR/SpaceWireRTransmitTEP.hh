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
 * SpaceWireRTransmitTEP.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERTRANSMITTEP_HH_
#define SPACEWIRERTRANSMITTEP_HH_

#include "SpaceWireR/SpaceWireRTEP.hh"

#undef DebugSpaceWireRTransmitTEP
#define DebugSpaceWireRTransmitTEPDumpCriticalIncidents

class SpaceWireRTransmitTEP: public SpaceWireRTEP, public CxxUtilities::StoppableThread {
private:
	class RetryTimerUpdater: public CxxUtilities::StoppableThread {
	private:
		SpaceWireRTransmitTEP* parent;

	public:
		RetryTimerUpdater(SpaceWireRTransmitTEP* parent) {
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

public:
	SpaceWireRTransmitTEP(SpaceWireREngine* spwREngine, uint16_t channel, //
			uint8_t destinationLogicalAddress, std::vector<uint8_t> destinationSpaceWireAddress, //
			uint8_t sourceLogicalAddress, std::vector<uint8_t> sourceSpaceWireAddress) :
			SpaceWireRTEP(SpaceWireRTEPType::TransmitTEP, spwREngine, channel) {
		retryTimerUpdater = new RetryTimerUpdater(this);
		this->destinationLogicalAddress = destinationLogicalAddress;
		this->destinationSpaceWireAddress = destinationSpaceWireAddress;
		this->sourceLogicalAddress = sourceLogicalAddress;
		this->sourceSpaceWireAddress = sourceSpaceWireAddress;
		this->maximumSegmentSize = DefaultMaximumSegmentSize;
		this->initializeSlidingWindowRelatedBuffers();
		this->initializeCounters();
		this->prepareSpaceWireRPacketInstances();
		this->nOfOutstandingPackets = 0;
		this->start();
	}

public:
	virtual ~SpaceWireRTransmitTEP() {
		this->stop();
		this->waitUntilRunMethodComplets();
		this->finalizeSlidingWindowRelatedBuffers();
	}

	/* --------------------------------------------- */
public:
	static const double WaitDurationInMsForClosedLoop = 100; //m1s
	static const double DefaultTimeoutDurationInMilliSec = 1000; //ms
	static const double WaitDurationInMsForClosingLoop = 100; //ms
	static const double WaitDurationInMsForEnabledLoop = 100; //m1s
	static const double WaitDurationInMsForPacketRetransmission = 2000; //ms
	static const double DefaultTimeoutDurationInMsForOpen = 500; //ms

public:
	//statistics counters
	size_t nRetriedSegments;
	size_t nSentSegments;
	size_t nSentUserData;
	size_t nSentUserDataInBytes;
	size_t nLostAckPackets;
	size_t nOfOutstandingPackets;

private:
	uint8_t sourceLogicalAddress;
	std::vector<uint8_t> sourceSpaceWireAddress;
	uint8_t destinationLogicalAddress;
	std::vector<uint8_t> destinationSpaceWireAddress;

private:
	// Sliding window related arrays
	double* retryTimeoutCounters;
	bool* packetHasBeenSent;
	bool* packetWasAcknowledged;
	size_t* retryCountsForSequenceNumber;

	RetryTimerUpdater* retryTimerUpdater;
	bool openCommandAcknowledged;
	bool closeCommandAcknowledged;
	/* --------------------------------------------- */

private:
	void initializeCounters() {
		this->nRetriedSegments = 0;
		this->nSentSegments = 0;
		this->nSentUserData = 0;
		this->nSentUserDataInBytes = 0;
		this->nLostAckPackets = 0;
	}

public:
	void open(double timeoutDrationInMilliSec = DefaultTimeoutDurationInMilliSec) throw (SpaceWireRTEPException) {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::open()" << endl;
#endif
		openCommandAcknowledged = false;
		closeCommandAcknowledged = false;
		if (this->state == SpaceWireRTEPState::Closed) {
			this->state = SpaceWireRTEPState::Enabled;
		} else if (state == SpaceWireRTEPState::Closing) {
			throw SpaceWireRTEPException(SpaceWireRTEPException::IlleagalOpenDirectionWhileClosingASocket);
		}
		this->sequenceNumber = 0;
		this->nOfOutstandingPackets = 0;
		this->state = SpaceWireRTEPState::Enabled;
		initializeRetryCounts();
		registerMeToSpaceWireREngine();
		retryTimerUpdater->start();
		sendOpenCommand();
	}

public:
	void close() throw (SpaceWireRTEPException) {
		if (state == SpaceWireRTEPState::Enabled || state == SpaceWireRTEPState::Open) {
			this->state = SpaceWireRTEPState::Closing;
			performClosingProcess();
		}
		initializeSlidingWindow();
		initializeRetryCounts();
		retryTimerUpdater->stop();
		openCommandAcknowledged = false;
		closeCommandAcknowledged = false;
	}

private:
	void closed() {
		unregisterMeToSpaceWireREngine();
	}

private:
	void registerMeToSpaceWireREngine() {
		spwREngine->registerTransmitTEP(this);
	}

private:
	void unregisterMeToSpaceWireREngine() {
		spwREngine->unregisterTransmitTEP(channel);
	}

private:
	void initializeRetryCounts() {
		for (size_t i = 0; i < SpaceWireRProtocol::SizeOfSlidingWindow; i++) {
			retryCountsForSequenceNumber[i] = 0;
		}
	}

private:
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
	inline SpaceWireRPacket* getAvailablePacketInstance() {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::getAvailablePacketInstance() nOfOutstandingPackets=" << dec << nOfOutstandingPackets
		<< endl;
#endif
		if (nOfOutstandingPackets < this->slidingWindowSize) {
			SpaceWireRPacket* packet = slidingWindowBuffer[this->sequenceNumber];
			return packet;
		} else {
#ifdef DebugSpaceWireRTransmitTEP
			cout << "SpaceWireRTransmitTEP::getAvailablePacketInstance() Returns NULL!!!" << endl;
#endif
			return NULL;
		}
	}

private:
	void malfunctioningTransportChannel() {
		this->state = SpaceWireRTEPState::Closed;
		closed();
		//todo
		//add message which should be sent to user
		using namespace std;
		cout << "SpaceWireRTransmitTEP::malfunctioningTransportChannel() !!! MALFUNCTIONING TEP !!!" << endl;
		cout << ((SpaceWireRTEPInterface*) (NULL))->channel << endl;
		//debug
		throw 1;
	}

private:
	void malfunctioningSpaceWireIF() {
		this->state = SpaceWireRTEPState::Closed;
		closed();
		//todo
		//send message to user that SpaceWire IF is failing
		//probably, Action mechanism is suitable.
		using namespace std;
		cout << "SpaceWireRTransmitTEP::malfunctioningSpaceWireIF() !!! MALFUNCTIONING SpaceWire IF !!!" << endl;
	}

private:
	void consumeReceivedPackets() {
		using namespace std;
		while (receivedPackets.size() != 0) {
#ifdef DebugSpaceWireRTransmitTEP
			cout << "SpaceWireRTransmitTEP::consumeReceivedPackets() process one packet." << endl;
#endif
			SpaceWireRPacket* packet = this->popReceivedSpaceWireRPacket();
			if (packet->isAckPacket()) {
				processAckPacket(packet);
				delete packet;
				continue;
			} else {
#ifdef DebugSpaceWireRTransmitTEP
				cout << "SpaceWireRTransmitTEP::consumeReceivedPackets() invalid packet. Stops this TEP." << endl;
#endif
				malfunctioningTransportChannel();
				delete packet;
				return;
			}
		}
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
				waitTimer.wait(WaitDurationInMsForEnabledLoop);
				consumeReceivedPackets();
				break;

			case SpaceWireRTEPState::Open:
				while (this->state == SpaceWireRTEPState::Open && !stopped) {
					if (receivedPackets.size() != 0) {
						consumeReceivedPackets();
					}
					if (SpaceWireRTEPState::Open) {
						packetArrivalNotifier.wait(WaitDurationForPacketReceiveLoop);
						if (receivedPackets.size() == 0) {
							//increment sendTimeoutCounter
							sendTimeoutCounter += WaitDurationForPacketReceiveLoop;
						}
						conditionForSendWait.signal();
					}
				}
				break;

			case SpaceWireRTEPState::Closing:
				break;

			default:
				this->stop();
				break;
			}
		}
	}

private:
	size_t maximumSegmentSize;
	static const size_t DefaultMaximumSegmentSize = 256;
	static const size_t DefaultMaximumRetryCount = 4;
	size_t maxRetryCount = DefaultMaximumRetryCount;

public:
	inline void setSegmentSize(size_t size) {
		this->maximumSegmentSize = size;
	}

private:
	void processAckPacket(SpaceWireRPacket* packet) {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::processAckPacket() sequence number = " << (uint32_t) packet->getSequenceNumber()
		<< endl;
#endif
		uint8_t sequenceNumberOfThisPacket = packet->getSequenceNumber();
		if (packetHasBeenSent[sequenceNumberOfThisPacket] == true) {
			packetWasAcknowledged[sequenceNumberOfThisPacket] = true;
			decrementNOfOutstandingPackets();
		}
		if (packet->isControlAckPacket() && slidingWindowBuffer[sequenceNumberOfThisPacket]->isControlPacketOpenCommand()) {
			openCommandAcknowledged = true;
		}
		if (packet->isControlAckPacket()
				&& slidingWindowBuffer[sequenceNumberOfThisPacket]->isControlPacketCloseCommand()) {
			closeCommandAcknowledged = true;
		}
		slideSlidingWindow();
	}

private:
	void slideSlidingWindow() {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::slideSlidingWindow()" << endl;
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
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::slideSlidingWindow() slidingWindowFrom=" << (uint32_t) this->slidingWindowFrom
		<< endl;
#endif
	}

private:
	CxxUtilities::Mutex sendMutex;

private:
	void initializeSlidingWindowRelatedBuffers() {
		retryTimeoutCounters = new double[SpaceWireRProtocol::SizeOfSlidingWindow];
		packetHasBeenSent = new bool[SpaceWireRProtocol::SizeOfSlidingWindow];
		packetWasAcknowledged = new bool[SpaceWireRProtocol::SizeOfSlidingWindow];
		retryCountsForSequenceNumber = new size_t[SpaceWireRProtocol::SizeOfSlidingWindow];
	}

private:
	void finalizeSlidingWindowRelatedBuffers() {
		delete retryTimeoutCounters;
		delete packetHasBeenSent;
		delete packetWasAcknowledged;
		delete retryCountsForSequenceNumber;
	}

private:
	static const double DefaultTimeoutDurationInMs = 1000; //ms
	static const double DefaultWaitDurationInMsForCompletionCheck = 50; //ms
	static const double DefaultWaitDurationInMsForSendSegment = 500; //ms
	static const double DefaultTimeoutDurationInMsForRetryTimers = 50; //ms
	double timeoutDurationForRetryTimers = DefaultTimeoutDurationInMsForRetryTimers; //ms

private:
	uint8_t sequenceNumber;
	double sendTimeoutCounter;
	size_t segmentIndex;
	CxxUtilities::Condition conditionForSendWait;
	CxxUtilities::Mutex mutexForNOfOutstandingPackets;
	CxxUtilities::Mutex mutexForRetryTimeoutCounters;

private:
	void updateRetryTimers() {
		using namespace std;
		mutexForRetryTimeoutCounters.lock();
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::updateRetryTimers()" << endl;
#endif
		for (size_t i = 0; i < this->slidingWindowSize; i++) {
			uint8_t index = (uint8_t) (this->slidingWindowFrom + i);
			if (packetHasBeenSent[index] == true && packetWasAcknowledged[index] == false) {
#ifdef DebugSpaceWireRTransmitTEP
				cout << "SpaceWireRTransmitTEP::updateRetryTimers() TimeoutCounter[" << dec << (uint32_t)index << "]="
				<< retryTimeoutCounters[index] << endl;
#endif
				retryTimeoutCounters[index] += timeoutDurationForRetryTimers;
			}
		}
		mutexForRetryTimeoutCounters.unlock();
	}

private:
	void checkRetryTimerThenRetry() throw (SpaceWireRTEPException) {
		using namespace std;
		mutexForRetryTimeoutCounters.lock();
		for (size_t i = 0; i < this->slidingWindowSize; i++) {
			uint8_t index = (uint8_t) (this->slidingWindowFrom + i);
#ifdef DebugSpaceWireRTransmitTEP
			cout << "SpaceWireRTransmitTEP::checkRetryTimerThenRetry() TimeoutCounter=" << retryTimeoutCounters[index]
			<< endl;
#endif
			if (packetHasBeenSent[index] == true && packetWasAcknowledged[index] == false
					&& retryTimeoutCounters[index] > WaitDurationInMsForPacketRetransmission) {
#ifdef DebugSpaceWireRTransmitTEPDumpCriticalIncidents
				std::stringstream ss;
				ss << "SpaceWireRTransmitTEP::checkRetryTimerThenRetry() Timer expired for sequence number = " << dec << right
				<< (uint32_t) index << " !!!" << endl;
				CxxUtilities::TerminalControl::displayInRed(ss.str());
				nLostAckPackets++;
#endif
				retryTimeoutCounters[index] = 0;
				retryCountsForSequenceNumber[index]++;
				//check if retry is necessary
				if (retryCountsForSequenceNumber[index] > maxRetryCount) {
					mutexForRetryTimeoutCounters.unlock();
					this->malfunctioningTransportChannel();
					throw SpaceWireRTEPException(SpaceWireRTEPException::TooManyRetryFailures);
				}

				slidingWindowBuffer[index]->setSequenceNumber(index);
#ifdef DebugSpaceWireRTransmitTEP
				cout << "SpaceWireRTransmitTEP::checkRetryTimerThenRetry() Retry for sequence number=" << (uint32_t) index
				<< " " << (uint32_t)slidingWindowBuffer[index]->getSequenceNumber() << " ";
				if (slidingWindowBuffer[index]->isFirstSegment()) {
					cout << "The First segment." << endl;
				}
				if (slidingWindowBuffer[index]->isContinuedSegment()) {
					cout << "Continued segment." << endl;
				}
				if (slidingWindowBuffer[index]->isLastSegment()) {
					cout << "The Last segment." << endl;
				}
				if (slidingWindowBuffer[index]->isCompleteSegment()) {
					cout << "Complete segment." << endl;
				}
#endif
				//do retry
				spwREngine->sendPacket(slidingWindowBuffer[index]);
				nRetriedSegments++;
			}
		}
		mutexForRetryTimeoutCounters.unlock();
	}

public:
	void send(std::vector<uint8_t>* data, double timeoutDuration = DefaultTimeoutDurationInMs)
			throw (SpaceWireRTEPException) {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::send()" << endl;
#endif
		sendMutex.lock();
		sendTimeoutCounter = 0;
		nOfOutstandingPackets = 0;
		//slideSlidingWindow();
		//sequenceNumber = this->getSlidingWindowFrom();
		size_t dataSize = data->size();
		size_t remainingSize = dataSize;
		size_t payloadSize;
		size_t index = 0;
		bool isFirst = true;
		while (remainingSize != 0 && this->state == SpaceWireRTEPState::Open) {
			//check timeout
			if (sendTimeoutCounter > timeoutDuration) {
				cout << "sendTimeoutCounter = " << dec << sendTimeoutCounter << "  timeoutDuration=" << timeoutDuration << endl;
				//timeout occurs
				malfunctioningTransportChannel();
				sendMutex.unlock();
				throw SpaceWireRTEPException(SpaceWireRTEPException::Timeout);
			}
			checkRetryTimerThenRetry();

			if (this->maximumSegmentSize < remainingSize) {
				payloadSize = this->maximumSegmentSize;
			} else {
				payloadSize = remainingSize;
			}

			//check if there is room in sliding window
			SpaceWireRPacket* packet = getAvailablePacketInstance();
			if (packet == NULL) { //if no room
				//wait then return
				conditionForSendWait.wait(DefaultWaitDurationInMsForSendSegment);
				continue;
			}

			//configure SpaceWireRPacket instance
			if (isFirst) {
				packet->setFirstSegmentFlag();
				isFirst = false;
			} else {
				packet->setContinuedSegmentFlag();
			}
			packet->setSequenceNumber(sequenceNumber);
			packet->setDataPacketFlag();
			packet->setPayload(data, index, payloadSize);

			//update counters
			remainingSize -= payloadSize;
			retryTimeoutCounters[sequenceNumber] = 0;
			index += payloadSize;
			sequenceNumber++;
			incrementNOfOutstandingPackets();

			//set LastFlag if necessary
			if (remainingSize == 0) {
				packet->setLastSegmentFlag();
			}

			//send segment
			try {
#ifdef DebugSpaceWireRTransmitTEP
				cout << "SpaceWireRTransmitTEP::send() sending a segment sequence number="
				<< (uint32_t) packet->getSequenceNumber() << " ";
				if (packet->isFirstSegment()) {
					cout << "The First segment." << endl;
				}
				if (packet->isContinuedSegment()) {
					cout << "Continued segment." << endl;
				}
				if (packet->isLastSegment()) {
					cout << "The Last segment." << endl;
				}
				if (packet->isCompleteSegment()) {
					cout << "Complete segment." << endl;
				}
#endif
				spwREngine->sendPacket(packet);
				nSentSegments++;
				packetHasBeenSent[packet->getSequenceNumber()] = true;
				//slidingWindowBuffer[packet->getSequenceNumber()] = packet;
			} catch (...) {
				sendMutex.unlock();
				this->malfunctioningSpaceWireIF();
				throw SpaceWireRTEPException(SpaceWireRTEPException::SpaceWireIFIsNotWorking);
			}
		}

		//check all sent packets were acknowledged
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::send() all segments were sent. Wait until acknowledged." << endl;
#endif

		while (!allOngoingPacketesWereAcknowledged()) {
			checkRetryTimerThenRetry();
			conditionForSendWait.wait(DefaultWaitDurationInMsForCompletionCheck);
		}
		nSentUserData++;
		nSentUserDataInBytes += data->size();
		sendMutex.unlock();
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::send() Completed." << endl;
#endif
	}

private:
	void incrementNOfOutstandingPackets() {
		mutexForNOfOutstandingPackets.lock();
		if (nOfOutstandingPackets < this->slidingWindowSize) {
			nOfOutstandingPackets++;
		} else {
			this->malfunctioningTransportChannel();
		}
		mutexForNOfOutstandingPackets.unlock();
	}

private:
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

private:
	bool allOngoingPacketesWereAcknowledged() {
		if (nOfOutstandingPackets == 0) {
			return true;
		} else {
			return false;
		}
	}

private:
	void performClosingProcess() {
		sendCloseCommand();
	}

private:
	void sendOpenCommand() throw (SpaceWireRTEPException) {
		using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
		cout << "SpaceWireRTransmitTEP::sendOpenCommand()" << endl;
#endif

		//initialize
		initializeRetryCounts();

		//get packet instance
		SpaceWireRPacket* packet = slidingWindowBuffer[0];
		nOfOutstandingPackets++;
		if (packet == NULL) {
#ifdef DebugSpaceWireRTransmitTEP
			cout << "SpaceWireRTransmitTEP::sendOpenCommand() no packet instance available." << endl;
#endif
			malfunctioningTransportChannel();
		}

		//set sequence flag
		packet->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

		//set packet type
		packet->setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);

		//set payload length
		packet->setPayloadLength(0x00);

		//set sequence number
		packet->setSequenceNumber(0x00);
		this->sequenceNumber = 0;
		openCommandAcknowledged = false;
		while (!openCommandAcknowledged) {
#ifdef DebugSpaceWireRTransmitTEP
			cout << "SpaceWireRTransmitTEP::sendOpenCommand() Sending open command." << endl;
#endif
			//send
			spwREngine->sendPacket(packet);
			packetHasBeenSent[sequenceNumber] = true;
			conditionForSendWait.wait(DefaultTimeoutDurationInMsForOpen);
			retryCountsForSequenceNumber[sequenceNumber]++;
			if (retryCountsForSequenceNumber[sequenceNumber] > maxRetryCount) {
				throw SpaceWireRTEPException(SpaceWireRTEPException::OpenFailed);
			}
		}
		//update state if open command is acknowledged
		this->state = SpaceWireRTEPState::Open;
		this->sequenceNumber = 1;
	}

private:
	void sendCloseCommand() {
		SpaceWireRPacket* packet = getAvailablePacketInstance();
		if (packet == NULL) {
			malfunctioningTransportChannel();
		}

		//set sequence flag
		packet->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

		//set packet type
		packet->setPacketType(SpaceWireRPacketType::ControlPacketCloseCommand);

		//set payload length
		packet->setPayloadLength(0x00);

		//set sequence number
		packet->setSequenceNumber(0x00);
		this->sequenceNumber = 0;
		closeCommandAcknowledged = false;
		while (!closeCommandAcknowledged) {
			//send
			spwREngine->sendPacket(packet);
			packetHasBeenSent[sequenceNumber] = true;
			conditionForSendWait.wait(DefaultTimeoutDurationInMsForOpen);
			retryCountsForSequenceNumber[sequenceNumber]++;
			if (retryCountsForSequenceNumber[sequenceNumber] > maxRetryCount) {
				break;
			}
		}
		//update state
		this->state = SpaceWireRTEPState::Closed;
	}

public:
	std::string toString() {
		using namespace std;
		std::stringstream ss;
		ss << "---------------------------------------------" << endl;
		ss << "SpaceWireRTransmitTEP" << endl;
		ss << "State                : " << SpaceWireRTEPState::toString(this->state) << endl;
		ss << "slidingWindowFrom    : " << dec << (uint32_t) this->slidingWindowFrom << endl;
		ss << "slidingWindowSize    : " << dec << (uint32_t) this->slidingWindowSize << endl;
		ss << "nOfOutstandingPckts  : " << dec << (uint32_t) this->nOfOutstandingPackets << endl;
		ss << "receivedPackets.size : " << dec << receivedPackets.size() << endl;
		ss << "Counters:" << endl;
		ss << "nSentUserData        : " << dec << nSentUserData << endl;
		ss << "nSentUserDataInBytes : " << dec << nSentUserDataInBytes / 1024 << "kB" << endl;
		ss << "nSentSegments        : " << dec << nSentSegments << endl;
		ss << "nLostAckPackets:     : " << dec << nLostAckPackets << endl;
		ss << "---------------------------------------------" << endl;
		return ss.str();
	}

};

#endif /* SPACEWIRERTRANSMITTEP_HH_ */
