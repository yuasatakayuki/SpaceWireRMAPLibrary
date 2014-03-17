/*
 * tutorial_RMAPLayer.cc
 *
 *  Created on: Jan 10, 2012
 *      Author: yuasa
 */

#include "SpaceWire.hh"
#include "RMAP.hh"

double readTimeoutDuration = 1000;//1000ms
double writeTimeoutDuration = 1000;//1000ms


uint32_t getRoutingTableRegisterAddress(uint8_t logicalAddress){
	uint32_t baseAddress=0x0080;
	return baseAddress+ (logicalAddress-32)*4;
}

void dump(uint8_t* buffer, size_t length, uint32_t address){
	using namespace std;
		cout << "0x" << hex << right << setw(4) << setfill('0')  << (uint32_t)address << " : ";
		for(size_t i=0;i<length;i++){
			cout << "0x" << hex << right << setw(2) << setfill('0')  << (uint32_t)buffer[i] << " ";
		}
		cout << endl;
}

void setRouter1(RMAPInitiator* rmapInitiator){
	using namespace std;
	cout << "setRouter1()" << endl;
	RMAPTargetNode* router1ConfigurationPort = new RMAPTargetNode();
	router1ConfigurationPort->setTargetLogicalAddress(0xfe);
	router1ConfigurationPort->setDefaultKey(0x02);
	std::vector<uint8_t> targetSpaceWireAddress;
	targetSpaceWireAddress.push_back(0x01);
	targetSpaceWireAddress.push_back(0x00);
	router1ConfigurationPort->setTargetSpaceWireAddress(targetSpaceWireAddress);
	std::vector<uint8_t> replyAddress;
	replyAddress.push_back(0x06);
	router1ConfigurationPort->setReplyAddress(replyAddress);
	cout << router1ConfigurationPort->toString() << endl;

	try {
		uint32_t length = 4;
		uint32_t address = 0x0470;
		uint8_t* buffer = new uint8_t[(size_t) length];

		cout << "setting virtual Logical Addresses..." << endl;
		buffer[0]=0x00;
		buffer[1]=0x00;
		buffer[2]=0x00;
		buffer[3]=0x00;

		//Port 1 Logical Address
		address=0x470;
		buffer[0]=0x20;
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Port 2 Logical Address
		address=0x474;
		buffer[0]=0x21;
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Port 3 Logical Address
		address=0x478;
		buffer[0]=0x22;
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		cout << "setting Routing Table..." << endl;
		
		// Router1 Ports

		//Routing Table (LA=0x20)
		address=getRoutingTableRegisterAddress(0x20);//Router1 Port0
		buffer[0]=0x00;//Router to Port0
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x21)
		address=getRoutingTableRegisterAddress(0x21);//Router1 Port1
		buffer[0]=0x02;//Router to Port1
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x22)
		address=getRoutingTableRegisterAddress(0x22);//Router1 Port2
		buffer[0]=0x04;//Router to Port2
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		// Router2 Ports

		//Routing Table (LA=0x30)
		address=getRoutingTableRegisterAddress(0x30);//Router2 Port0
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x31)
		address=getRoutingTableRegisterAddress(0x31);//Router2 Port1
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x32)
		address=getRoutingTableRegisterAddress(0x32);//Router2 Port2
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router1ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router1ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		delete buffer;
		delete router1ConfigurationPort;

		cout << "Router1 configuration done" << endl;
	} catch (RMAPInitiatorException e) {
		cerr << "RMAPInitiatorException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPReplyException e) {
		cerr << "RMAPReplyException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPEngineException e) {
		cerr << "RMAPEngineException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (...) {
		cerr << "Unkown error" << endl;
		exit(-1);
	}
}

