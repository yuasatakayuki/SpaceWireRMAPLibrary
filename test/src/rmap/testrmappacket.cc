#include "gtest/gtest.h"
#include "rmap/rmappacket.hh"

TEST(RMAPPacketTest, TestRMAPPacketCreation) {
  RMAPPacket rmapPacket;
  rmapPacket.constructHeader();
}
