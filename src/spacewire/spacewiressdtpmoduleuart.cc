#include "spacewire/spacewiressdtpmoduleuart.hh"
#include "types.h"

SpaceWireSSDTPModuleUART::SpaceWireSSDTPModuleUART(SerialPortPtr serialPort)
    : serialPort_(std::move(serialPort)), sendBuffer_(MAX_RECEIVE_SIZE_BYTES) {
  assert(serialPort_);
}

void SpaceWireSSDTPModuleUART::send(const u8* data, size_t length, EOPType eopType) {
  std::lock_guard<std::mutex> guard(sendMutex_);
  if (closed_) {
    return;
  }
  if (eopType == EOPType::EOP) {
    sheader_[0] = DATA_FLAG_COMPLETE_EOP;
  } else if (eopType == EOPType::EEP) {
    sheader_[0] = DATA_FLAG_COMPLETE_EEP;
  } else if (eopType == EOPType::Continued) {
    sheader_[0] = DATA_FLAG_FLAGMENTED;
  }
  sheader_[1] = 0x00;
  size_t asize = length;
  for (size_t i = 11; i > 1; i--) {
    sheader_[i] = asize % 0x100;
    asize = asize / 0x100;
  }
  try {
    serialPort_->send(sheader_.data(), HEADER_LENGTH_BYTES);
    serialPort_->send(data, length);
  } catch (...) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
  }
}

size_t SpaceWireSSDTPModuleUART::receive(std::vector<u8>* receiveBuffer, EOPType& eopType) {
  receiveBuffer->clear();
  receiveCanceled_ = false;

  try {
    std::lock_guard<std::mutex> guard(receiveMutex_);
    // header
    rheader_[0] = 0xFF;
    rheader_[1] = 0x00;

    while (rheader_[0] != DATA_FLAG_COMPLETE_EOP && rheader_[0] != DATA_FLAG_COMPLETE_EEP) {
      // flag and size part
      try {
        size_t receivedHeaderSizeBytes = 0;
        while (receivedHeaderSizeBytes != HEADER_LENGTH_BYTES) {
          if (closed_) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
          }
          if (receiveCanceled_) {
            return 0;
          }
          receivedHeaderSizeBytes += serialPort_->receive(rheader_.data() + receivedHeaderSizeBytes,
                                                          HEADER_LENGTH_BYTES - receivedHeaderSizeBytes);
        }
      } catch (const SerialPortException& e) {
        throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
      } catch (...) {
        throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
      }
      // data or control code part
      if (rheader_[0] == DATA_FLAG_COMPLETE_EOP || rheader_[0] == DATA_FLAG_COMPLETE_EEP ||
          rheader_[0] == DATA_FLAG_FLAGMENTED) {
        // data
        size_t payloadSizeBytes = 0;
        for (size_t i = 2; i < HEADER_LENGTH_BYTES; i++) {
          payloadSizeBytes = (payloadSizeBytes << 8) + rheader_[i];
        }
        // verify fragment size
        if (atLeastOnePacketSuccessfulyReceived_ && payloadSizeBytes > MAX_RECEIVE_SIZE_BYTES) {
          // When the connected node (e.g. FPGA) is not properly initialized (e.g. the last communication was abruptly
          // terminated due to an error), the connected node might send corrupted data, but those should be ignored
          // until a valid packet is successfully received.
          spdlog::error("Too large SSDTP segment size requested by the peer ({:d} bytes)", payloadSizeBytes);
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::DataSizeTooLarge);
        }

        // Resize receive buffer
        const size_t alreadyReceivedBytes = receiveBuffer->size();
        receiveBuffer->resize(receiveBuffer->size() + payloadSizeBytes);

        size_t receivedPayloadSizeBytes = 0;
        while (receivedPayloadSizeBytes != payloadSizeBytes) {
          if (receiveCanceled_) {
            return 0;
          }
          try {
            auto bufferPointer = receiveBuffer->data() + alreadyReceivedBytes + receivedPayloadSizeBytes;
            receivedPayloadSizeBytes +=
                serialPort_->receive(bufferPointer, payloadSizeBytes - receivedPayloadSizeBytes);
          } catch (const SerialPortException& e) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
          }
        }
      } else if (rheader_[0] == CONTROL_FLAG_SEND_TIME_CODE || rheader_[0] == CONTROL_FLAG_GOT_TIME_CODE) {
        constexpr size_t timeCodeDataSizeBytes = 2;
        std::array<u8, timeCodeDataSizeBytes> timecodeReceiveBuffer{};
        size_t receivedSize = 0;
        try {
          while (receivedSize < timeCodeDataSizeBytes) {
            receivedSize +=
                serialPort_->receive(timecodeReceiveBuffer.data() + receivedSize, timeCodeDataSizeBytes - receivedSize);
          }
        } catch (const SerialPortException& e) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::TimecodeReceiveError);
        }
      } else {
        throw SpaceWireSSDTPException(SpaceWireSSDTPException::ReceiveFailed);
      }
    }

    if (rheader_[0] == DATA_FLAG_COMPLETE_EOP) {
      eopType = EOPType::EOP;
    } else if (rheader_[0] == DATA_FLAG_COMPLETE_EEP) {
      eopType = EOPType::EEP;
    } else {
      eopType = EOPType::Continued;
    }
    atLeastOnePacketSuccessfulyReceived_ = true;
    return receiveBuffer->size();
  } catch (const SpaceWireSSDTPException& e) {
    throw e;
  } catch (const SerialPortException& e) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
  }
}

void SpaceWireSSDTPModuleUART::sendRawData(const u8* data, size_t length) {
  std::lock_guard<std::mutex> guard(sendMutex_);
  try {
    serialPort_->send(data, length);
  } catch (const SerialPortException& e) {
    throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
  }
}