void setRouter2(RMAPInitiator* rmapInitiator){
	using namespace std;
	cout << "setRouter2()" << endl;
	RMAPTargetNode* router2ConfigurationPort = new RMAPTargetNode();
	router2ConfigurationPort->setTargetLogicalAddress(0xfe);
	router2ConfigurationPort->setDefaultKey(0x02);
	std::vector<uint8_t> targetSpaceWireAddress;
	targetSpaceWireAddress.push_back(0x02);
	targetSpaceWireAddress.push_back(0x00);
	router2ConfigurationPort->setTargetSpaceWireAddress(targetSpaceWireAddress);
	std::vector<uint8_t> replyAddress;
	replyAddress.push_back(0x06);
	router2ConfigurationPort->setReplyAddress(replyAddress);
	cout << router2ConfigurationPort->toString() << endl;

	try {
		uint32_t length = 4;
		uint32_t address = 0x0470;
		uint8_t* buffer = new uint8_t[(size_t) length];
		buffer[0]=0x00;
		buffer[1]=0x00;
		buffer[2]=0x00;
		buffer[3]=0x00;

		cout << "setting virtual Logical Addresses..." << endl;

		//Port 1 Logical Address
		address=0x470;
		buffer[0]=0x30;
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Port 2 Logical Address
		address=0x474;
		buffer[0]=0x31;
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Port 3 Logical Address
		address=0x478;
		buffer[0]=0x32;
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		cout << "setting Routing Table..." << endl;
		
		// Router1 Ports

		//Routing Table (LA=0x20)
		address=getRoutingTableRegisterAddress(0x20);//Router1 Port0
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x21)
		address=getRoutingTableRegisterAddress(0x21);//Router1 Port1
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x22)
		address=getRoutingTableRegisterAddress(0x22);//Router1 Port2
		buffer[0]=0x08;//Router to Port3
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		// Router2 Ports

		//Routing Table (LA=0x30)
		address=getRoutingTableRegisterAddress(0x30);//Router2 Port0
		buffer[0]=0x00;//Router to Port0
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x31)
		address=getRoutingTableRegisterAddress(0x31);//Router2 Port1
		buffer[0]=0x02;//Router to Port1
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		//Routing Table (LA=0x32)
		address=getRoutingTableRegisterAddress(0x32);//Router2 Port2
		buffer[0]=0x04;//Router to Port2
		rmapInitiator->write(router2ConfigurationPort, address, buffer, length, writeTimeoutDuration);
		rmapInitiator->read(router2ConfigurationPort, address, length, buffer, readTimeoutDuration);
		dump(buffer,length,address);

		delete buffer;
		delete router2ConfigurationPort;

		cout << "Router2 configuration done" << endl;
	} catch (RMAPInitiatorException e) {
		cerr << "RMAPInitiatorException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPReplyException e) {
		cerr << "RMAPReplyException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPEngineException e) {
		cerr << "RMAPEngineException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (...) {
		cerr << "Unkown error" << endl;
		exit(-1);
	}
}

int main(int argc, char* argv[]) {
	using namespace std;

	/* Open the SpaceWire interface */
	cout << "Opening SpaceWireIF...";
	SpaceWireIF* spwif = new SpaceWireIFOverIPClient("192.168.1.100", 10030);
	try {
		spwif->open();
	} catch (...) {
		cerr << "Connection timed out." << endl;
		exit(-1);
	}
	cout << "done" << endl;

	/* Construct and start RMAP Engine */
	RMAPEngine* rmapEngine = new RMAPEngine(spwif);
	rmapEngine->start();

	/* Construct an RMAP Initiator instance */
	RMAPInitiator* rmapInitiator = new RMAPInitiator(rmapEngine);
	rmapInitiator->setInitiatorLogicalAddress(0xFE);

	setRouter1(rmapInitiator);
	setRouter2(rmapInitiator);

	rmapEngine->stop();

	const size_t PacketSize=1024;
	uint8_t* buffer=new uint8_t[PacketSize];
	buffer[0]=0x01;
	buffer[1]=0x03;
	buffer[2]=0x01;
	buffer[3]=0x06;
	buffer[4]=0xFE;
	for(size_t i=5;i<PacketSize;i++){
		buffer[i]=(uint8_t)i;
	}
	spwif->send(buffer, PacketSize);
	spwif->setTimeoutDuration(1e6);
	try{
		std::vector<uint8_t>* receivedPacket = spwif->receive();
	}catch(SpaceWireIFException& e){
		cout << "Receive Exception : " << e.toString() << endl;
	}
}
