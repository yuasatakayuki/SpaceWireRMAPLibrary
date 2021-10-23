#include "rmap/rmapengine.hh"

#include "rmap/rmapinitiator.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewireutil.hh"
#include "spacewire/types.hh"

#include "spdlog/spdlog.h"

RMAPEngine::RMAPEngine(SpaceWireIF* spwif, const ReceivedPacketOption& receivedPacketOption)
    : spwif_(spwif), receivedPacketOption_(receivedPacketOption) {
  initialize();
}
RMAPEngine::~RMAPEngine() {
  stopped_ = true;
  spwif_->cancelReceive();
  if (runThread_.joinable()) {
    spdlog::info("Waiting for RMAPEngine receive thread to join");
    runThread_.join();
    spdlog::info("RMAPEngine receive thread joined");
  }
}

void RMAPEngine::start() {
  if (!spwif_) {
    throw RMAPEngineException(RMAPEngineException::SpaceWireInterfaceIsNotSet);
  }
  stopped_ = false;
  runThread_ = std::thread(&RMAPEngine::run, this);
}

void RMAPEngine::run() {
  hasStopped_ = false;

  spwif_->setTimeoutDuration(DEFAULT_RECEIVE_TIMEOUT_DURATION_MICROSEC);
  while (!stopped_) {
    try {
      auto rmapPacket = receivePacket();
      if (rmapPacket) {
        if (rmapPacket->isCommand()) {
          nDiscardedReceivedCommandPackets_++;
        } else {
          rmapReplyPacketReceived(std::move(rmapPacket));
        }
      }
    } catch (const RMAPPacketException& e) {
      if (!stopped_) {
        spdlog::warn("RMAPPacketException in RMAPEngine::run() {}", e.what());
      }
    } catch (const RMAPEngineException& e) {
      if (!stopped_) {
        spdlog::warn("RMAPEngineException in RMAPEngine::run() {}", e.toString());
      }
    }
  }
  stopped_ = true;
  hasStopped_ = true;
}

void RMAPEngine::stop() {
  spdlog::info("Stopping RMAPEngine");
  if (!stopped_) {
    stopped_ = true;
    spwif_->cancelReceive();

    while (!hasStopped_) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(50ms);
    }
  }
  spdlog::info("RMAPEngine has stopped");
}

TransactionID RMAPEngine::initiateTransaction(RMAPPacket* commandPacket, RMAPInitiator* rmapInitiator) {
  if (!isStarted()) {
    throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
  }
  const auto transactionID = getNextAvailableTransactionID();
  // register the transaction to management list if Reply is required
  if (commandPacket->isReplyFlagSet()) {
    std::lock_guard guard(transactionIDMutex_);
    transactions_[transactionID] = rmapInitiator;
  } else {
    // otherwise put back transaction Id to available id list
    releaseTransactionID(transactionID);
  }
  // send a command packet
  commandPacket->setTransactionID(transactionID);
  commandPacket->constructPacket();
  const auto packet = commandPacket->getPacketBufferPointer();

  spwif_->send(packet->data(), packet->size());
  return transactionID;
}

bool RMAPEngine::isTransactionIDAvailable(u16 transactionID) {
  std::lock_guard guard(transactionIDMutex_);
  return transactions_.find(transactionID) == transactions_.end();
}

void RMAPEngine::putBackRMAPPacketInstnce(RMAPPacketPtr packet) {
  std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
  freeRMAPPacketList_.emplace_back(std::move(packet));
}

void RMAPEngine::initialize() {
  std::lock_guard guard(transactionIDMutex_);
  latestAssignedTransactionID_ = 0;
  availableTransactionIDList_.clear();
  for (size_t i = 0; i < MAX_TID_NUMBER; i++) {
    availableTransactionIDList_.push_back(i);
  }
}

u16 RMAPEngine::getNextAvailableTransactionID() {
  std::lock_guard guard(transactionIDMutex_);
  if (!availableTransactionIDList_.empty()) {
    const auto tid = availableTransactionIDList_.front();
    availableTransactionIDList_.pop_front();
    return tid;
  } else {
    throw RMAPEngineException(RMAPEngineException::TooManyConcurrentTransactions);
  }
}

void RMAPEngine::deleteTransactionIDFromDB(TransactionID transactionID) {
  std::lock_guard guard(transactionIDMutex_);
  const auto& it = transactions_.find(transactionID);
  if (it != transactions_.end()) {
    transactions_.erase(it);
    releaseTransactionID(transactionID);
  }
}

void RMAPEngine::releaseTransactionID(u16 transactionID) {
  std::lock_guard guard(transactionIDMutex_);
  availableTransactionIDList_.push_back(transactionID);
}

RMAPPacketPtr RMAPEngine::receivePacket() {
  try {
    spwif_->receive(&receivePacketBuffer_);
  } catch (const SpaceWireIFException& e) {
  }

  RMAPPacketPtr packet = reuseOrCreateRMAPPacket();
  try {
    packet->setDataCRCIsChecked(!receivedPacketOption_.skipDataCrcCheck);
    packet->interpretAsAnRMAPPacket(&receivePacketBuffer_, receivedPacketOption_.skipConstructingWholePacketVector);
  } catch (const RMAPPacketException& e) {
    nDiscardedReceivedPackets_++;
    return {};
  }
  return packet;
}

RMAPInitiator* RMAPEngine::resolveTransaction(const RMAPPacket* packet) {
  const auto transactionID = packet->getTransactionID();
  if (isTransactionIDAvailable(transactionID)) {  // if tid is not in use
    throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived);
  } else {  // if tid is registered to tid db
    // resolve transaction
    auto rmapInitiator = transactions_[transactionID];
    // delete registered tid
    deleteTransactionIDFromDB(transactionID);
    // return resolved transaction
    return rmapInitiator;
  }
}

void RMAPEngine::rmapReplyPacketReceived(RMAPPacketPtr packet) {
  try {
    // find a corresponding command packet
    auto rmapInitiator = resolveTransaction(packet.get());
    // register reply packet to the resolved transaction
    rmapInitiator->replyReceived(std::move(packet));
  } catch (const RMAPEngineException& e) {
    // if not found, increment error counter
    nErrorneousReplyPackets_++;
    putBackRMAPPacketInstnce(std::move(packet));
    return;
  }
}

RMAPPacketPtr RMAPEngine::reuseOrCreateRMAPPacket() {
  std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
  if (!freeRMAPPacketList_.empty()) {
    auto packet = std::move(freeRMAPPacketList_.back());
    freeRMAPPacketList_.pop_back();
    return packet;
  } else {
    return std::make_unique<RMAPPacket>();
  }
}
