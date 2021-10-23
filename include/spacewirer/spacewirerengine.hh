#ifndef SPACEWIRERENGINE_HH_
#define SPACEWIRERENGINE_HH_

#include "spacewirer/spacewirerclassinterfaces.hh"
#include "spacewirer/spacewirerpacket.hh"

class SpaceWireREngineException {
 public:
  SpaceWireREngineException(int status) {}
  ~SpaceWireREngineException() = default;
  enum { SpaceWireREngineIsNotRunning, SpaceWireIFIsNotWorking };
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

class SpaceWireREngine {
 public:
  SpaceWireREngine(SpaceWireIF* spwif) {
    this->spwif = spwif;
    nDiscardedReceivedPackets = 0;
    nSentPackets = 0;
    nReceivedPackets = 0;
  }

  static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 1000000;

  void processReceivedSpaceWireRPacket(SpaceWireRPacket* packet) throw(SpaceWireREngineException) {
    using namespace std;
#ifdef DebugSpaceWireREngine
    cout << "SpaceWireREngine::processReceivedSpaceWireRPacket()" << endl;
#endif
    uint16_t channel = packet->getChannelNumber();
    if (packet->isHeartBeatPacketType() || packet->isHeartBeatAckPacketType()) {
      // for HeartBeat/HeartBeatAck packets, all TEPs can be a potential destiantion TEP,
      // and therefore, search in the allTEPs map.
#ifdef DebugSpaceWireREngine
      cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is HeartBeat/HeartBeatAck packet." << endl;
#endif
      std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = allTEPs.find(channel);
      if (it_find != allTEPs.end()) {
        // if there is TransmitTEP/ReceiveTEP corresponding to the channel number in the received packet.
        it_find->second->pushReceivedSpaceWireRPacket(packet);  // pass the received packet to the ReceiveTEP
        // tell the TEP that a packet has arrived.
        it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to "
             << "0x" << hex << right << setw(8) << setfill('0') << (uint64_t)it_find->second
             << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
      } else {
        // if there is no ReceiveTEP to receive the packet.
        // discard the packet
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
        nDiscardedReceivedPackets++;
        delete packet;
      }
    } else if (packet->isFlowControlPacket()) {
#ifdef DebugSpaceWireREngine
      cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is FlowControl packet (sequence number="
           << packet->getSequenceNumberAs32bitInteger() << ")." << endl;
#endif
      std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = allTEPs.find(channel);
      if (it_find != allTEPs.end()) {
        // if there is TransmitTEP/ReceiveTEP corresponding to the channel number in the received packet.
        it_find->second->pushReceivedSpaceWireRPacket(packet);  // pass the received packet to the ReceiveTEP
        // tell the TEP that a packet has arrived.
        it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to "
             << "0x" << hex << right << setw(8) << setfill('0') << (uint64_t)it_find->second
             << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
      } else {
        // if there is no ReceiveTEP to receive the packet.
        // discard the packet
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
        nDiscardedReceivedPackets++;
        delete packet;
      }
    } else if (!packet->isAckPacket()) {  // command/data packet
#ifdef DebugSpaceWireREngine
      cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is Command/Data packet." << endl;
#endif
      std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = receiveTEPs.find(channel);
      if (it_find != receiveTEPs.end()) {
        // if there is ReceiveTEP corresponding to the channel number in the received packet.
        it_find->second->pushReceivedSpaceWireRPacket(packet);  // pass the received packet to the ReceiveTEP
        // tell the ReceiveTEP that a packet has arrived.
        it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to "
             << "0x" << hex << right << setw(8) << setfill('0') << (uint64_t)it_find->second
             << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
      } else {
        // if there is no ReceiveTEP to receive the packet.
        // discard the packet
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
        nDiscardedReceivedPackets++;
        delete packet;
      }
    } else {  // ack packet
#ifdef DebugSpaceWireREngine
      cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() is Ack packet." << endl;
#endif
      std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it_find = transmitTEPs.find(channel);
      if (it_find != transmitTEPs.end()) {
        // if there is TransmitTEP corresponding to the channel number in the received packet.
        it_find->second->pushReceivedSpaceWireRPacket(packet);  // pass the received packet to the ReceiveTEP
        // tell the ReceiveTEP that a packet has arrived.
        it_find->second->packetArrivalNotifier.signal();
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() pushed to "
             << "0x" << hex << right << setw(8) << setfill('0') << (uint64_t)it_find->second
             << " nPackets=" << it_find->second->receivedPackets.size() << endl;
#endif
      } else {
        // if there is no ReceiveTEP to receive the packet.
        // discard the packet
#ifdef DebugSpaceWireREngine
        cout << "SpaceWireREngine::processReceivedSpaceWireRPacket() TEP is not found." << endl;
#endif
        nDiscardedReceivedPackets++;
        delete packet;
      }
    }
  }

  void sendPacket(SpaceWireRPacket* packet) throw(SpaceWireREngineException) {
    using namespace std;
    if (this->isStopped()) {
      throw SpaceWireREngineException(SpaceWireREngineException::SpaceWireREngineIsNotRunning);
    }
    sendMutex.lock();
    try {
#ifdef DebugSpaceWireREngine
      cout << "SpaceWireREngine::sendPacket() sending packet." << endl;
#endif
      std::vector<uint8_t>* packetBuffer = packet->getPacketBufferPointer();
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

  void registerReceiveTEP(SpaceWireRTEPInterface* instance) {
    receiveTEPs[instance->channel] = instance;
    allTEPs[instance->channel] = instance;
    using namespace std;
#ifdef DebugSpaceWireREngine
    cout << "SpaceWireREngine::registerReceiveTEP() channel=" << (uint32_t)instance->channel
         << " receiveTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
  }

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
    allTEPs[instance->channel] = instance;
#ifdef DebugSpaceWireREngine
    using namespace std;
    cout << "SpaceWireREngine::registerTransmitTEP() channel=" << (uint32_t)instance->channel
         << " transmitTEPPacketListMap.size()=" << instance->receivedPackets.size() << endl;
#endif
  }

  void unregisterTransmitTEP(uint16_t channel) {
    if (transmitTEPs.find(channel) != transmitTEPs.end()) {
      transmitTEPs.erase(transmitTEPs.find(channel));
    }
    if (allTEPs.find(channel) != allTEPs.end()) {
      allTEPs.erase(allTEPs.find(channel));
    }
  }

  void tellDisconnectionToAllTEPs() {
    std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it1 = receiveTEPs.begin();
    while (it1 != receiveTEPs.end()) {
      it1->second->closeDueToSpaceWireIFFailure();
    }
    std::map<uint16_t, SpaceWireRTEPInterface*>::iterator it = transmitTEPs.begin();
    while (it != transmitTEPs.end()) {
      it->second->closeDueToSpaceWireIFFailure();
    }
  }

 public:
  void run() {
    using namespace std;
    spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
    std::vector<uint8_t>* data;
    SpaceWireRPacket* packet;
  _SpaceWireREngine_run_loop:  //
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
             << (uint32_t)packet->getChannelNumber() << endl;
#endif
        processReceivedSpaceWireRPacket(packet);
        delete data;
      } catch (SpaceWireIFException& e) {
        // todo
#ifdef DebugSpaceWireREngine
        cerr << "SpaceWireREngine::run() got SpaceWireIF " << e.toString() << endl;
#endif
        if (e.getStatus() == SpaceWireIFException::Timeout) {
          goto _SpaceWireREngine_run_loop;
        } else if (e.getStatus() == SpaceWireIFException::Disconnected) {
          tellDisconnectionToAllTEPs();
          this->stop();
        }
        break;
      } catch (SpaceWireRPacketException& e) {
        // todo
        cerr << "SpaceWireREngine::run() got SpaceWireRPacketException " << e.toString() << endl;
        dumpReceivedPacket(data);
        this->stop();
        delete data;
        delete packet;
        goto _SpaceWireREngine_run_loop;
      } catch (...) {
        // todo
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
  }

  void stop() { stopCondition.signal(); }

  SpaceWireIF* getSpaceWireIF() const { return spwif; }
  size_t getNDiscardedReceivedPackets() const { return nDiscardedReceivedPackets; }
  size_t getNSentPackets() const { return nSentPackets; }
  size_t getNReceivedPackets() const { return nReceivedPackets; }

 private:
  void dumpReceivedPacket(const std::vector<uint8_t>* data) {
    cerr << "SpaceWireREngine got invalid SpaceWire-R packet." << endl;
    SpaceWireUtilities::dumpPacket(data);
  }

  SpaceWireIF* spwif;
  CxxUtilities::Condition stopCondition;

  std::map<uint16_t, SpaceWireRTEPInterface*> receiveTEPs;
  std::map<uint16_t, SpaceWireRTEPInterface*> transmitTEPs;
  std::map<uint16_t, SpaceWireRTEPInterface*> allTEPs;

  size_t nDiscardedReceivedPackets;
  size_t nSentPackets;
  size_t nReceivedPackets;
  CxxUtilities::Mutex sendMutex;

  static constexpr double TimeoutDurationForStopCondition = 1000;
};

#endif /* SPACEWIRERENGINE_HH_ */
