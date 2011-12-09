/*
 * test_RMAPPacket.cc
 *
 *  Created on: Aug 10, 2011
 *      Author: yuasa
 */

#include "RMAPPacket.hh"
#include "SpaceWireUtilities.hh"

int main(int argc,char* argv[]){
	using namespace std;

	vector<uint8_t> targetSpaceWireAddress;
	targetSpaceWireAddress.push_back(3);
	targetSpaceWireAddress.push_back(10);
	targetSpaceWireAddress.push_back(21);

	vector<uint8_t> replyAddress;
	replyAddress.push_back(5);
	replyAddress.push_back(3);

	//construct
	uint32_t dataLength=0x31;
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
	for(size_t i=0;i<dataLength;i++){
		rmapPacket1.addData((uint8_t)i);
	}
	rmapPacket1.constructPacket();
	cout << "RMAPPacket1" << endl;
	SpaceWireUtilities::dumpPacket(rmapPacket1.getPacketBufferPointer());
	cout << "----------------------------" << endl;
	rmapPacket1.setHeaderCRCMode(RMAPPacket::AutoCRC);
	rmapPacket1.constructHeader();
	cout << rmapPacket1.toString() << endl;
	cout << rmapPacket1.toXMLString() << endl;



	//interpret
	RMAPPacket rmapPacket2;
	
	uint8_t ppp[]={0x07, 0x0B, 0x06, 0x04, 0xFE, 0x01, 0x4F, 0x91, 00, 00, 00, 00, 00, 00, 00, 0x02, 0x0C, 0x0A, 0x04, 0x06, 0xFE, 0xAD, 0xDF, 0x00, 0xFF, 0x80, 0x11, 0x00,0x00,0x00,0x10,0x2A};
	
	//rmapPacket2.interpretAsAnRMAPPacket(rmapPacket1.getPacketBufferPointer());
	rmapPacket2.interpretAsAnRMAPPacket(ppp,sizeof(ppp));
	cout << "RMAPPacket2" << endl;
	SpaceWireUtilities::dumpPacket(rmapPacket2.getPacketBufferPointer());

	if(rmapPacket1.getPacket()==rmapPacket2.getPacket()){
		std::cout << "the same" << endl;
	}else{

	}
	cout << rmapPacket2.toString() << endl;

	cout << "----------------------------" << endl;
	rmapPacket2.setHeaderCRCMode(RMAPPacket::AutoCRC);
	rmapPacket2.constructHeader();
	cout << rmapPacket2.toString() << endl;
	cout << rmapPacket2.toXMLString() << endl;
}
