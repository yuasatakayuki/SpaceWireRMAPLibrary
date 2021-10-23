/* 
============================================================================
SpaceWire/RMAP Library is provided under the MIT License.
============================================================================

Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
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
		InvalidTargetLogicalAddress=0x0c
	};
	static std::string replyStatusToString(uint8_t status) {
		using namespace std;
		//Status
		std::string statusstring;
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

	static std::string replyStatusToStringWithoutCodeValue(uint8_t status) {
		using namespace std;
		//Status
		std::string statusstring;
		stringstream ss;
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
		default:
			ss << "Reserved (" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) status << ")";
			statusstring = ss.str();
			break;
		}
		return statusstring;
	}
};

#endif /* RMAPREPLYSTATUS_HH_ */
