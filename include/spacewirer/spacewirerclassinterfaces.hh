#ifndef SPACEWIRERCLASSINTERFACES_HH_
#define SPACEWIRERCLASSINTERFACES_HH_

#include "spacewirer/spacewirerpacket.hh"

#include <deque>

class SpaceWireRTEPInterface {
 public:
  virtual ~SpaceWireRTEPInterface() = default;
  virtual void closeDueToSpaceWireIFFailure() = 0;
  void pushReceivedSpaceWireRPacket(SpaceWireRPacket* packet) {
    std::lock_guard<std::mutex> guard(mutexReceivedPackets);
    receivedPackets.push_back(packet);
  }

  std::deque<SpaceWireRPacket*> receivedPackets;
  CxxUtilities::Condition packetArrivalNotifier;  // used to notify arrival of SpaceWire-R packets by SpaceWire-R Engine
  uint16_t channel{};

 protected:
  SpaceWireRPacket* popReceivedSpaceWireRPacket() {
    std::lock_guard<std::mutex> guard();
    SpaceWireRPacket* result = receivedPackets.front();
    receivedPackets.pop_front();
    return result;
  }

 private:
  std::mutex mutexReceivedPackets;
};

#endif /* SPACEWIRERCLASSINTERFACES_HH_ */
