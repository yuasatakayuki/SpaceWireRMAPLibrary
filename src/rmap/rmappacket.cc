#include "rmap/rmappacket.hh"

RMAPPacket::RMAPPacket() : SpaceWirePacket(RMAPProtocol::ProtocolIdentifier) {}

void RMAPPacket::constructHeader() {
  header_.clear();
  if (isCommand()) {
    // if command packet
    header_.push_back(targetLogicalAddress_);
    header_.push_back(protocolID_);
    header_.push_back(instruction_);
    header_.push_back(key_);
    std::vector<u8> tmpvector;
    u8 tmporaryCounter = replyAddress_.size();
    while (tmporaryCounter % 4 != 0) {
      header_.push_back(0x00);
      tmporaryCounter++;
    }
    for (size_t i = 0; i < replyAddress_.size(); i++) {
      header_.push_back(replyAddress_.at(i));
    }
    header_.push_back(initiatorLogicalAddress_);
    header_.push_back((transactionID_ & 0xff00) >> 8);
    header_.push_back((transactionID_ & 0x00ff) >> 0);
    header_.push_back(extendedAddress_);
    header_.push_back((address_ & 0xff000000) >> 24);
    header_.push_back((address_ & 0x00ff0000) >> 16);
    header_.push_back((address_ & 0x0000ff00) >> 8);
    header_.push_back((address_ & 0x000000ff) >> 0);
    header_.push_back((dataLength_ & 0x00ff0000) >> 16);
    header_.push_back((dataLength_ & 0x0000ff00) >> 8);
    header_.push_back((dataLength_ & 0x000000ff) >> 0);
  } else {
    // if reply packet
    header_.push_back(initiatorLogicalAddress_);
    header_.push_back(protocolID_);
    header_.push_back(instruction_);
    header_.push_back(status_);
    header_.push_back(targetLogicalAddress_);
    header_.push_back((transactionID_ & 0xff00) >> 8);
    header_.push_back((transactionID_ & 0x00ff) >> 0);
    if (isRead()) {
      header_.push_back(0);
      header_.push_back((dataLength_ & 0x00ff0000) >> 16);
      header_.push_back((dataLength_ & 0x0000ff00) >> 8);
      header_.push_back((dataLength_ & 0x000000ff) >> 0);
    }
  }

  if (headerCRCMode_ == RMAPPacket::AutoCRC) {
    headerCRC_ = RMAPUtilities::calculateCRC(header_.data(), header_.size());
  }
  header_.push_back(headerCRC_);
}

void RMAPPacket::constructPacket() {
  constructHeader();
  if (dataCRCMode_ == RMAPPacket::AutoCRC) {
    dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
  }
  wholePacket_.clear();
  if (isCommand()) {
    wholePacket_.insert(wholePacket_.end(), targetSpaceWireAddress_.begin(), targetSpaceWireAddress_.end());
  } else {
    wholePacket_.insert(wholePacket_.end(), replyAddress_.begin(), replyAddress_.end());
  }
  wholePacket_.insert(wholePacket_.end(), header_.begin(), header_.end());
  wholePacket_.insert(wholePacket_.end(), data_.begin(), data_.end());
  if (hasData()) {
    wholePacket_.push_back(dataCRC_);
  }
}

