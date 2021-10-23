#include "rmap/rmapreplystatus.hh"

std::string RMAPReplyStatus::replyStatusToString(u8 status) {
  std::string statusstring;
  switch (status) {
    case 0x00:
      statusstring = "Successfully Executed (0x00)";
      break;
    case 0x01:
      statusstring = "General Error (0x01)";
      break;
    case 0x02:
      statusstring = "Unused RMAP Packet Type or Command Code (0x02)";
      break;
    case 0x03:
      statusstring = "Invalid Target Key (0x03)";
      break;
    case 0x04:
      statusstring = "Invalid Data CRC (0x04)";
      break;
    case 0x05:
      statusstring = "Early EOP (0x05)";
      break;
    case 0x06:
      statusstring = "Cargo Too Large (0x06)";
      break;
    case 0x07:
      statusstring = "EEP (0x07)";
      break;
    case 0x08:
      statusstring = "Reserved (0x08)";
      break;
    case 0x09:
      statusstring = "Verify Buffer Overrun (0x09)";
      break;
    case 0x0a:
      statusstring = "RMAP Command Not Implemented or Not Authorized (0x0a)";
      break;
    case 0x0b:
      statusstring = "Invalid Target Logical Address (0x0b)";
      break;
    default: {
      std::stringstream ss;
      ss << "Reserved ("
         << "0x" << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<u32>(status) << ")";
      statusstring = ss.str();
      break;
    }
  }
  return statusstring;
}

std::string RMAPReplyStatus::replyStatusToStringWithoutCodeValue(u8 status) {
  std::string statusstring;
  switch (status) {
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
      statusstring = "RMW Data Length Error";
      break;
    case 0x0c:
      statusstring = "Invalid Target Logical Address";
      break;
    default: {
      std::stringstream ss;
      ss << "Reserved ("
         << "0x" << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<u32>(status) << ")";
      statusstring = ss.str();
      break;
    }
  }
  return statusstring;
}
