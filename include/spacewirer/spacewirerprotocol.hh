#ifndef SPACEWIRERPROTOCOL_HH_
#define SPACEWIRERPROTOCOL_HH_

#include <spacewire/types.hh>

class SpaceWireRProtocol {
 public:
  static constexpr u8 ProtocolID = 0x52;
  static constexpr size_t SizeOfSlidingWindow = 256;
};

#endif /* SPACEWIRERPROTOCOL_HH_ */
