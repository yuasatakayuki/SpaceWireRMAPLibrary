#ifndef SPACEWIRE_RMAPINITIATOR_HH_
#define SPACEWIRE_RMAPINITIATOR_HH_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "rmap/rmapengine.hh"
#include "rmap/rmappacket.hh"
#include "rmap/rmapprotocol.hh"
#include "rmap/rmapreplystatus.hh"
#include "rmap/rmaptargetnode.hh"

#include "spdlog/spdlog.h"

class RMAPInitiatorException : public Exception {
 public:
  enum {
    Timeout = 0x100,
    Aborted = 0x200,
    ReadReplyWithInsufficientData,
    ReadReplyWithTooMuchData,
    UnexpectedWriteReplyReceived,
    RMAPTransactionCouldNotBeInitiated,
  };

  explicit RMAPInitiatorException(u32 status) : Exception(status) {}
  virtual ~RMAPInitiatorException() = default;
  std::string toString() const override {
    std::string result;
    switch (status_) {
      case Timeout:
        result = "Timeout";
        break;
      case Aborted:
        result = "Aborted";
        break;
      case ReadReplyWithInsufficientData:
        result = "ReadReplyWithInsufficientData";
        break;
      case ReadReplyWithTooMuchData:
        result = "ReadReplyWithTooMuchData";
        break;
      case UnexpectedWriteReplyReceived:
        result = "UnexpectedWriteReplyReceived";
        break;
      case RMAPTransactionCouldNotBeInitiated:
        result = "RMAPTransactionCouldNotBeInitiated";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

class RMAPInitiator {
 public:
  explicit RMAPInitiator(RMAPEnginePtr rmapEngine);

  void read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer,
            i32 timeoutDurationMillisec = DEFAULT_TIMEOUT_DURATION_MILLISEC);
  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, u32 length,
             i32 timeoutDurationMillisec = DEFAULT_TIMEOUT_DURATION_MILLISEC);

  void replyReceived(RMAPPacketPtr packet);

  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress_; }
  bool getReplyMode() const { return replyMode_; }
  bool getIncrementMode() const { return incrementMode_; }
  bool getVerifyMode() const { return verifyMode_; }

  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress_ = logicalAddress; }
  void setReplyMode(bool replyMode) { replyMode_ = replyMode; }
  void setIncrementMode(bool incrementMode) { incrementMode_ = incrementMode; }
  void setVerifyMode(bool verifyMode) { verifyMode_ = verifyMode; }

  static constexpr i32 DEFAULT_TIMEOUT_DURATION_MILLISEC = 2000;
  static constexpr bool DEFAULT_INCREMENT_MODE = true;
  static constexpr bool DEFAULT_VERIFY_MODE = true;
  static constexpr bool DEFAULT_REPLY_MODE = true;

 private:
  RMAPEnginePtr rmapEngine_{};
  RMAPPacketPtr commandPacket_{};
  RMAPPacketPtr replyPacket_{};
  std::atomic<bool> replyPacketSet_{false};

  u8 initiatorLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;
  bool incrementMode_ = DEFAULT_INCREMENT_MODE;
  bool verifyMode_ = DEFAULT_VERIFY_MODE;
  bool replyMode_ = DEFAULT_REPLY_MODE;

  std::condition_variable replyWaitCondition_{};
  std::mutex replyWaitMutex_{};
};
#endif
