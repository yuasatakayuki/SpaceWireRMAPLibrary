#include "spacewire/spacewireifoveruart.hh"

#include "spdlog/spdlog.h"

#include <chrono>
#include <thread>
#include <cassert>

SpaceWireIFOverUART::SpaceWireIFOverUART(const std::string& deviceName) : SpaceWireIF(),deviceName_(deviceName) {}

bool SpaceWireIFOverUART::open() {
  auto serial = std::make_unique<SerialPortBoostAsio>(deviceName_, BAUD_RATE);
  ssdtp_ = std::make_unique<SpaceWireSSDTPModuleUART>(std::move(serial));
  isOpen_ = true;
  return isOpen_;
}

void SpaceWireIFOverUART::close() {
  ssdtp_->cancelReceive();
  isOpen_ = false;
}

void SpaceWireIFOverUART::send(const u8* data, size_t length, EOPType eopType) {
  assert(!ssdtp_);
  try {
    ssdtp_->send(data, length, eopType);
  } catch (const SpaceWireSSDTPException& e) {
    if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
      throw SpaceWireIFException(SpaceWireIFException::Timeout);
    } else {
      throw SpaceWireIFException(SpaceWireIFException::SendFailed);
    }
  }
}

void SpaceWireIFOverUART::receive(std::vector<u8>* buffer) {
  assert(!ssdtp_);
  try {
    EOPType eopType{};
    ssdtp_->receive(buffer, eopType);
    if (eopType == EOPType::EEP) {
      throw SpaceWireIFException(SpaceWireIFException::EEP);
    }
  } catch (const SpaceWireSSDTPException& e) {
    spdlog::debug("SpaceWireIFOverUART::receive() SpaceWireSSDTPException::{} ({})", e.toString(), e.getStatus());
    if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
      throw SpaceWireIFException(SpaceWireIFException::Timeout);
    } else {
      throw SpaceWireIFException(SpaceWireIFException::ReceiveFailed);
    }
  } catch (const SerialPortException& e) {
    spdlog::debug("SpaceWireIFOverUART::receive() SerialPortException::{} ({})", e.toString(), e.getStatus());
    throw SpaceWireIFException(SpaceWireIFException::Timeout);
  }
}

void SpaceWireIFOverUART::cancelReceive() { ssdtp_->cancelReceive(); }
