#ifndef SPACEWIRERTRANSMITTEP_HH_
#define SPACEWIRERTRANSMITTEP_HH_

#include "spacewirer/spacewirertep.hh"

//#define DebugSpaceWireRTransmitTEP
//#define DebugSpaceWireRTransmitTEPDumpCriticalIncidents
#undef DebugSpaceWireRTransmitTEP
#undef DebugSpaceWireRTransmitTEPDumpCriticalIncidents

class SpaceWireRTransmitTEP : public SpaceWireRTEP, public CxxUtilities::StoppableThread {
 public:
  SpaceWireRTransmitTEP(SpaceWireREngine* spwREngine, u16 channel,                                  //
                        u8 destinationLogicalAddress, std::vector<u8> destinationSpaceWireAddress,  //
                        u8 sourceLogicalAddress, std::vector<u8> sourceSpaceWireAddress)
      : SpaceWireRTEP(SpaceWireRTEPType::TransmitTEP, spwREngine, channel) {
    this->destinationLogicalAddress = destinationLogicalAddress;
    this->destinationSpaceWireAddress = destinationSpaceWireAddress;
    this->sourceLogicalAddress = sourceLogicalAddress;
    this->sourceSpaceWireAddress = sourceSpaceWireAddress;
    this->maximumSegmentSize = DefaultMaximumSegmentSize;
    this->initializeCounters();
    this->prepareSpaceWireRPacketInstances();
    this->start();
  }

 public:
  virtual ~SpaceWireRTransmitTEP() {
    this->stop();
    this->waitUntilRunMethodComplets();
  }

  /* --------------------------------------------- */
 public:
  static constexpr double WaitDurationInMsForClosedLoop = 100;      // ms
  static constexpr double DefaultTimeoutDurationInMilliSec = 1000;  // ms
  static constexpr double WaitDurationInMsForClosingLoop = 100;     // ms
  static constexpr double WaitDurationInMsForEnabledLoop = 100;     // ms
  static constexpr double DefaultTimeoutDurationInMsForOpen = 500;  // ms

 private:
  bool openCommandAcknowledged;
  bool closeCommandAcknowledged;

 private:
  u8 maximumAcceptableSequenceNumber;
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
  void open(double timeoutDrationInMilliSec = DefaultTimeoutDurationInMilliSec) throw(SpaceWireRTEPException) {
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
    sendOpenCommand();
  }

 public:
  void close() throw(SpaceWireRTEPException) {
    if (state == SpaceWireRTEPState::Enabled || state == SpaceWireRTEPState::Open) {
      this->state = SpaceWireRTEPState::Closing;
      performClosingProcess();
    }
    initializeSlidingWindow();
    initializeRetryCounts();
    openCommandAcknowledged = false;
    closeCommandAcknowledged = false;
  }

 private:
  void closed() { unregisterMeToSpaceWireREngine(); }

 private:
  void registerMeToSpaceWireREngine() { spwREngine->registerTransmitTEP(this); }

 private:
  void unregisterMeToSpaceWireREngine() { spwREngine->unregisterTransmitTEP(channel); }

 private:
  void initializeRetryCounts() {
    for (size_t i = 0; i < SpaceWireRProtocol::SizeOfSlidingWindow; i++) {
      retryCountsForSequenceNumber[i] = 0;
    }
  }

 private:
  void malfunctioningTransportChannel() {
    this->state = SpaceWireRTEPState::Closed;
    closed();
    // todo
    // add message which should be sent to user
    using namespace std;
    cout << "SpaceWireRTransmitTEP::malfunctioningTransportChannel() !!! MALFUNCTIONING TEP !!!" << endl;
    cout << ((SpaceWireRTEPInterface*)(NULL))->channel << endl;
    // debug
    throw 1;
  }

