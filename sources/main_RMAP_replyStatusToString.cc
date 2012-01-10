/*
 * main_RMAP_replyStatusToString.cc
 *
 *  Created on: Dec 22, 2011
 *      Author: yuasa
 */

#include "CxxUtilities/CxxUtilities.hh"
#include "RMAPReplyStatus.hh"

using namespace std;
using namespace CxxUtilities;

int main(int argc, char* argv[]) {
	if (argc == 1) {
		cerr << "usage   : RMAP_replyStatusToString (status value)" << endl;
		cerr << "example : RMAP_replyStatusToString 0x05" << endl << endl;
		cerr << "RMAP Reply Status:" << endl;
		cerr << "0x00 CommandExcecutedSuccessfully" << endl;
		cerr << "0x01 GeneralError" << endl;
		cerr << "0x02 UnusedRMAPPacketTypeOrCommandCode" << endl;
		cerr << "0x03 InvalidDestinationKey" << endl;
		cerr << "0x04 InvalidDataCRC" << endl;
		cerr << "0x05 EarlyEOP" << endl;
		cerr << "0x06 CargoTooLarge" << endl;
		cerr << "0x07 EEP" << endl;
		cerr << "0x09 VerifyBufferOverrun" << endl;
		cerr << "0x0a CommandNotImplementedOrNotAuthorized" << endl;
		cerr << "0x0b RMWDataLengthError" << endl;
		cerr << "0x0c InvalidDataDestinationLogicalAddress" << endl;
		exit(-1);
	}
	for (int i = 1; i < argc; i++) {
		cout << "String::toUInt8(argv[i])="<<String::toInteger(argv[i]) << endl;
		uint8_t status = String::toInteger(argv[i]);
		cerr << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) status << " "
				<< RMAPReplyStatus::replyStatusToString(status) << endl;
	}
}

