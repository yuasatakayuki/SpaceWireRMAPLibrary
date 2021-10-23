#ifndef SPACEWIRE_RMAPTARGETNODE_HH_
#define SPACEWIRE_RMAPTARGETNODE_HH_

#include "spacewire/types.hh"
#include "spacewire/spacewireutil.hh"

class RMAPTargetNode {
 public:
  RMAPTargetNode() = default;

  u8 getDefaultKey() const { return defaultKey_; }
  const std::vector<u8>& getReplyAddress() const { return replyAddress_; }
  u8 getTargetLogicalAddress() const { return targetLogicalAddress_; }
  const std::vector<u8>& getTargetSpaceWireAddress() const { return targetSpaceWireAddress_; }
  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress_; }

  void setDefaultKey(u8 defaultKey) { defaultKey_ = defaultKey; }
  void setReplyAddress(const std::vector<u8>& replyAddress) { replyAddress_ = replyAddress; }
  void setTargetLogicalAddress(u8 targetLogicalAddress) { targetLogicalAddress_ = targetLogicalAddress; }
  void setTargetSpaceWireAddress(const std::vector<u8>& spaceWireAddress) {
    targetSpaceWireAddress_ = spaceWireAddress;
  }
  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress_ = logicalAddress; }

  std::string toString(size_t nTabs = 0) const {
    using namespace std;
    stringstream ss;
    ss << "Initiator Logical Address : 0x" << right << hex << setw(2) << setfill('0')
       << (uint32_t)initiatorLogicalAddress_ << endl;
    ss << "Target Logical Address    : 0x" << right << hex << setw(2) << setfill('0')
       << static_cast<uint32_t>(targetLogicalAddress_) << endl;
    ss << "Target SpaceWire Address  : "
       << spacewire::util::packetToString(targetSpaceWireAddress_.data(), targetSpaceWireAddress_.size()) << '\n';
    ss << "Reply Address             : " << spacewire::util::packetToString(replyAddress_.data(), replyAddress_.size())
       << '\n';
    ss << "Default Key               : 0x" << right << hex << setw(2) << setfill('0')
       << static_cast<uint32_t>(defaultKey_) << endl;
    stringstream ss2;
    while (!ss.eof()) {
      std::string line{};
      getline(ss, line);
      for (size_t i = 0; i < nTabs; i++) {
        ss2 << "	";
      }
      ss2 << line << endl;
    }
    return ss2.str();
  }

  static constexpr u8 DEFAULT_LOGICAL_ADDRESS = 0xFE;
  static constexpr u8 DEFAULT_KEY = 0x20;

 private:
  std::vector<u8> targetSpaceWireAddress_{};
  std::vector<u8> replyAddress_{};
  u8 targetLogicalAddress_ = DEFAULT_LOGICAL_ADDRESS;
  u8 initiatorLogicalAddress_ = DEFAULT_LOGICAL_ADDRESS;
  u8 defaultKey_ = DEFAULT_KEY;
};
#endif
