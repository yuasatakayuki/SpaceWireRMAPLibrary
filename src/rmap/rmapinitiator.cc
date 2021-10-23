#include "rmap/rmapinitiator.hh"

RMAPInitiator::RMAPInitiator(RMAPEnginePtr rmapEngine) : rmapEngine_(std::move(rmapEngine)) {
  commandPacket_ = std::make_unique<RMAPPacket>();
  replyPacket_ = std::make_unique<RMAPPacket>();
}

void RMAPInitiator::read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer,
                         i32 timeoutDurationMillisec) {
  commandPacket_->setInitiatorLogicalAddress(getInitiatorLogicalAddress());
  commandPacket_->setRead();
  commandPacket_->setCommand();
  commandPacket_->setIncrementFlag(incrementMode_);
  commandPacket_->setVerifyFlag(false);
  commandPacket_->setReplyFlag(true);
  commandPacket_->setExtendedAddress(0x00);
  commandPacket_->setAddress(memoryAddress);
  commandPacket_->setDataLength(length);
  commandPacket_->clearData();
  // InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below.
  commandPacket_->setRMAPTargetInformation(rmapTargetNode);

  if (replyPacket_) {
    rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket_));
    replyPacketSet_ = false;
  }

  std::unique_lock<std::mutex> lock(replyWaitMutex_);

  const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket_.get(), this);

  const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
  replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket_); });

  if (replyPacketSet_ && replyPacket_) {
    if (replyPacket_->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
      throw RMAPReplyException(replyPacket_->getStatus());
    }
    if (length < replyPacket_->getDataBuffer()->size()) {
      throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
    }
    replyPacket_->getData(buffer, length);
    // when successful, replay packet is retained until next transaction for inspection by user application
    return;
  } else {
    // cancel transaction (return transaction ID)
    rmapEngine_->cancelTransaction(assignedTransactionID);
    throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
  }
}

void RMAPInitiator::write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, u32 length,
                          i32 timeoutDurationMillisec) {
  commandPacket_->setInitiatorLogicalAddress(getInitiatorLogicalAddress());
  commandPacket_->setWrite();
  commandPacket_->setCommand();
  commandPacket_->setIncrementFlag(incrementMode_);
  commandPacket_->setVerifyFlag(verifyMode_);
  commandPacket_->setReplyFlag(replyMode_);
  commandPacket_->setExtendedAddress(0x00);
  commandPacket_->setAddress(memoryAddress);
  commandPacket_->setDataLength(length);
  commandPacket_->setRMAPTargetInformation(rmapTargetNode);
  commandPacket_->setData(data, length);

  if (replyPacket_) {
    rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket_));
    replyPacketSet_ = false;
  }

  std::unique_lock<std::mutex> lock(replyWaitMutex_);
  const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket_.get(), this);

  if (!replyMode_) {  // if reply is not expected
    return;
  }
  const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
  replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket_); });
  if (replyPacketSet_ && replyPacket_) {
    if (replyPacket_->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
      return;
    } else {
      throw RMAPReplyException(replyPacket_->getStatus());
    }
  } else {
    rmapEngine_->cancelTransaction(assignedTransactionID);
    throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
  }
}

void RMAPInitiator::replyReceived(RMAPPacketPtr packet) {
  std::lock_guard<std::mutex> guard(replyWaitMutex_);
  replyPacket_ = std::move(packet);
  replyPacketSet_ = true;
  replyWaitCondition_.notify_one();
}
