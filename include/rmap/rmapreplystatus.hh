#ifndef SPACEWIRE_RMAPREPLYSTATUS_HH_
#define SPACEWIRE_RMAPREPLYSTATUS_HH_

#include <iomanip>
#include <iostream>
#include <string>

#include "spacewire/types.hh"

class RMAPReplyStatus {
 public:
  enum {
    CommandExcecutedSuccessfully = 0x00,
    GeneralError = 0x01,
    UnusedRMAPPacketTypeOrCommandCode = 0x02,
    InvalidDestinationKey = 0x03,
    InvalidDataCRC = 0x04,
    EarlyEOP = 0x05,
    CargoTooLarge = 0x06,
    EEP = 0x07,
    VerifyBufferOverrun = 0x09,
    CommandNotImplementedOrNotAuthorized = 0x0a,
    RMWDataLengthError = 0x0b,
    InvalidTargetLogicalAddress = 0x0c
  };
  static std::string replyStatusToString(u8 status);
  static std::string replyStatusToStringWithoutCodeValue(u8 status);
};

#endif