void RMAPPacket::interpretAsAnRMAPPacket(const u8* packet, size_t length, bool skipConstructingWhopePacketVector) {
  if (length < 8) {
    std::stringstream ss;
    ss << "packet interpretation failed (length=" << length << " bytes is too short for an RMAP packet)";
    throw(RMAPPacketException(ss.str()));
  }
  size_t i = 0;
  size_t rmapIndex = 0;
  size_t rmapIndexAfterSourcePathAddress = 0;
  size_t dataIndex = 0;
  temporaryPathAddress_.clear();
  while (packet[i] < 0x20) {
    temporaryPathAddress_.push_back(packet[i]);
    i++;
    if (i >= length) {
      throw(RMAPPacketException("no destination logical address appeared before end of packet"));
    }
  }

  rmapIndex = i;
  if (rmapIndex + 8 > length) {
    throw(RMAPPacketException("packet length after destination logical address is too short"));
  }
  const auto protocolIdentifier = packet[rmapIndex + 1];
  if (protocolIdentifier != RMAPProtocol::ProtocolIdentifier) {
    std::stringstream ss;
    ss << "non-rmap protocol id (0x" << std::hex << std::setfill('0') << static_cast<u32>(protocolIdentifier) << ')';
    throw(RMAPPacketException(ss.str()));
  }

  instruction_ = packet[rmapIndex + 2];
  u8 replyPathAddressLength = getReplyPathAddressLength();
  if (isCommand()) {
    // if command packet
    setTargetSpaceWireAddress(temporaryPathAddress_);
    setTargetLogicalAddress(packet[rmapIndex]);
    setKey(packet[rmapIndex + 3]);
    std::vector<u8> temporaryReplyAddress{};
    constexpr size_t chunkBytes = 4;
    for (u8 i = 0; i < replyPathAddressLength * chunkBytes; i++) {
      temporaryReplyAddress.push_back(packet[rmapIndex + chunkBytes + i]);
    }
    setReplyAddress(temporaryReplyAddress);
    rmapIndexAfterSourcePathAddress = rmapIndex + chunkBytes + replyPathAddressLength * chunkBytes;
    setInitiatorLogicalAddress(packet[rmapIndexAfterSourcePathAddress + 0]);

    const u16 uppertid = packet[rmapIndexAfterSourcePathAddress + 1];
    const u16 lowertid = packet[rmapIndexAfterSourcePathAddress + 2];
    const auto tid = (uppertid << 8) + lowertid;
    setTransactionID(tid);
    setExtendedAddress(packet[rmapIndexAfterSourcePathAddress + 3]);
    const u32 address_3 = packet[rmapIndexAfterSourcePathAddress + 4];
    const u32 address_2 = packet[rmapIndexAfterSourcePathAddress + 5];
    const u32 address_1 = packet[rmapIndexAfterSourcePathAddress + 6];
    const u32 address_0 = packet[rmapIndexAfterSourcePathAddress + 7];
    setAddress((address_3 << 24) + (address_2 << 16) + (address_1 << 8) + address_0);
    const u32 length_2 = packet[rmapIndexAfterSourcePathAddress + 8];
    const u32 length_1 = packet[rmapIndexAfterSourcePathAddress + 9];
    const u32 length_0 = packet[rmapIndexAfterSourcePathAddress + 10];
    const auto lengthSpecifiedInPacket = (length_2 << 16) + (length_1 << 8) + length_0;
    setDataLength(lengthSpecifiedInPacket);
    const u8 temporaryHeaderCRC = packet[rmapIndexAfterSourcePathAddress + 11];
    if (headerCRCIsChecked_) {
      const u32 headerCRCMode_original = headerCRCMode_;
      headerCRCMode_ = RMAPPacket::AutoCRC;
      constructHeader();
      headerCRCMode_ = headerCRCMode_original;
      if (headerCRC_ != temporaryHeaderCRC) {
        throw(RMAPPacketException("invalid header crc"));
      }
    } else {
      headerCRC_ = temporaryHeaderCRC;
    }
    dataIndex = rmapIndexAfterSourcePathAddress + 12;
    data_.clear();
    if (isWrite()) {
      for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
        if ((dataIndex + i) < (length - 1)) {
          data_.push_back(packet[dataIndex + i]);
        } else {
          throw(RMAPPacketException("data length mismatch"));
        }
      }

      // length check for DataCRC
      u8 temporaryDataCRC = 0x00;
      if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
        temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
      } else {
        throw(RMAPPacketException("data length mismatch"));
      }
      dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
      if (dataCRCIsChecked_) {
        if (dataCRC_ != temporaryDataCRC) {
          throw(RMAPPacketException("invalid data crc"));
        }
      } else {
        dataCRC_ = temporaryDataCRC;
      }
    }

  } else {
    // if reply packet
    setReplyAddress(temporaryPathAddress_, false);
    setInitiatorLogicalAddress(packet[rmapIndex]);
    setStatus(packet[rmapIndex + 3]);
    setTargetLogicalAddress(packet[rmapIndex + 4]);
    const u16 uppertid = packet[rmapIndex + 5];
    const u16 lowertid = packet[rmapIndex + 6];
    const u16 tid = (uppertid << 8) + lowertid;
    setTransactionID(tid);
    if (isWrite()) {
      const u8 temporaryHeaderCRC = packet[rmapIndex + 7];
      u32 headerCRCMode_original = headerCRCMode_;
      headerCRCMode_ = RMAPPacket::AutoCRC;
      constructHeader();
      headerCRCMode_ = headerCRCMode_original;
      if (headerCRCIsChecked_) {
        if (headerCRC_ != temporaryHeaderCRC) {
          throw(RMAPPacketException("invalid header crc"));
        }
      } else {
        headerCRC_ = temporaryHeaderCRC;
      }
    } else {
      const u32 length_2 = packet[rmapIndex + 8];
      const u32 length_1 = packet[rmapIndex + 9];
      const u32 length_0 = packet[rmapIndex + 10];
      const auto lengthSpecifiedInPacket = (length_2 << 16) + (length_1 << 8) + length_0;
      setDataLength(lengthSpecifiedInPacket);
      const u8 temporaryHeaderCRC = packet[rmapIndex + 11];
      constructHeader();
      if (headerCRCIsChecked_) {
        if (headerCRC_ != temporaryHeaderCRC) {
          throw(RMAPPacketException("invalid header crc"));
        }
      } else {
        headerCRC_ = temporaryHeaderCRC;
      }
      dataIndex = rmapIndex + 12;
      data_.clear();
      data_.reserve(lengthSpecifiedInPacket);
      for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
        if ((dataIndex + i) < (length - 1)) {
          data_.push_back(packet[dataIndex + i]);
        } else {
          dataCRC_ = 0x00;  // initialized
          throw(RMAPPacketException("data length mismatch"));
        }
      }

      // length check for DataCRC
      u8 temporaryDataCRC = 0x00;
      if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
        temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
      } else {
        throw(RMAPPacketException("data length mismatch"));
      }
      dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
      if (dataCRCIsChecked_) {
        if (dataCRC_ != temporaryDataCRC) {
          throw(RMAPPacketException("invalid data crc"));
        }
      } else {
        dataCRC_ = temporaryDataCRC;
      }
    }
  }
  if (!skipConstructingWhopePacketVector) {
    const u32 previousHeaderCRCMode = headerCRCMode_;
    const u32 previousDataCRCMode = dataCRCMode_;
    headerCRCMode_ = RMAPPacket::ManualCRC;
    dataCRCMode_ = RMAPPacket::ManualCRC;
    constructPacket();
    headerCRCMode_ = previousHeaderCRCMode;
    dataCRCMode_ = previousDataCRCMode;
  }
}

