/*
 * tutorial_RMAPPacket_creationInterpretation.cc
 *
 *  Created on: Jan 10, 2012
 *      Author: yuasa
 */

#include "RMAPPacket.hh"
#include "SpaceWireUtilities.hh"

int main(int argc, char* argv[]) {
	using namespace std;

	//Example1 : Manually construct an RMAP packet
	vector<uint8_t> targetSpaceWireAddress;
	targetSpaceWireAddress.push_back(3);
	targetSpaceWireAddress.push_back(10);
	targetSpaceWireAddress.push_back(21);

	vector<uint8_t> replyAddress;
	replyAddress.push_back(5);
	replyAddress.push_back(3);

	uint32_t dataLength = 0x31;
	RMAPPacket rmapPacket1;

	rmapPacket1.setTargetSpaceWireAddress(targetSpaceWireAddress);
	rmapPacket1.setReplyAddress(replyAddress);
	rmapPacket1.setWrite();
	rmapPacket1.setCommand();
	rmapPacket1.setIncrementMode();
	rmapPacket1.setNoVerifyMode();
	rmapPacket1.setExtendedAddress(0x00);
	rmapPacket1.setAddress(0xff803800);
	rmapPacket1.setDataLength(dataLength);
	for (size_t i = 0; i < dataLength; i++) {
		rmapPacket1.addData((uint8_t) i);
	}
	rmapPacket1.constructPacket();
	cout << "RMAPPacket1" << endl;
	SpaceWireUtilities::dumpPacket(rmapPacket1.getPacketBufferPointer());
	cout << "----------------------------" << endl;
	rmapPacket1.setHeaderCRCMode(RMAPPacket::AutoCRC);
	rmapPacket1.constructHeader();
	cout << rmapPacket1.toString() << endl;
	cout << rmapPacket1.toXMLString() << endl;
	cout << endl;

	//Example2 : Interpret a byte sequence as an RMAP packet
	RMAPPacket rmapPacket2;
	uint8_t bytes[] = { 0x07, 0x0B, 0x06, 0x04, 0xFE, 0x01, 0x4F, 0x91, 00, 00, 00, 00, 00, 00, 00, 0x02, 0x0C, 0x0A,
			0x04, 0x06, 0xFE, 0xAD, 0xDF, 0x00, 0xFF, 0x80, 0x11, 0x00, 0x00, 0x00, 0x10, 0x2A };

	try {
		rmapPacket2.interpretAsAnRMAPPacket(bytes, sizeof(bytes));
	} catch (RMAPPacketException e) {
		cerr << "RMAPPacketException " << e.toString() << endl;
		exit(-1);
	}
	cout << "RMAPPacket2" << endl;
	SpaceWireUtilities::dumpPacket(rmapPacket2.getPacketBufferPointer());

	cout << "----------------------------" << endl;
	rmapPacket2.setHeaderCRCMode(RMAPPacket::AutoCRC);
	rmapPacket2.constructHeader();
	cout << rmapPacket2.toString() << endl;
	cout << rmapPacket2.toXMLString() << endl;
}
