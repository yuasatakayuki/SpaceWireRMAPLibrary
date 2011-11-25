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
};

#endif /* RMAPREPLYSTATUS_HH_ */
