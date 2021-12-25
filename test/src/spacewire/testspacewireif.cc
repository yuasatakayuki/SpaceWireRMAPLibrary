#include "gtest/gtest.h"
#include "spacewire/spacewireif.hh"
#include "spacewire/types.hh"

#include <deque>
#include <tuple>

using Packet = std::tuple<std::vector<u8>, EOPType>;
class SpaceWireIFVirtual : public SpaceWireIF {
 public:
  SpaceWireIFVirtual(std::deque<Packet>& sendFifo, std::deque<Packet>& receiveFifo)
      : sendFifo_(sendFifo), receiveFifo_(receiveFifo) {}
  ~SpaceWireIFVirtual() override = default;

  bool open() override { return true; }
  bool isOpen() const { return true; }

  void close() override {}

  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) override {
    std::vector<u8> dataVec;
    sendFifo_.emplace_back(std::vector<u8>(data, data + length), eopType);
  }
  void receive(std::vector<u8>* buffer) override {
    if (receiveFifo_.empty()) {
      buffer->clear();
    } else {
      EOPType eopType{};
      std::tie(*buffer, eopType) = receiveFifo_.front();
      receiveFifo_.pop_front();

      if (eopType == EOPType::EEP) {
        throw SpaceWireIFException(SpaceWireIFException::EEP);
      }
    }
  }
  void cancelReceive() override {}
  void setTimeoutDuration(f64 microsecond) override {}

 private:
  std::deque<Packet>& sendFifo_;
  std::deque<Packet>& receiveFifo_;
};

class SpaceWireIFTest : public ::testing::Test {
 protected:
  std::deque<Packet> fifoAtoB_;
  std::deque<Packet> fifoBtoA_;
  SpaceWireIFVirtual portA_{fifoAtoB_, fifoBtoA_};
  SpaceWireIFVirtual portB_{fifoBtoA_, fifoAtoB_};
};

TEST_F(SpaceWireIFTest, TestSendReceiveEOP) {
  const std::array<u8, 3> sendData = {1, 2, 3};
  portA_.send(sendData.data(), sendData.size());

  std::vector<u8> receiveBuffer;
  portB_.receive(&receiveBuffer);
  ASSERT_EQ(receiveBuffer.size(), sendData.size());
}

TEST_F(SpaceWireIFTest, TestSendReceiveEEP) {
  const std::array<u8, 3> sendData = {1, 2, 3};
  portA_.send(sendData.data(), sendData.size(), EOPType::EEP);

  std::vector<u8> receiveBuffer;
  EXPECT_THROW(
      try { portB_.receive(&receiveBuffer); } catch (const SpaceWireIFException& e) {
        ASSERT_EQ(e.getStatus(), SpaceWireIFException::EEP);
        ASSERT_EQ(receiveBuffer.size(), sendData.size());
        throw;
      },
      SpaceWireIFException);
}