 private:
  void malfunctioningSpaceWireIF() {
    this->state = SpaceWireRTEPState::Closed;
    closed();
    // todo
    // send message to user that SpaceWire IF is failing
    // probably, Action mechanism is suitable.
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
      if (packet->isHeartBeatPacketType()) {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::consumeReceivedPackets() HeartBeat packet received." << endl;
#endif
        processHeartBeatPacket(packet);
        delete packet;
        continue;
      } else if (packet->isFlowControlPacket()) {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::consumeReceivedPackets() FlowControl packet received." << endl;
#endif
        processFlowControlPacket(packet);
        delete packet;
        continue;
      } else if (packet->isAckPacket()) {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::consumeReceivedPackets() Ack packet received." << endl;
#endif
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
                // increment sendTimeoutCounter
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

 public:
  /** Sets segment size.
   * @param[in] size segment size
   */
  inline void setSegmentSize(size_t size) { this->maximumSegmentSize = size; }

 private:
  /** Processes a received FlowControl packet.
   * MASN will be updated by retrieving the value contained in the
   * FlowControl packet.
   */
  void processFlowControlPacket(SpaceWireRPacket* packet) {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::processFlowControlPacket() entered." << endl;
#endif
    nReceivedFlowControlPackets++;
    updateMaximumAcceptableSequenceNumber(packet);
    flowControlPacket->constructAckForPacket(packet);
    try {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::processFlowControlPacket() replying FlowControlAck for sequence number = "
           << (u32)flowControlPacket->getSequenceNumber() << endl;
#endif
      // todo: inject CRC error
      spwREngine->sendPacket(flowControlPacket);
      nTransmittedFlowControlPackets++;

#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::processFlowControlPacket() FlowControlAck packet for sequence number = "
           << (u32)flowControlPacket->getSequenceNumber() << " has been sent." << endl;
#endif
    } catch (...) {
      malfunctioningSpaceWireIF();
    }
  }

 private:
  /** Replies to HeartBeat packet sent from Receive TEP.
   * This is the same implementation as one used in SpaceWireRReceiveTEP::replyAckForPacket(SpaceWireRPacket* packet).
   * @param[in] pakcet incoming SpaceWire-R packet for which this method should construct a reply pakcet
   */
  void processHeartBeatPacket(SpaceWireRPacket* packet) {
    using namespace std;
    nReceivedHeartBeatPackets++;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::processHeartBeatPacket() nHeartBeat = " << nReceivedHeartBeatPackets
         << " sequence number = " << (u32)packet->getSequenceNumber() << endl;
#endif

    // return if response to received HeartBeat packets are disabled
    if (!shouldRespondToReceivedHeartBeatPacket()) {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::processHeartBeatPacket() nHeartBeat = " << nReceivedHeartBeatPackets
           << " does not reply HeartBeatAck." << endl;
#endif
      return;
    }

    heartBeatAckPacket->constructAckForPacket(packet);
    try {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::processHeartBeatPacket() replying HeartBeatAck for sequence number = "
           << (u32)heartBeatAckPacket->getSequenceNumber() << endl;
#endif

      // todo: inject CRC error

      spwREngine->sendPacket(heartBeatAckPacket);
      nTransmittedHeartBeatAckPackets++;

#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::processHeartBeatPacket() HeartBeatAck for sequence number = "
           << (u32)heartBeatAckPacket->getSequenceNumber() << " has been sent." << endl;
#endif
    } catch (...) {
      malfunctioningSpaceWireIF();
    }
  }

 private:
  /** Processes a received Ack pakcet, and frees sliding window slot.
   * @param[in] packet incoming SpaceWire-R Ack packet
   */
  void processAckPacket(SpaceWireRPacket* packet) {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::processAckPacket() sequence number = " << (u32)packet->getSequenceNumber() << endl;
#endif
    u8 sequenceNumberOfThisPacket = packet->getSequenceNumber();
    if (packetHasBeenSent[sequenceNumberOfThisPacket] == true) {
      packetWasAcknowledged[sequenceNumberOfThisPacket] = true;
      decrementNOfOutstandingPackets();
    }
    if (packet->isControlAckPacket() && slidingWindowBuffer[sequenceNumberOfThisPacket]->isControlPacketOpenCommand()) {
      openCommandAcknowledged = true;
    }
    if (packet->isControlAckPacket() &&
        slidingWindowBuffer[sequenceNumberOfThisPacket]->isControlPacketCloseCommand()) {
      closeCommandAcknowledged = true;
    }
    if (packet->isHeartBeatAckPacketType()) {
      nReceivedHeartBeatAckPackets++;
    }
    if (this->isFlowControlEnabled()) {
      this->updateMaximumAcceptableSequenceNumber(packet);
    }
    slideSlidingWindow();
  }

 private:
  void sendPacket(SpaceWireRPacket* packet,
                  double timeoutDuration = DefaultTimeoutDurationInMs /*ms*/) throw(SpaceWireRTEPException) {
    sendPacketWithSpecifiedSequenceNumber(packet, sequenceNumber, timeoutDuration);
  }

 public:
  void send(std::vector<u8>* data, double timeoutDuration = DefaultTimeoutDurationInMs) throw(SpaceWireRTEPException) {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::send() entered." << endl;
#endif
    sendMutex.lock();
    this->heartBeatTimer->resetHeartBeatTimer();
    sendTimeoutCounter = 0;
    nOfOutstandingPackets = 0;
    size_t dataSize = data->size();
    size_t remainingSize = dataSize;
    size_t payloadSize;
    size_t index = 0;
    size_t nSegmentation = 0;
    bool isFirst = true;
    while (remainingSize != 0 && this->state == SpaceWireRTEPState::Open) {
      // check timeout
      if (sendTimeoutCounter > timeoutDuration) {
        cout << "sendTimeoutCounter = " << dec << sendTimeoutCounter << "  timeoutDuration=" << timeoutDuration << endl;
        // timeout occurs
        malfunctioningTransportChannel();
        sendMutex.unlock();
        throw SpaceWireRTEPException(SpaceWireRTEPException::Timeout);
      }
      checkRetryTimerThenRetry();

      // wait until MASN becomes larger than sequenceNumber
      if (this->isFlowControlEnabled()) {
        if (!this->masnAllowsToSend()) {
          conditionForSendWait.wait(DefaultWaitDurationInMsForSendSegment);
          continue;
        }
      }

      if (this->maximumSegmentSize < remainingSize) {
        payloadSize = this->maximumSegmentSize;
      } else {
        payloadSize = remainingSize;
      }

      // check if there is room in sliding window
      SpaceWireRPacket* packet = getAvailablePacketInstance();
      if (packet == NULL) {  // if no room
        // wait then return
        conditionForSendWait.wait(DefaultWaitDurationInMsForSendSegment);
        continue;
      }

      // configure SpaceWireRPacket instance
      if (isFirst) {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::send() setting the FirstSegment flag." << endl;
#endif
        packet->setFirstSegmentFlag();
        isFirst = false;
      } else {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::send() setting the ContinuedSegment flag." << endl;
#endif
        packet->setContinuedSegmentFlag();
      }
      packet->setSequenceNumber(sequenceNumber);
      packet->setDataPacketFlag();
      packet->setPayload(data, index, payloadSize);

      // update counters
      remainingSize -= payloadSize;
      retryTimeoutCounters[sequenceNumber] = 0;
      index += payloadSize;
      sequenceNumber++;
      nSegmentation++;

      if (sequenceNumber == 0) {
        sequenceNumberLaps();
      }
      incrementNOfOutstandingPackets();

      // set LastFlag/CompleteSegment if necessary
      if (remainingSize == 0) {
        if (nSegmentation == 1) {
#ifdef DebugSpaceWireRTransmitTEP
          cout << "SpaceWireRTransmitTEP::send() setting the CompleteSement flag." << endl;
#endif
          packet->setCompleteSegmentFlag();
        } else {
#ifdef DebugSpaceWireRTransmitTEP
          cout << "SpaceWireRTransmitTEP::send() setting the LastSegment flag. " << endl;
#endif
          packet->setLastSegmentFlag();
        }
      }

      // send segment
      try {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::send() sending a segment sequence number="
             << packet->getSequenceNumberAs32bitInteger() << " " << packet->getSequenceFlagsAsString() << endl;
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
    }

    // check all sent packets were acknowledged
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
  bool masnGuardBit_true_if_MASNLapped_SNNotLappedYet = false;
  CxxUtilities::Mutex masnGuardBit_mutex;

 private:
  void sequenceNumberLaps() {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::sequenceNumberLaps() entered." << endl;
#endif
    masnGuardBit_mutex.lock();
    masnGuardBit_true_if_MASNLapped_SNNotLappedYet = !masnGuardBit_true_if_MASNLapped_SNNotLappedYet;
    masnGuardBit_mutex.unlock();
  }

 private:
  void masnLaps() {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::masnLaps() entered." << endl;
#endif
    masnGuardBit_mutex.lock();
    masnGuardBit_true_if_MASNLapped_SNNotLappedYet = !masnGuardBit_true_if_MASNLapped_SNNotLappedYet;
    masnGuardBit_mutex.unlock();
  }

 private:
  bool masnAllowsToSend() {
    u8 n = sequenceNumber;
    u8 masn = maximumAcceptableSequenceNumber;
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::masnAllowsToSend() sequenceNumber=" << dec << (u32)n << " MASN=" << (u32)masn
         << " Lap=" << ((masnGuardBit_true_if_MASNLapped_SNNotLappedYet) ? "true" : "false") << " ." << endl;
#endif
    if (masnGuardBit_true_if_MASNLapped_SNNotLappedYet) {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::masnAllowsToSend() true." << endl;
#endif
      return true;  // can send packet since masn is larger than n.
    } else {
      if (n <= masn) {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::masnAllowsToSend() true." << endl;
#endif
        return true;  // can send packet since masn is larger than n.
      } else {
#ifdef DebugSpaceWireRTransmitTEP
        cout << "SpaceWireRTransmitTEP::masnAllowsToSend() false." << endl;
#endif
        return false;  // should wait until masn becomes larger than n
      }
    }
  }

 private:
  void performClosingProcess() { sendCloseCommand(); }

 private:
  void sendOpenCommand() throw(SpaceWireRTEPException) {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::sendOpenCommand()" << endl;
#endif

    // initialize
    initializeRetryCounts();

    // get packet instance
    SpaceWireRPacket* packet = slidingWindowBuffer[0];
    nOfOutstandingPackets++;
    if (packet == NULL) {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::sendOpenCommand() no packet instance available." << endl;
#endif
      malfunctioningTransportChannel();
    }

    // set sequence flag
    packet->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

    // set packet type
    packet->setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand);

    // set payload length
    packet->setPayloadLength(0x00);

    // set sequence number
    packet->setSequenceNumber(0x00);
    this->sequenceNumber = 0;
    openCommandAcknowledged = false;
    while (!openCommandAcknowledged) {
#ifdef DebugSpaceWireRTransmitTEP
      cout << "SpaceWireRTransmitTEP::sendOpenCommand() Sending open command." << endl;
      cout << packet->toString() << endl;
#endif
      // send
      spwREngine->sendPacket(packet);
      packetHasBeenSent[sequenceNumber] = true;
      conditionForSendWait.wait(DefaultTimeoutDurationInMsForOpen);
      retryCountsForSequenceNumber[sequenceNumber]++;
      if (retryCountsForSequenceNumber[sequenceNumber] > maxRetryCount) {
        throw SpaceWireRTEPException(SpaceWireRTEPException::OpenFailed);
      }
    }
    // update state if open command is acknowledged
    this->sequenceNumber = 1;
    this->state = SpaceWireRTEPState::Open;
  }

 private:
  void sendCloseCommand() {
    SpaceWireRPacket* packet = getAvailablePacketInstance();
    if (packet == NULL) {
      malfunctioningTransportChannel();
    }

    // set sequence flag
    packet->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

    // set packet type
    packet->setPacketType(SpaceWireRPacketType::ControlPacketCloseCommand);

    // set payload length
    packet->setPayloadLength(0x00);

    // set sequence number
    packet->setSequenceNumber(0x00);
    this->sequenceNumber = 0;
    closeCommandAcknowledged = false;
    while (!closeCommandAcknowledged) {
      // send
      spwREngine->sendPacket(packet);
      packetHasBeenSent[sequenceNumber] = true;
      conditionForSendWait.wait(DefaultTimeoutDurationInMsForOpen);
      retryCountsForSequenceNumber[sequenceNumber]++;
      if (retryCountsForSequenceNumber[sequenceNumber] > maxRetryCount) {
        break;
      }
    }
    // update state
    this->state = SpaceWireRTEPState::Closed;
  }

 public:
  void updateMaximumAcceptableSequenceNumber(SpaceWireRPacket* packet) {
    using namespace std;
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::updateMaximumAcceptableSequenceNumber() entered." << endl;
#endif
    if (packet->getPayloadLength() != 1 && packet->getPayload()->size() != 1) {
      // MASM should be one octet.
      malfunctioningTransportChannel();
    }

    u8 newMASN = packet->getPayload()->at(0);
    if (newMASN < maximumAcceptableSequenceNumber) {
      masnLaps();
    }
#ifdef DebugSpaceWireRTransmitTEP
    cout << "SpaceWireRTransmitTEP::updateMaximumAcceptableSequenceNumber() currentMASN="
         << (u32)maximumAcceptableSequenceNumber << " newMASN=" << (u32)newMASN << endl;
#endif
    maximumAcceptableSequenceNumber = newMASN;
  }

 public:
  std::string toString() {
    using namespace std;
    std::stringstream ss;
    ss << "---------------------------------------------" << endl;
    ss << "SpaceWireRTransmitTEP" << endl;
    ss << "State                : " << SpaceWireRTEPState::toString(this->state) << endl;
    ss << "slidingWindowFrom    : " << dec << (u32)this->slidingWindowFrom << endl;
    ss << "slidingWindowSize    : " << dec << (u32)this->slidingWindowSize << endl;
    ss << "nOfOutstandingPckts  : " << dec << (u32)this->nOfOutstandingPackets << endl;
    ss << "MASN                 : " << dec << (u32)this->maximumAcceptableSequenceNumber << endl;
    ss << "receivedPackets.size : " << dec << receivedPackets.size() << endl;
    ss << "Counters:" << endl;
    ss << "nSentUserData        : " << dec << nSentUserData << endl;
    ss << "nSentUserDataInBytes : " << dec << nSentUserDataInBytes / 1024 << "kB" << endl;
    ss << "nSentSegments        : " << dec << nSentSegments << endl;
    ss << "nLostAckPackets:     : " << dec << nLostAckPackets << endl;
    ss << dec;
    ss << "nReceivedHeartBeat         : " << nReceivedHeartBeatPackets << endl;
    ss << "nTransmittedHeartBeatAck   : " << nTransmittedHeartBeatAckPackets << endl;
    ss << "nTransmittedHeartBeat      : " << nTransmittedHeartBeatPackets << endl;
    ss << "nReceivedHeartBeatAck      : " << nReceivedHeartBeatAckPackets << endl;
    ss << "nReceivedFlowControlPackets: " << nReceivedFlowControlPackets << endl;
    ss << "---------------------------------------------" << endl;
    return ss.str();
  }
};

#endif /* SPACEWIRERTRANSMITTEP_HH_ */
