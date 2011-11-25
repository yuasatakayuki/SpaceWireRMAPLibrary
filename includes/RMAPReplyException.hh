/*
 * RMAPReplyException.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef RMAPREPLYEXCEPTION_HH_
#define RMAPREPLYEXCEPTION_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "RMAPReplyStatus.hh"
#include <string>

class RMAPReplyException: public CxxUtilities::Exception, public RMAPReplyStatus {
public:
	RMAPReplyException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}

	std::string toString() {
		switch (status) {
		case RMAPReplyStatus::CommandExcecutedSuccessfully:
			return "CommandExcecutedSuccessfully (0x00)";
		case RMAPReplyStatus::GeneralError:
			return "GeneralError (0x01)";
		case RMAPReplyStatus::UnusedRMAPPacketTypeOrCommandCode:
			return "UnusedRMAPPacketTypeOrCommandCode (0x02)";
		case RMAPReplyStatus::InvalidDestinationKey:
			return "InvalidDestinationKey (0x03)";
		case RMAPReplyStatus::InvalidDataCRC:
			return "InvalidDataCRC (0x04)";
		case RMAPReplyStatus::EarlyEOP:
			return "EarlyEOP (0x05)";
		case RMAPReplyStatus::CargoTooLarge:
			return "CargoTooLarge (0x06)";
		case RMAPReplyStatus::EEP:
			return "EEP (0x07)";
		case RMAPReplyStatus::VerifyBufferOverrun:
			return "VerifyBufferOverrun (0x08)";
		case RMAPReplyStatus::CommandNotImplementedOrNotAuthorized:
			return "CommandNotImplementedOrNotAuthorized (0x0a)";
		case RMAPReplyStatus::RMWDataLengthError:
			return "RMWDataLengthError (0x0b)";
		case RMAPReplyStatus::InvalidDataDestinationLogicalAddress:
			return "InvalidDataDestinationLogicalAddress (0x0c)";
		default:
			using namespace std;
			std::stringstream ss;
			ss << "Undefined status value (0x" << right << setw(2) << setfill('0') << hex << status << ")";
			return ss.str();
		}
	}
};

#endif /* RMAPREPLYEXCEPTION_HH_ */
