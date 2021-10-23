#ifndef SPACEWIRE_SPACEWIREIFOVERTCP_HH_
#define SPACEWIRE_SPACEWIREIFOVERTCP_HH_

#include "spacewire/spacewireif.hh"
#include "spacewire/spacewiressdtpmodule.hh"
#include "spacewire/spacewireutil.hh"
#include "spacewire/tcpsocket.hh"
#include "spacewire/types.hh"

#include <memory>

class SpaceWireIFOverTCP : public SpaceWireIF {
 public:
  enum class ConnectionMode { ClientMode, ServerMode };

 public:
  SpaceWireIFOverTCP(const std::string& iphostname, u32 portnumber)
      : iphostname_(iphostname), port_(portnumber), operationMode_(ConnectionMode::ClientMode) {}
  SpaceWireIFOverTCP(u32 portnumber) : port_(portnumber), operationMode_(ConnectionMode::ServerMode) {}
  virtual ~SpaceWireIFOverTCP() = default;

  void open() override {
    if (isClientMode()) {  // client mode
      try {
        dataSocket_ = std::make_unique<TCPClientSocket>(iphostname_, port_);
        ((TCPClientSocket*)dataSocket_)->open(1000);
        setTimeoutDuration(500000);
      } catch (TCPSocketException& e) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      } catch (...) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      }
    } else {  // server mode
      try {
        serverSocket_ = std::make_unique<TCPServerSocket>(port_);
        serverSocket_->open();
        dataSocket_ = serverSocket_->accept();
        setTimeoutDuration(500000);
      } catch (TCPSocketException& e) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      } catch (...) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      }
    }
    dataSocket_->setNoDelay();
    ssdtp_ = std::make_unique<SpaceWireSSDTPModule>(dataSocket_);
  }

  void close() override {
    if (dataSocket_) {
      dataSocket_->close();
    }
    if (!isClientMode()) {
      serverSocket_->close();
    }
  }

  void send(u8* data, size_t length, EOPType eopType = EOP) {
    if (!ssdtp_) {
      throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
    }
    try {
      ssdtp_->send(data, length, eopType);
    } catch (const SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::Disconnected);
      }
    }
  }

  void receive(std::vector<u8>* buffer) {
    if (!ssdtp_) {
      throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
    }
    try {
      ssdtp_->receive(buffer);
    } catch (const SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      }
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    }
  }

  void setTimeoutDuration(f64 microsecond) override {
    dataSocket_->setTimeout(microsecond / 1000.);
    timeoutDurationInMicroSec_ = microsecond;
  }

  SpaceWireSSDTPModule* getSSDTPModule() { return ssdtp_; }
  u32 getOperationMode() const { return operationMode_; }
  bool isClientMode() const { return operationMode_ == ConnectionMode::ClientMode; }
  void cancelReceive() override {}

 private:
  std::string iphostname_{};
  u32 port_{};
  std::unique_ptr<SpaceWireSSDTPModule> ssdtp_{};
  TCPSocket* dataSocket_{};
  std::unique_ptr<TCPServerSocket> serverSocket_{};
  u32 operationMode_{};
};

#endif
