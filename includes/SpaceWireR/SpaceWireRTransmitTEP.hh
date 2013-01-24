/*
 * SpaceWireRTransmitTEP.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERTRANSMITTEP_HH_
#define SPACEWIRERTRANSMITTEP_HH_

#include "SpaceWireR/SpaceWireRTEP.hh"

class SpaceWireRTransmitTEP: public SpaceWireRTEP, public CxxUtilities::StoppableThread {

public:
	SpaceWireRTransmitTEP(SpaceWireREngine* spwREngine, uint8_t channel, //
			uint8_t destinationLogicalAddress, std::vector<uint8_t> destinationSpaceWireAddress, //
			uint8_t sourceLogicalAddress, std::vector<uint8_t> sourceSpaceWireAddress) :
			SpaceWireRTEP(SpaceWireRTEPType::TransmitTEP, spwREngine, channel) {
		this->destinationLogicalAddress = destinationLogicalAddress;
		this->destinationSpaceWireAddress = destinationSpaceWireAddress;
		this->sourceLogicalAddress = destinationLogicalAddress;
		this->sourceSpaceWireAddress = destinationSpaceWireAddress;
	}

public:
	virtual ~SpaceWireRTransmitTEP() {
	}

	/* --------------------------------------------- */
public:
	static const double WaitDurationInMsForClosedLoop = 100; //m1s

	static const double DefaultTimeoutDurationInMilliSec = 1000; //ms
	static const double WaitDurationInMsForClosingLoop = 100; //ms
	static const double WaitDurationInMsForEnabledLoop = 100; //m1s
	static const double WaitDurationInMsForPacketRetransmission = 500; //ms

private:
	uint8_t sourceLogicalAddress;
	std::vector<uint8_t> sourceSpaceWireAddress;
	uint8_t destinationLogicalAddress;
	std::vector<uint8_t> destinationSpaceWireAddress;

private:
	std::vector<CxxUtilities::Condition*> outstandingPacketTimers;
	std::vector<SpaceWireRPacket*> outstandingPackets;
	std::stack<CxxUtilities::Condition*> retryTimerInstancePool;
	std::stack<SpaceWireRPacket*> packetInstancePool;

private:
	std::vector<size_t> retryCountsForSequenceNumber;

private:

	/* --------------------------------------------- */

public:
	void open(double timeoutDrationInMilliSec = DefaultTimeoutDurationInMilliSec) throw (SpaceWireRTEPException) {
		if (this->state == SpaceWireRTEPState::Closed) {
			this->state = SpaceWireRTEPState::Enabled;
		} else if (state == SpaceWireRTEPState::Closing) {
			throw SpaceWireRTEPException(SpaceWireRTEPException::IlleagalOpenDirectionWhileClosingASocket);
		}
		prepareRetryTimerInstances();
		prepareSpaceWireRPacketInstances();
		initializeRetryCounts();
		registerMeToSpaceWireREngine();
	}

public:
	void close() throw (SpaceWireRTEPException) {
		if (state == SpaceWireRTEPState::Enabled || state == SpaceWireRTEPState::Open) {
			this->state = SpaceWireRTEPState::Closing;
			performClosingProcess();
		}
		initializeSlidingWindow();
		initializeRetryCounts();
	}

private:
	void closed() {
		unregisterMeToSpaceWireREngine();
	}

private:
	void registerMeToSpaceWireREngine() {
		spwREngine->registerTransmitTEP(channel, &receivedPackets, &packetArrivalNotifier);
	}

private:
	void unregisterMeToSpaceWireREngine() {
		spwREngine->unregisterTransmitTEP(channel);
	}

private:
	void initializeRetryCounts() {
		retryCountsForSequenceNumber.clear();
		retryCountsForSequenceNumber.resize(MaxOfSlidingWindow, 0);
	}

private:
	void prepareRetryTimerInstances() {
		while (retryTimerInstancePool.size() < this->slidingWindowSize) {
			retryTimerInstancePool.push(new CxxUtilities::Condition);
		}
	}

private:
	void prepareSpaceWireRPacketInstances() {
		while (packetInstancePool.size() < this->slidingWindowSize) {
			SpaceWireRPacket* packet = new SpaceWireRPacket;
			//set common parameters
			packet->setChannelNumber(channel);
			packet->setDestinationLogicalAddress(destinationLogicalAddress);
			packet->setDestinationSpaceWireAddress(destinationSpaceWireAddress);
			packet->setSecondaryHeaderFlag(SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed);
			packet->setPrefix(this->sourceSpaceWireAddress);
			packet->setSourceSpaceWireLogicalAddress(this->sourceLogicalAddress);
			packetInstancePool.push(packet);

			/* Note */
			/* The following fields should be set packet by packet. */
			//- sequence flag
			//- packet type
			//- payload length
			//- sequence number
		}
	}

private:
	SpaceWireRPacket* getAvailablePacketInstance() {
		if (packetInstancePool.size() != 0) {
			SpaceWireRPacket* packet = packetInstancePool.top();
			packetInstancePool.pop();
			return packet;
		} else {
			return NULL;
		}
	}

private:
	void returnPacketInstance(SpaceWireRPacket* packet) {
		packetInstancePool.push(packet);
	}

private:
	CxxUtilities::Condition* getAvailableRetryTimerInstance(){
		if(retryTimerInstancePool.size()!=0){
			CxxUtilities::Condition* timer=retryTimerInstancePool.top();
			retryTimerInstancePool.pop();
			return timer;
		}else{
			return NULL;
		}
	}

private:
	void returnRetryTimerInstance(CxxUtilities::Condition* timer){
		retryTimerInstancePool.push(timer);
	}

private:
	void malfunctioningTransportChannel() {
		this->state = SpaceWireRTEPState::Closed;
		closed();
		//todo
		//add message which should be sent to user
	}

private:
	void malfunctioningSpaceWireIF() {
		this->state = SpaceWireRTEPState::Closed;
		closed();
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
				sendOpenCommand();
				waitTimer.wait(WaitDurationInMsForEnabledLoop);
				break;

			case SpaceWireRTEPState::Open:
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
	void performClosingProcess() {
		//todo
	}

private:
	void sendOpenCommand() {
		SpaceWireRPacket* packet = getAvailablePacketInstance();
		if (packet == NULL) {
			malfunctioningTransportChannel();
		}
		CxxUtilities::Condition* timer=getAvailableRetryTimerInstance();

		//set sequence flag
		packet->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

		//set packet type
		packet->setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);

		//set payload length
		packet->setPayloadLength(0x00);

		//set sequence number
		packet->setSequenceNumber(0x00);

		//send
		spwREngine->sendPacket(packet);



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
		packet->setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);

		//set payload length
		packet->setPayloadLength(0x00);

		//set sequence number
		packet->setSequenceNumber(0x00);

		spwREngine->sendPacket(packet);
	}

};

#endif /* SPACEWIRERTRANSMITTEP_HH_ */
