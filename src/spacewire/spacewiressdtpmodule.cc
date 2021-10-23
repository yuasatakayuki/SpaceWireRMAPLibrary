#include "spacewire/spacewiressdtpmodule.hh"
#include "spacewire/tcpsocket.hh"

#include <cstring>

std::string SpaceWireSSDTPException::toString() const {
  std::string result;
  switch (status_) {
    case DataSizeTooLarge:
      result = "DataSizeTooLarge";
      break;
    case Disconnected:
      result = "Disconnected";
      break;
    case ReceiveFailed:
      result = "ReceiveFailed";
      break;
    case SendFailed:
      result = "SendFailed";
      break;
    case Timeout:
      result = "Timeout";
      break;
    case TCPSocketError:
      result = "TCPSocketError";
      break;
    case EEP:
      result = "EEP";
      break;
    case SequenceError:
      result = "SequenceError";
      break;
    case NotImplemented:
      result = "NotImplemented";
      break;
    case TimecodeReceiveError:
      result = "TimecodeReceiveError";
      break;
    case Undefined:
      result = "Undefined";
      break;
    default:
      result = "Undefined";
      break;
  }
  return result;
}

SpaceWireSSDTPModule::SpaceWireSSDTPModule(std::unique_ptr<TCPSocket> socket): socket_(std::move(socket)), receiveBuffer_(SpaceWireSSDTPModule::BUFFER_SIZE_BYTES) {
}

void SpaceWireSSDTPModule::send(std::vector<u8>* data, EOPType eopType) {
  std::lock_guard<std::mutex> guard(sendMutex_);
  if (this->closed_) {
    return;
  }
  size_t size = data->size();
  if (eopType == EOPType ::EOP) {
    sendHeader_[0] = DATA_FLAG_COMPLETE_EOP;
  } else if (eopType == EOPType ::EEP) {
    sendHeader_[0] = DATA_FLAG_COMPLETE_EEP;
  } else if (eopType == EOPType ::Continued) {
    sendHeader_[0] = DATA_FLAG_FLAGMENTED;
  }
  sendHeader_[1] = 0x00;
  for (size_t i = 11; i > 1; i--) {
    sendHeader_[i] = size % 0x100;
    size = size / 0x100;
  }
  try {
    socket_->send(sendHeader_.data(), 12);
    socket_->send(data->data(), data->size());
  } catch (...) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
  }
}

void SpaceWireSSDTPModule::send(const u8* data, size_t length, EOPType eopType) {
  std::lock_guard<std::mutex> guard(sendMutex_);
  if (this->closed_) {
    return;
  }
  if (eopType == EOPType::EOP) {
    sendHeader_[0] = DATA_FLAG_COMPLETE_EOP;
  } else if (eopType == EOPType::EEP) {
    sendHeader_[0] = DATA_FLAG_COMPLETE_EEP;
  } else if (eopType == EOPType::Continued) {
    sendHeader_[0] = DATA_FLAG_FLAGMENTED;
  }
  sendHeader_[1] = 0x00;
  size_t asize = length;
  for (size_t i = 11; i > 1; i--) {
    sendHeader_[i] = asize % 0x100;
    asize = asize / 0x100;
  }
  try {
    socket_->send(sendHeader_.data(), 12);
    socket_->send(data, length);
  } catch (...) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
  }
}

std::vector<u8> SpaceWireSSDTPModule::receive() {
  std::lock_guard<std::mutex> guard(receiveMutex_);
  if (this->closed_) {
    return {};
  }
  std::vector<u8> data;
  EOPType eopType{};
  receive(&data, eopType);
  return data;
}

size_t SpaceWireSSDTPModule::receive(std::vector<u8>* data, EOPType& eopType) {
  size_t size = 0;
  size_t hsize = 0;
  size_t flagment_size = 0;
  size_t received_size = 0;

  try {
    std::lock_guard<std::mutex> guard(receiveMutex_);
  // header
  receive_header:  //
    receiveHeader_[0] = 0xFF;
    receiveHeader_[1] = 0x00;
    while (receiveHeader_[0] != DATA_FLAG_COMPLETE_EOP && receiveHeader_[0] != DATA_FLAG_COMPLETE_EEP) {
      hsize = 0;
      flagment_size = 0;
      received_size = 0;
      // flag and size part
      try {
        while (hsize != 12) {
          if (this->closed_) {
            return 0;
          }
          if (this->receiveCanceled_) {
            // reset receiveCanceled
            this->receiveCanceled_ = false;
            // return with no data
            return 0;
          }
          const auto result = socket_->receive(receiveHeader_.data() + hsize, 12 - hsize);
          hsize += result;
        }
      } catch (const TCPSocketException& e) {
        if (e.getStatus() == TCPSocketException::Timeout) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
        } else {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
        }
      } catch (...) {
        throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
      }

      // data or control code part
      if (receiveHeader_[0] == DATA_FLAG_COMPLETE_EOP || receiveHeader_[0] == DATA_FLAG_COMPLETE_EEP ||
          receiveHeader_[0] == DATA_FLAG_FLAGMENTED) {
        // data
        for (u32 i = 2; i < 12; i++) {
          flagment_size = flagment_size * 0x100 + receiveHeader_[i];
        }
        while (received_size != flagment_size) {
          long result;
        _loop_receiveDataPart:  //
          try {
            result = socket_->receive(receiveBuffer_.data() + size + received_size, flagment_size - received_size);
          } catch (const TCPSocketException& e) {
            if (e.getStatus() == TCPSocketException::Timeout) {
              goto _loop_receiveDataPart;
            }
            // TODO: error handling
            exit(-1);
          }
          received_size += result;
        }
        size += received_size;
      } else if (receiveHeader_[0] == CONTROL_FLAG_SEND_TIME_CODE || receiveHeader_[0] == CONTROL_FLAG_GOT_TIME_CODE) {
        // control
        u8 timecode_and_reserved[2];
        u32 tmp_size = 0;
        try {
          while (tmp_size != 2) {
            int result = socket_->receive(timecode_and_reserved + tmp_size, 2 - tmp_size);
            tmp_size += result;
          }
        } catch (...) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
        }
      } else {
        throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
      }
    }
    data->resize(size);
    if (size != 0) {
      memcpy(data->data(), receiveBuffer_.data(), size);
    } else {
      goto receive_header;
    }
    if (receiveHeader_[0] == DATA_FLAG_COMPLETE_EOP) {
      eopType = EOPType::EOP;
    } else if (receiveHeader_[0] == DATA_FLAG_COMPLETE_EEP) {
      eopType = EOPType::EEP;
    } else {
      eopType = EOPType::Continued;
    }

    return size;
  } catch (SpaceWireSSDTPException& e) {
    throw e;
  } catch (TCPSocketException& e) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
  }
}