void RMAPPacket::interpretAsAnRMAPPacket(const std::vector<u8>* data, bool skipConstructingWhopePacketVector) {
  if (data->empty()) {
    throw(RMAPPacketException("empty packet cannot be parsed as an RMAP packet"));
  }
  interpretAsAnRMAPPacket(data->data(), data->size(), skipConstructingWhopePacketVector);
}

void RMAPPacket::setRMAPTargetInformation(const RMAPTargetNode* rmapTargetNode) {
  setTargetLogicalAddress(rmapTargetNode->getTargetLogicalAddress());
  setReplyAddress(rmapTargetNode->getReplyAddress());
  setTargetSpaceWireAddress(rmapTargetNode->getTargetSpaceWireAddress());
  setKey(rmapTargetNode->getDefaultKey());
  setInitiatorLogicalAddress(rmapTargetNode->getInitiatorLogicalAddress());
}

void RMAPPacket::setVerifyFlag(bool mode) {
  if (mode) {
    instruction_ = instruction_ | RMAPPacket::BIT_MASK_VERIFY_FLAG;
  } else {
    instruction_ = instruction_ & (~RMAPPacket::BIT_MASK_VERIFY_FLAG);
  }
}

void RMAPPacket::setReplyFlag(bool mode) {
  if (mode) {
    instruction_ = instruction_ | RMAPPacket::BIT_MASK_REPLY_FLAG;
  } else {
    instruction_ = instruction_ & (~RMAPPacket::BIT_MASK_REPLY_FLAG);
  }
}

