#ifndef GROWTHDAQ_SERIALPORT_HH_
#define GROWTHDAQ_SERIALPORT_HH_

#include "types.h"
#include "spacewire/types.hh"

#include "asio.hpp"
#include "asio/serial_port.hpp"

#include "serial/serial.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <atomic>

class SerialPortException : public Exception {
 public:
  enum { Timeout, SerialPortClosed, TooLargeDataReceived };
  SerialPortException(i32 status) : Exception(status) {}

 public:
  std::string toString() const override {
    switch (getStatus()) {
      case Timeout:
        return "Timeout";
      case SerialPortClosed:
        return "SerialPortClosed";
      case TooLargeDataReceived:
        return "TooLargeDataReceived";
      default:
        return "UndefinedException";
    }
  }
};

class SerialPort {
 public:
  virtual ~SerialPort() = default;
  virtual void close() = 0;
  virtual void send(const u8* buffer, size_t length) = 0;
  virtual size_t receive(u8* buffer, size_t length) = 0;
  /** Receives data until specified length is completely received.
   * Received data are stored in the data buffer.
   * @param[in] data u8 buffer where received data will be stored
   * @param[in] length length of data to be received
   * @return received size
   */
  virtual size_t receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length);
  /** Receives data. The maximum length can be specified as length.
   * Receive size may be shorter than the specified length.
   * @param[in] data u8 buffer where received data will be stored
   * @param[in] length the maximum size of the data buffer
   * @return received size
   */
  virtual size_t receive(u8* data, u32 length, bool waitUntilSpecifiedLengthCompletes);
};

class SerialPortLibSerial : public SerialPort {
 public:
  static constexpr size_t ReadBufferSize = 10 * 1024;

 public:
  SerialPortLibSerial(const std::string& deviceName, size_t baudRate = 9600);
  void close() override;
  void send(const u8* buffer, size_t length) override;
  size_t receive(u8* buffer, size_t length) override;

 private:
  std::unique_ptr<serial::Serial> serial_;
};

class SerialPortBoostAsio : public SerialPort {
 public:
  static constexpr size_t ReadBufferSize = 10 * 1024;

 public:
  SerialPortBoostAsio(const std::string& deviceName, size_t baudRate = 9600);
  void close() override;
  void send(const u8* buffer, size_t length) override;
  size_t receive(u8* buffer, size_t length) override;

 private:
  template <typename MutableBufferSequence>
  void receiveWithTimeout(MutableBufferSequence& buffers, size_t& nReceivedBytes,
                          const asio::steady_timer::duration& expiry_time) {
    std::optional<asio::error_code> timer_result;
    auto& io_service = port_->get_io_service();
    asio::steady_timer timer(io_service);
    timer.expires_from_now(expiry_time);
    timer.async_wait([&timer_result](const asio::error_code& error) { timer_result = error; });

    std::optional<asio::error_code> read_result;
    port_->async_read_some(buffers,
                           [&read_result, &nReceivedBytes](const asio::error_code& error, size_t _nReceivedBytes) {
                             read_result = error;
                             nReceivedBytes = _nReceivedBytes;
                           });

    io_service.reset();
    while (io_service.run_one()) {
      if (read_result) {
        timer.cancel();
      } else if (timer_result) {
        port_->cancel();
        if (anythingHasBeenSent_) {
          spdlog::error("UART receive timeout timer expired");
          openDevice();
        }
      }
    }

    if (*read_result) {
      throw asio::system_error(*read_result);
    }
  }

 private:
  void openDevice();

  const std::string deviceName_;
  size_t baudRate_{};
  std::unique_ptr<asio::serial_port> port_;
  asio::io_service io_;
  asio::streambuf receive_buffer_;
  size_t nSerialPortOpen_ = 0;
  std::atomic<bool> anythingHasBeenSent_{false};
};

#endif
