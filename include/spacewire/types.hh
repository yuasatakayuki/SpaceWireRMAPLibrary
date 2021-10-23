#ifndef SPACEWIRE_TYPES_HH_
#define SPACEWIRE_TYPES_HH_

#include <memory>
#include <sstream>

using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

enum class EOPType { EOP = 0, EEP = 1, Continued = 0xFFFF };

class SerialPort;
class RMAPEngine;
class RMAPInitiator;
class RMAPPacket;
using SerialPortPtr = std::shared_ptr<SerialPort>;
using RMAPEnginePtr = std::shared_ptr<RMAPEngine>;
using RMAPInitiatorPtr = std::shared_ptr<RMAPInitiator>;
using RMAPPacketPtr = std::unique_ptr<RMAPPacket>;

class Exception {
 public:
  Exception(u32 status) : status_(status) {}
  virtual ~Exception() = default;
  virtual std::string toString() const = 0;
  u32 getStatus() const { return status_; }

 protected:
  u32 status_{};
};

class RMAPReplyException : public Exception {
public:
  RMAPReplyException(u32 status) : Exception(status) {}
  std::string toString() const override {
    std::stringstream ss;
    ss << "Reply Status " << status_;
    return ss.str();
  }
};

using TransactionID = u16;
#endif