void RMAPPacket::setIncrementFlag(bool mode) {
  if (mode) {
    instruction_ = instruction_ | RMAPPacket::BIT_MASK_INCREMENT_FLAG;
  } else {
    instruction_ = instruction_ & (~RMAPPacket::BIT_MASK_INCREMENT_FLAG);
  }
}

void RMAPPacket::setReplyPathAddressLength(u8 pathAddressLength) {
  // TODO: verify the following logic
  instruction_ =
      (instruction_ & ~(RMAPPacket::BIT_MASK_INCREMENT_FLAG)) + pathAddressLength & RMAPPacket::BIT_MASK_INCREMENT_FLAG;
}

void RMAPPacket::getData(u8* buffer, size_t maxLength) const {
  const auto length = data_.size();
  if (maxLength < length) {
    throw RMAPPacketException("insufficient buffer size");
  }
  for (size_t i = 0; i < length; i++) {
    buffer[i] = data_[i];
  }
}

void RMAPPacket::setData(const u8* data, size_t length) {
  data_.clear();
  for (size_t i = 0; i < length; i++) {
    data_.push_back(data[i]);
  }
  dataLength_ = length;
}

void RMAPPacket::setReplyAddress(const std::vector<u8>& replyAddress,  //
                                 bool automaticallySetPathAddressLengthToInstructionField) {
  replyAddress_ = replyAddress;
  if (automaticallySetPathAddressLengthToInstructionField) {
    if (replyAddress.size() % 4 == 0) {
      instruction_ = (instruction_ & (~BIT_MASK_REPLY_PATH_ADDRESS_LENGTH)) + replyAddress.size() / 4;
    } else {
      instruction_ = (instruction_ & (~BIT_MASK_REPLY_PATH_ADDRESS_LENGTH)) + (replyAddress.size() + 4) / 4;
    }
  }
}

std::vector<u8>* RMAPPacket::getPacketBufferPointer() {
  constructPacket();
  return &wholePacket_;
}

/** Constructs a reply packet for a corresponding command packet.
 * @param[in] commandPacket a command packet of which reply packet will be constructed
 */
RMAPPacket* RMAPPacket::constructReplyForCommand(RMAPPacket* commandPacket, u8 status) {
  RMAPPacket* replyPacket = new RMAPPacket();
  *replyPacket = *commandPacket;

  // remove leading zeros in the Reply Address (Reply SpaceWire Address)
  replyPacket->setReplyAddress(removeLeadingZerosInReplyAddress(replyPacket->getReplyAddress()));

  replyPacket->clearData();
  replyPacket->setReply();
  replyPacket->setStatus(status);
  return replyPacket;
}

std::vector<u8> RMAPPacket::removeLeadingZerosInReplyAddress(std::vector<u8> replyAddress) {
  bool nonZeroValueHasAppeared = false;
  std::vector<u8> result;
  for (size_t i = 0; i < replyAddress.size(); i++) {
    if (!nonZeroValueHasAppeared && replyAddress[i] != 0x00) {
      nonZeroValueHasAppeared = true;
    }
    if (nonZeroValueHasAppeared) {
      result.push_back(replyAddress[i]);
    }
  }
  return result;
}

