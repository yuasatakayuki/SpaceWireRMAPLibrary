#include "gtest/gtest.h"
#include "rmap/rmappacket.hh"

TEST(RMAPPacketTest, TestRMAPPacketConstructionAndInterpretation) {
  const std::vector<u8> replyPathAddress{5, 4, 3, 2, 1};
  const std::vector<u8> writeData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  RMAPPacket rmapPacket;
  rmapPacket.setCommand();
  rmapPacket.setWrite();
  rmapPacket.setVerifyFlag(true);
  rmapPacket.setReplyAddress(replyPathAddress);
  rmapPacket.setData(writeData.data(), writeData.size());

  const auto packetBufferPtr = rmapPacket.getPacketBufferPointer();

  RMAPPacket packetInterpret;
  packetInterpret.interpretAsAnRMAPPacket(packetBufferPtr);
  const auto interpretedPacketBufferPtr = packetInterpret.getPacketBufferPointer();

  ASSERT_EQ(*packetBufferPtr, *interpretedPacketBufferPtr);
}
