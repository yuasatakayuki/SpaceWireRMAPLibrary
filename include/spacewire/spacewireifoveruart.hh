#ifndef SPACEWIRE_SPACEWIREIFOVERUART_HH_
#define SPACEWIRE_SPACEWIREIFOVERUART_HH_

#include "spacewire/serialport.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewiressdtpmoduleuart.hh"
#include "spacewire/types.hh"

#include <memory>
#include <string>

/** SpaceWire IF class which transfers data over UART.
 */
class SpaceWireIFOverUART : public SpaceWireIF {
 public:
  static constexpr i32 BAUD_RATE = 230400;

  SpaceWireIFOverUART(const std::string& deviceName);
  ~SpaceWireIFOverUART() override = default;

  bool open() override;
  void close() override;
  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) override;
  void receive(std::vector<u8>* buffer) override;
  void setTimeoutDuration(f64 microsecond) override {}

  /** Cancels ongoing receive() method if any exist.
   */
  void cancelReceive() override;

 private:
  const std::string deviceName_;
  std::unique_ptr<SpaceWireSSDTPModuleUART> ssdtp_;
};

#endif