std::string RMAPPacket::toStringCommandPacket() const {
  using namespace std;

  RMAPPacket clonedPacket = *this;
  clonedPacket.constructPacket();

  stringstream ss;
  ///////////////////////////////
  // Command
  ///////////////////////////////
  // Target SpaceWire Address
  if (!targetSpaceWireAddress_.empty()) {
    ss << "--------- Target SpaceWire Address ---------" << endl;
    spacewire::util::dumpPacket(&ss, targetSpaceWireAddress_.data(), targetSpaceWireAddress_.size(), 1, 128);
  }
  // Header
  ss << "--------- RMAP Header Part ---------" << endl;
  // Initiator Logical Address
  ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress_)
     << endl;
  // Target Logical Address
  ss << "Target Logic. Address     : 0x" << right << setw(2) << setfill('0') << hex << (u32)(targetLogicalAddress_)
     << endl;
  // Protocol Identifier
  ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (u32)(1) << endl;
  // Instruction
  ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (u32)(instruction_) << endl;
  toStringInstructionField(ss);
  // Key
  ss << "Key                       : 0x" << setw(2) << setfill('0') << hex << (u32)(key_) << endl;
  // Reply Address
  if (!replyAddress_.empty()) {
    ss << "Reply Address             : ";
    spacewire::util::dumpPacket(&ss, replyAddress_.data(), replyAddress_.size(), 1, 128);
  } else {
    ss << "Reply Address         : --none--" << endl;
  }
  ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (u32)(transactionID_) << endl;
  ss << "Extended Address          : 0x" << right << setw(2) << setfill('0') << hex << (u32)(extendedAddress_) << endl;
  ss << "Address                   : 0x" << right << setw(8) << setfill('0') << hex << (u32)(address_) << endl;
  ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (u32)(dataLength_) << " (" << dec
     << dataLength_ << "dec)" << endl;
  ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (u32)(headerCRC_);
  if (headerCRC_ != clonedPacket.headerCRC_) {
    ss << " (Header CRC error. Correct CRC is 0x" << right << setw(2) << setfill('0') << static_cast<u32>(clonedPacket.headerCRC_);
  }
  ss << endl;
  // Data Part
  ss << "---------  RMAP Data Part  ---------" << endl;
  if (isWrite()) {
    ss << "[data size = " << dec << dataLength_ << "bytes]" << endl;
    spacewire::util::dumpPacket(&ss, data_.data(), data_.size(), 1, 16);
    ss << "Data CRC                  : " << right << setw(2) << setfill('0') << hex << (u32)(dataCRC_);
    if (dataCRC_ != clonedPacket.dataCRC_) {
      ss << " (Data CRC error. Correct CRC is 0x" << right << setw(2) << setfill('0') << static_cast<u32>(clonedPacket.dataCRC_);
    }
    ss << endl;
  } else {
    ss << "--- none ---" << endl;
  }
  ss << endl;

  ss << "Total data (bytes)        : " << dec << clonedPacket.getPacketBufferPointer()->size() << endl;
  ss << dec << endl;
  return ss.str();
}

