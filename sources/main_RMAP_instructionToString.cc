/*
 * main_RMAP_instructionToString.cc
 *
 *  Created on: Nov 26, 2011
 *      Author: yuasa
 */

#include "RMAPPacket.hh"
#include "SpaceWireUtilities.hh"
#include "CxxUtilities/String.hh"
#include <bitset>

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;
	if (argc == 1) {
		cerr << "usage : main_RMAP_instructionToString (value in dec or hex)" << endl;
		cerr << "This program displays information contained in an input instruction field value." << endl;
		exit(-1);
	}

	uint8_t instruction = String::toInteger(string(argv[1]));

	RMAPPacket packet;
	packet.setInstruction(instruction);

	stringstream ss;
	packet.toStringInstructionField(ss);

	bitset<8> a(instruction);
	cout << "Instruction : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t) instruction << " ("
			<< a.to_string() << "b)" << endl;
	cout << ss.str() << endl;
}
