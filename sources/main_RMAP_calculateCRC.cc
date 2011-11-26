/*
 * main_RMAP_CalculateCRC.cc
 *
 *  Created on: Nov 26, 2011
 *      Author: yuasa
 */

/*
 * main_RMAP_interpretAsAnRMAPPacket.cc
 *
 *  Created on: Nov 25, 2011
 *      Author: yuasa
 */

#include "RMAPPacket.hh"
#include "SpaceWireUtilities.hh"
#include "CxxUtilities/String.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;
	if (argc == 1) {
		cerr << "usage : main_RMAP_CalculateCRC (FILE)" << endl;
		cerr << "This program calculates a CRC code for a byte sequence written in the input text file." << endl;
		cerr << "Numbers consisting any of 0-9 will be treated as decimal numbers, and ones start with" << endl;
		cerr << "0x will be hex numbers." << endl;
		cerr << "e.g. An array containing 0x12 0xfe 95 16 will be treated as 18 254 95 16 in decimal." << endl;
		exit(-1);
	}

	string filename(argv[1]);

	string str = File::getAllLinesAsString(filename);
	vector<uint8_t> bytes = String::toUInt8Array(str);

	if(bytes.size()==0){
		return 0;
	}

	uint8_t crc=RMAPUtilities::calculateCRC(bytes);
	cout << "CRC = " << hex << setw(2) << setfill('0') << right << (uint32_t)crc << endl;
}