std::string RMAPPacket::toStringReplyPacket() const {
  using namespace std;

  RMAPPacket clonedPacket = *this;
  clonedPacket.constructPacket();

  stringstream ss;
  ///////////////////////////////
  // Reply
  ///////////////////////////////
  ss << "RMAP Packet Dump" << endl;
  // Reply Address
  if (!replyAddress_.empty()) {
    ss << "--------- Reply Address ---------" << endl;
    ss << "Reply Address       : ";
    spacewire::util::dumpPacket(&ss, replyAddress_.data(), replyAddress_.size(), 1, 128);
  }
  // Header
  ss << "--------- RMAP Header Part ---------" << endl;
  // Initiator Logical Address
  ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress_)
     << endl;
  // Target Logical Address
  ss << "Target Logical Address    : 0x" << right << setw(2) << setfill('0') << hex << (u32)(targetLogicalAddress_)
     << endl;
  // Protocol Identifier
  ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (u32)(1) << endl;
  // Instruction
  ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (u32)(instruction_) << endl;
  toStringInstructionField(ss);
  // Status
  std::string statusstring;
  switch (status_) {
    case 0x00:
      statusstring = "Successfully Executed";
      break;
    case 0x01:
      statusstring = "General Error";
      break;
    case 0x02:
      statusstring = "Unused RMAP Packet Type or Command Code";
      break;
    case 0x03:
      statusstring = "Invalid Target Key";
      break;
    case 0x04:
      statusstring = "Invalid Data CRC";
      break;
    case 0x05:
      statusstring = "Early EOP";
      break;
    case 0x06:
      statusstring = "Cargo Too Large";
      break;
    case 0x07:
      statusstring = "EEP";
      break;
    case 0x08:
      statusstring = "Reserved";
      break;
    case 0x09:
      statusstring = "Verify Buffer Overrun";
      break;
    case 0x0a:
      statusstring = "RMAP Command Not Implemented or Not Authorized";
      break;
    case 0x0b:
      statusstring = "Invalid Target Logical Address";
      break;
    default:
      statusstring = "Reserved";
      break;
  }
  ss << "Status                    : 0x" << right << setw(2) << setfill('0') << hex << (u32)(status_) << " ("
     << statusstring << ")" << endl;
  ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (u32)(transactionID_) << endl;
  if (isRead()) {
    ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (u32)(dataLength_) << " ("
       << dec << dataLength_ << "dec)" << endl;
  }
  ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (u32)(headerCRC_);
  if (headerCRC_ != clonedPacket.headerCRC_) {
    ss << " (Header CRC error. Correct CRC is 0x" << right << setw(2) << setfill('0') << static_cast<u32>(clonedPacket.headerCRC_);
  }
  ss << endl;

  // Data Part
  ss << "---------  RMAP Data Part  ---------" << endl;
  if (isRead()) {
    ss << "[data size = " << dec << dataLength_ << "bytes]" << endl;
    spacewire::util::dumpPacket(&ss, data_.data(), data_.size(), 1, 128);
    ss << "Data CRC    : 0x" << right << setw(2) << setfill('0') << hex << (u32)(dataCRC_);
    if (dataCRC_ != clonedPacket.dataCRC_) {
      ss << " (Data CRC error. Correct CRC is 0x" << right << setw(2) << setfill('0') << static_cast<u32>(clonedPacket.dataCRC_);
    }
    ss << endl;
  } else {
    ss << "--- none ---" << endl;
  }
  ss << endl;

  ss << "Total data (bytes)        : " << dec << clonedPacket.getPacketBufferPointer()->size() << endl;
  ss << dec << endl;
  return ss.str();
}

void RMAPPacket::toStringInstructionField(std::stringstream& ss) const {
  using namespace std;

  // packet type (Command or Reply)
  ss << " ------------------------------" << endl;
  ss << " |Reserved    : 0" << endl;
  ss << " |Packet Type : " << (isCommand() ? 1 : 0);
  ss << " " << (isCommand() ? "(Command)" : "(Reply)") << endl;
  // Write or Read
  ss << " |Write/Read  : " << (isWrite() ? 1 : 0);
  ss << " " << (isWrite() ? "(Write)" : "(Read)") << endl;
  // Verify mode
  ss << " |Verify Mode : " << (isVerifyFlagSet() ? 1 : 0);
  ss << " " << (isVerifyFlagSet() ? "(Verify)" : "(No Verify)") << endl;
  // Ack mode
  ss << " |Reply Mode  : " << (isReplyFlagSet() ? 1 : 0);
  ss << " " << (isReplyFlagSet() ? "(Reply)" : "(No Reply)") << endl;
  // Increment
  ss << " |Increment   : " << (isIncrementFlagSet() ? 1 : 0);
  ss << " " << (isIncrementFlagSet() ? "(Increment)" : "(No Increment)") << endl;
  // SPAL
  ss << " |R.A.L.      : " << setw(1) << setfill('0') << hex
     << (u32)((instruction_ & BIT_MASK_REPLY_PATH_ADDRESS_LENGTH)) << endl;
  ss << " |(R.A.L. = Reply Address Length)" << endl;
  ss << " ------------------------------" << endl;
}
