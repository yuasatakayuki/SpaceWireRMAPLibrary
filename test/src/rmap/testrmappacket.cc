#include "gtest/gtest.h"
#include "rmap/rmappacket.hh"

TEST(RMAPPacketTest, TestRMAPPacketConstructionWriteCommand) {
  const std::vector<u8> replyPathAddress{5, 4, 3, 2, 1};
  const std::vector<u8> writeData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  RMAPPacket packetA;
  packetA.setCommand();
  packetA.setWrite();
  packetA.setVerifyFlag(true);
  packetA.setReplyAddress(replyPathAddress);
  packetA.setData(writeData.data(), writeData.size());

  const auto packetBufferPtr = packetA.getPacketBufferPointer();

  RMAPPacket packetB;
  packetB.interpretAsAnRMAPPacket(packetBufferPtr);
  const auto interpretedPacketBufferPtr = packetB.getPacketBufferPointer();

  ASSERT_EQ(*packetBufferPtr, *interpretedPacketBufferPtr);
  ASSERT_EQ(*packetA.getDataBuffer(), *packetB.getDataBuffer());
}

TEST(RMAPPacketTest, TestRMAPPacketConstructionReadReply) {
  const std::vector<u8> replyPathAddress{5, 4, 3, 2, 1};
  const std::vector<u8> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  RMAPPacket packetA;
  packetA.setReply();
  packetA.setRead();
  packetA.setReplyAddress(replyPathAddress);
  packetA.setData(data.data(), data.size());

  const auto packetBufferPtr = packetA.getPacketBufferPointer();

  RMAPPacket packetB;
  packetB.interpretAsAnRMAPPacket(packetBufferPtr);
  const auto interpretedPacketBufferPtr = packetB.getPacketBufferPointer();

  ASSERT_EQ(*packetBufferPtr, *interpretedPacketBufferPtr);
  ASSERT_EQ(*packetA.getDataBuffer(), *packetB.getDataBuffer());
}
