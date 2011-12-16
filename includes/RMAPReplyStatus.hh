/*
 * RMAPReplyStatus.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef RMAPREPLYSTATUS_HH_
#define RMAPREPLYSTATUS_HH_

class RMAPReplyStatus {
public:
	enum {
		CommandExcecutedSuccessfully=0x00,
		GeneralError=0x01,
		UnusedRMAPPacketTypeOrCommandCode=0x02,
		InvalidDestinationKey=0x03,
		InvalidDataCRC=0x04,
		EarlyEOP=0x05,
		CargoTooLarge=0x06,
		EEP=0x07,
		VerifyBufferOverrun=0x09,
		CommandNotImplementedOrNotAuthorized=0x0a,
		RMWDataLengthError=0x0b,
		InvalidDataDestinationLogicalAddress=0x0c
	};
	static std::string replyStatusToString(uint8_t status) {
		using namespace std;
		//Status
		string statusstring;
		stringstream ss;
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
		default:
			ss << "Reserved (" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) status << ")";
			statusstring = ss.str();
			break;
		}
		return statusstring;
	}
};

#endif /* RMAPREPLYSTATUS_HH_ */
