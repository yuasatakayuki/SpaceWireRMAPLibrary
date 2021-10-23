#ifndef SPACEWIRE_RMAPPROTOCOL_HH_
#define SPACEWIRE_RMAPPROTOCOL_HH_

#include "spacewire/types.hh"

class RMAPProtocol {
 public:
  static constexpr u8 ProtocolIdentifier = 0x01;
  static constexpr u8 DefaultPacketType = 0x01;
  static constexpr u8 DefaultWriteOrRead = 0x01;
  static constexpr u8 DefaultVerifyMode = 0x01;
  static constexpr u8 DefaultAckMode = 0x01;
  static constexpr u8 DefaultIncrementMode = 0x01;
  static constexpr u16 DEFAULT_TID = 0x00;
  static constexpr u8 DEFAULT_EXTENDED_ADDRESS = 0x00;
  static constexpr u32 DefaultAddress = 0x00;
  static constexpr u32 DefaultLength = 0x00;
  static constexpr u8 DefaultHeaderCRC = 0x00;
  static constexpr u8 DefaultDataCRC = 0x00;
  static constexpr u8 DEFAULT_STATUS = 0x00;

  static constexpr u8 PacketTypeCommand = 0x01;
  static constexpr u8 PacketTypeReply = 0x00;

  static constexpr u8 PacketWriteMode = 0x01;
  static constexpr u8 PacketReadMode = 0x00;

  static constexpr u8 PacketVerifyMode = 0x01;
  static constexpr u8 PacketNoVerifyMode = 0x00;

  static constexpr u8 PacketAckMode = 0x01;
  static constexpr u8 PacketNoAckMode = 0x00;

  static constexpr u8 PacketIncrementMode = 0x01;
  static constexpr u8 PacketNoIncrementMode = 0x00;

  static constexpr u8 PacketCRCDraftE = 0x00;
  static constexpr u8 PacketCRCDraftF = 0x01;

  static constexpr u8 DEFAULT_KEY = 0x20;

  static constexpr bool DefaultCRCCheckMode = true;

  static constexpr u8 DefaultLogicalAddress = 0xFE;
};

#endif
