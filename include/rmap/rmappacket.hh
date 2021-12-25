#ifndef SPACEWIRE_RMAPPACKET_HH_
#define SPACEWIRE_RMAPPACKET_HH_

#include "rmap/rmapprotocol.hh"
#include "rmap/rmapreplystatus.hh"
#include "rmap/rmaptargetnode.hh"
#include "rmap/rmaputilities.hh"
#include "spacewire/spacewirepacket.hh"
#include "spacewire/spacewireutil.hh"

class RMAPPacketException : public std::runtime_error {
 public:
  RMAPPacketException(const std::string& message) : std::runtime_error(message) {}
};

class RMAPPacket : public SpaceWirePacket {
 public:
  RMAPPacket();
  virtual ~RMAPPacket() override = default;

  void constructHeader();
  void constructPacket();
  void interpretAsAnRMAPPacket(const u8* packet, size_t length, bool skipConstructingWhopePacketVector = false);
  void interpretAsAnRMAPPacket(const std::vector<u8>* data, bool skipConstructingWhopePacketVector = false);
  void setRMAPTargetInformation(const RMAPTargetNode* rmapTargetNode);
  void setCommand() { instruction_ = instruction_ | RMAPPacket::BIT_MASK_COMMAND_REPLY; }
  void setReply() { instruction_ = instruction_ & (~RMAPPacket::BIT_MASK_COMMAND_REPLY); }
  void setWrite() { instruction_ = instruction_ | RMAPPacket::BIT_MASK_WRITE_READ; }
  void setRead() { instruction_ = instruction_ & (~RMAPPacket::BIT_MASK_WRITE_READ); }
  void setVerifyFlag(bool mode);
  void setReplyFlag(bool mode);
  void setIncrementFlag(bool mode);
  void setReplyPathAddressLength(u8 pathAddressLength);

  bool isCommand() const { return (instruction_ & RMAPPacket::BIT_MASK_COMMAND_REPLY) != 0; }
  bool isReply() const { return (instruction_ & RMAPPacket::BIT_MASK_COMMAND_REPLY) == 0; }
  bool isWrite() const { return (instruction_ & RMAPPacket::BIT_MASK_WRITE_READ) != 0; }
  bool isRead() const { return (instruction_ & RMAPPacket::BIT_MASK_WRITE_READ) == 0; }
  bool isVerifyFlagSet() const { return (instruction_ & RMAPPacket::BIT_MASK_VERIFY_FLAG) != 0; }
  bool isReplyFlagSet() const { return (instruction_ & RMAPPacket::BIT_MASK_REPLY_FLAG) != 0; }
  bool isIncrementFlagSet() const { return (instruction_ & RMAPPacket::BIT_MASK_INCREMENT_FLAG) != 0; }

  u8 getReplyPathAddressLength() const { return instruction_ & RMAPPacket::BIT_MASK_REPLY_PATH_ADDRESS_LENGTH; }
  u32 getAddress() const { return address_; }
  void getData(u8* buffer, size_t maxLength) const;
  void getData(std::vector<u8>* buffer) const { *buffer = data_; }
  std::vector<u8>* getDataBuffer() { return &data_; }
  bool hasData() const { return !data_.empty() || (isCommand() && isWrite()) || (isReply() && isRead()); }

  u8 getDataCRC() const { return dataCRC_; }
  u32 getDataLength() const { return dataLength_; }
  u8 getExtendedAddress() const { return extendedAddress_; }
  u8 getHeaderCRC() const { return headerCRC_; }
  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress_; }
  u8 getInstruction() const { return instruction_; }
  u8 getKey() const { return key_; }
  const std::vector<u8>& getReplyAddress() const { return replyAddress_; }
  u16 getTransactionID() const { return transactionID_; }
  u8 getStatus() const { return status_; }
  u8 getTargetLogicalAddress() const { return targetLogicalAddress_; }
  const std::vector<u8>& getTargetSpaceWireAddress() const { return targetSpaceWireAddress_; }

  u32 getHeaderCRCMode() const { return headerCRCMode_; }
  u32 getDataCRCMode() const { return dataCRCMode_; }

  void setAddress(u32 address) { address_ = address; }
  void setData(const u8* data, size_t length);

  void setDataCRC(u8 dataCRC) { dataCRC_ = dataCRC; }
  void setDataLength(u32 dataLength) { dataLength_ = dataLength; }
  // void setLength(u32 dataLength) { dataLength_ = dataLength; }
  void setExtendedAddress(u8 extendedAddress) { extendedAddress_ = extendedAddress; }
  void setHeaderCRC(u8 headerCRC) { headerCRC_ = headerCRC; }
  void setInitiatorLogicalAddress(u8 initiatorLogicalAddress) { initiatorLogicalAddress_ = initiatorLogicalAddress; }
  void setInstruction(u8 instruction) { instruction_ = instruction; }
  void setKey(u8 key) { key_ = key; }
  void setReplyAddress(const std::vector<u8>& replyAddress,  //
                       bool automaticallySetPathAddressLengthToInstructionField = true);

  void setTransactionID(u16 transactionID) { transactionID_ = transactionID; }
  void setStatus(u8 status) { status_ = status; }
  void setHeaderCRCMode(u32 headerCRCMode) { headerCRCMode_ = headerCRCMode; }
  void setDataCRCMode(u32 dataCRCMode) { dataCRCMode_ = dataCRCMode; }

  void clearData() { data_.clear(); }

  std::string toString() const { return isCommand() ? toStringCommandPacket() : toStringReplyPacket(); }

  void setTargetLogicalAddress(u8 targetLogicalAddress) { targetLogicalAddress_ = targetLogicalAddress; }
  void setTargetSpaceWireAddress(const std::vector<u8>& targetSpaceWireAddress) {
    targetSpaceWireAddress_ = targetSpaceWireAddress;
  }

  std::vector<u8>* getPacketBufferPointer();

  /** Constructs a reply packet for a corresponding command packet.
   * @param[in] commandPacket a command packet of which reply packet will be constructed
   */
  static RMAPPacket* constructReplyForCommand(RMAPPacket* commandPacket,
                                              u8 status = RMAPReplyStatus::CommandExcecutedSuccessfully);
  bool getDataCRCIsChecked() const { return dataCRCIsChecked_; }
  bool getHeaderCRCIsChecked() const { return headerCRCIsChecked_; }
  void setDataCRCIsChecked(bool dataCRCIsChecked) { dataCRCIsChecked_ = dataCRCIsChecked; }
  void setHeaderCRCIsChecked(bool headerCRCIsChecked) { headerCRCIsChecked_ = headerCRCIsChecked; }

  enum { AutoCRC, ManualCRC };

 private:
  static std::vector<u8> removeLeadingZerosInReplyAddress(std::vector<u8> replyAddress);
  std::string toStringCommandPacket() const;
  std::string toStringReplyPacket() const;
  void toStringInstructionField(std::stringstream& ss) const;

  static constexpr u8 BIT_MASK_RESERVED = 0x80;
  static constexpr u8 BIT_MASK_COMMAND_REPLY = 0x40;
  static constexpr u8 BIT_MASK_WRITE_READ = 0x20;
  static constexpr u8 BIT_MASK_VERIFY_FLAG = 0x10;
  static constexpr u8 BIT_MASK_REPLY_FLAG = 0x08;
  static constexpr u8 BIT_MASK_INCREMENT_FLAG = 0x04;
  static constexpr u8 BIT_MASK_REPLY_PATH_ADDRESS_LENGTH = 0x3;

  std::vector<u8> targetSpaceWireAddress_{};
  u8 targetLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;

  u8 instruction_{};
  u8 key_ = RMAPProtocol::DEFAULT_KEY;
  std::vector<u8> replyAddress_{};
  u8 initiatorLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;
  u8 extendedAddress_ = RMAPProtocol::DEFAULT_EXTENDED_ADDRESS;
  u32 address_{};
  u32 dataLength_{};
  u8 status_ = RMAPProtocol::DEFAULT_STATUS;
  u8 headerCRC_{};
  u16 transactionID_ = RMAPProtocol::DEFAULT_TID;

  std::vector<u8> header_{};
  std::vector<u8> data_{};
  u8 dataCRC_{};

  std::vector<u8> wholePacket_{};

  u32 headerCRCMode_ = RMAPPacket::AutoCRC;
  u32 dataCRCMode_ = RMAPPacket::AutoCRC;

  std::vector<u8> temporaryPathAddress_{};

  bool headerCRCIsChecked_{};
  bool dataCRCIsChecked_{};
};
#endif
