/*
 * main_SpaceWireRTest.cc
 *
 *  Created on: Mar 20, 2014
 *      Author: yuasa
 */

#include "SpaceWire.hh"
#include "RMAP.hh"
#include "ConfigurationPorts/ShimafujiElectricSpaceWireToGigabitEthernetStandalone.hh"
#include "ConfigurationPorts/ShimafujiElectricSpaceWire6PortRouter.hh"

class NonBlockingTest : public CxxUtilities::StoppableThread {
private:
	SpaceWireIFOverTCP* spwif;
	//SpaceWireIFOverTCP* spwif_port7;

private:
	std::string ipAddress;
	uint32_t tcpPortNumber;

public:
	NonBlockingTest(std::string ipAddress, uint32_t tcpPortNumber) {
		this->ipAddress = ipAddress;
		this->tcpPortNumber = tcpPortNumber;
		initializeSpaceWireInterface();
	}

public:
	void initializeSpaceWireInterface() {
		spwif = new SpaceWireIFOverTCP(this->ipAddress, this->tcpPortNumber);
//		spwif_port7 = new SpaceWireIFOverTCP(this->ipAddress, this->tcpPortNumber + 1);
		try {
			spwif->open();
		} catch (SpaceWireIFException& e) {
			using namespace std;
			cerr << "Open(TCP Port " <<  tcpPortNumber << ") failed: " << e.toString() << endl;
			cerr << "IPAddress = " << this->ipAddress << " TCPPort = " << this->tcpPortNumber << endl;
			::exit(-1);
		}
//		try {
//			spwif_port7->open();
//		} catch (SpaceWireIFException& e) {
//			using namespace std;
//			cerr << "Open(Port7) failed: " << e.toString() << endl;
//			cerr << "IPAddress = " << this->ipAddress << " TCPPort = " << this->tcpPortNumber + 1 << endl;
//			::exit(-1);
//		}
		spwif->setTimeoutDuration(3e6);
//		spwif_port7->setTimeoutDuration(3e6);
	}

private:
	uint8_t* buffer;

public:
	void run() {
		using namespace std;
		while (!stopped) {
			cout << "Receiving..." << endl;
			std::vector<uint8_t> receivedPacket_port7;
			try {
				spwif->receive(&receivedPacket_port7);
			} catch (SpaceWireIFException& e) {
				cerr << "Receive failed: " << e.toString() << endl;
				continue;
			}
			cout << "Received " << receivedPacket_port7.size() << " bytes terminated with "
					<< ((spwif->getReceivedPacketEOPMarkerType() == 0x00) ? "EOP" : "EEP") << endl;
		}
	}

public:
	void doSendReceive(size_t length = 1024) {
		this->start();
		using namespace std;
		buffer = new uint8_t[length];
		buffer[0] = 0x01;
		buffer[1] = 0x03;
		buffer[2] = 0x01;
		buffer[3] = 0x06; //Port 6 of SpaceWire-to-GigabitEther
		buffer[4] = 0xFE;
		buffer[5] = 0xAB;
		buffer[6] = 0xAD;
		buffer[7] = 0xCA;
		buffer[8] = 0xFE;
		for (size_t i = 10; i < length; i++) {
			buffer[i] = i;
		}
		cout << "Sending " << length << " bytes..." << endl;
		try {
			spwif->send(buffer, length);
		} catch (SpaceWireIFException& e) {
			cerr << "Send failed: " << e.toString() << endl;
			::exit(-1);
		}
		cout << "Sending " << length << " bytes done." << endl;
		CxxUtilities::Condition c;
		c.wait(1000);
		this->stop();
		this->join();
	}

public:
	void doJamming(size_t length = 1024) {
		this->start();
		using namespace std;
		buffer = new uint8_t[length];
		buffer[0] = 0x03; //Port 3 of SpaceWire-to-GigabitEther
		buffer[1] = 0x03; //Port 3 of RouterA
		buffer[2] = 0x02; //Port 2 of RouterB
		buffer[3] = 0x07; //Invalid port of SpaceWire-to-GigabitEther
		buffer[4] = 0xFE;
		for (size_t i = 5; i < length; i++) {
			buffer[i] = i;
		}
		CxxUtilities::Condition c;
		while(true){
			try{
				spwif->send(buffer,300,SpaceWireEOPMarker::Continued);
				c.wait(2000);
				spwif->send(buffer,length/10,SpaceWireEOPMarker::EEP);
			cout << "Sent " << length << " bytes." << endl;
			}catch(SpaceWireIFException& e){
				cerr << "Error while sending jam packets: " << e.toString() << endl;
				::exit(-1);
			}
		}
	}

};

class RouterConfigurator {
private:
	ShimafujiElectricSpaceWireToGigabitEthernetStandalone* router;
	ShimafujiElectricSpaceWire6PortRouter* router6a;
	ShimafujiElectricSpaceWire6PortRouter* router6b;

	RMAPTargetNodeDB db;

private:
	SpaceWireIFOverTCP* spwif;
	RMAPEngine* rmapEngine;
	RMAPInitiator* rmapInitiator;

private:
	std::string ipAddress;
	uint32_t tcpPortNumber;

public:
	RouterConfigurator(std::string ipAddress, uint32_t tcpPortNumber) {
		//set instance variables
		this->ipAddress = ipAddress;
		this->tcpPortNumber = tcpPortNumber;

		router = new ShimafujiElectricSpaceWireToGigabitEthernetStandalone();
		router6a = new ShimafujiElectricSpaceWire6PortRouter();
		router6b = new ShimafujiElectricSpaceWire6PortRouter();

		initializeSpaceWireAndRMAP();
		setRouterInstances();
	}

public:
	~RouterConfigurator() {
		finalizeSpaceWireAndRMAP();
		using namespace std;
		cout << "Bye." << endl;
	}

public:
	void setRouterInstances() {
		std::vector<uint8_t> targetSpaceWireAddress;
		std::vector<uint8_t> replyAddress;
		router->setRMAPInitiator(rmapInitiator);
		router6a->setRMAPInitiator(rmapInitiator);
		router6b->setRMAPInitiator(rmapInitiator);

		//router6a
		targetSpaceWireAddress.clear();
		replyAddress.clear();
		targetSpaceWireAddress.push_back(0x01);
		targetSpaceWireAddress.push_back(0x00);
		replyAddress.push_back(0x06);
		router6a->applyNewPathAddresses(targetSpaceWireAddress, replyAddress);

		//router6a
		targetSpaceWireAddress.clear();
		replyAddress.clear();
		targetSpaceWireAddress.push_back(0x02);
		targetSpaceWireAddress.push_back(0x00);
		replyAddress.push_back(0x06);
		router6b->applyNewPathAddresses(targetSpaceWireAddress, replyAddress);
	}

private:
	void initializeSpaceWireAndRMAP() {
		using namespace std;
		using namespace CxxUtilities;

		cout << "Opening SpaceWireIF...";
		spwif = new SpaceWireIFOverTCP(ipAddress, tcpPortNumber);
		try {
			spwif->open();
		} catch (...) {
			cerr << "Connection timed out. SpaceWire-to-GigabitEther was not found." << endl;
			exit(-1);
		}
		cout << "done" << endl;
		Condition spwifWait;
		spwifWait.wait(100);

		//RMAPEngine/RMAPInitiator part
		rmapEngine = new RMAPEngine(spwif);
		rmapEngine->start();
		rmapInitiator = new RMAPInitiator(rmapEngine);
		rmapInitiator->setInitiatorLogicalAddress(0xFE);
	}

public:
	void finalizeSpaceWireAndRMAP() {
		using namespace std;
		cout << "Stopping RMAP Engine..." << endl;
		rmapEngine->stop();
		delete rmapEngine;
		spwif->close();
		delete spwif;
		cout << "Disconnecting SpaceWire-to-GigabitEther..." << endl;

		delete router;
		delete router6a;
		delete router6b;
	}

public:
	void setLinkRate(uint32_t port, double frequency) {
		using namespace std;
		if (router->isSpecifiedLinkFrequencyAvailable(frequency) == false) {
			cerr << "Specified link rate is not available on this port." << endl;
			return;
		}
		using namespace std;
		router->setLinkFrequency(port, frequency);
		cout << "Link rate of Port " << (uint32_t) port << " was changed to " << frequency << " MHz." << endl;
		readLinkCSRThenDump(port);
	}

public:
	void readLinkCSRThenDump(uint8_t port) {
		if (isPortNumberValid(port)) {
			using namespace std;
			cout << router->getLinkCSRAsString(port) << endl;
		}
	}

public:
	bool isPortNumberValid(uint8_t port) {
		if (port > ShimafujiElectricSpaceWireToGigabitEthernetStandalone::NumberOfExternalPorts) {
			using namespace std;
			cerr << "Port number is invalid." << endl;
			return false;
		} else {
			return true;
		}
	}

private:
	std::vector<uint8_t> portNumberVector(uint8_t port) {
		std::vector<uint8_t> result;
		result.push_back(port);
		return result;
	}
public:
	void doConfigurationForNonBlockingTest() {
		using namespace std;
		cout << "Configuring SpaceWire-R Router A and B..." << endl;
		//link rate of SpaceWire-to-GigabitEther
		for (size_t i = 0; i < 4; i++) {
			router->setLinkEnable(i);
			router->setLinkFrequency(i, 100);
		}
		//for slow jamming packet
		router->setLinkFrequency(2, 5);

		size_t nTimeout = 0;
		const size_t nTimeoutMax = 10;

		for (size_t i = 0; i < 3; i++) {
			//link enbale and link frequency
			linkEnableAndLinkFrequencyA: try {
				router6a->setLinkEnable(i);
				router6a->setLinkFrequency(i, 100);
			} catch (...) {
				if (nTimeout < nTimeoutMax) {
					cerr << "Retrying (RouterA)" << endl;
					nTimeout++;
					goto linkEnableAndLinkFrequencyA;
				}
				using namespace std;
				cerr << *(uint32_t*) 0 << endl;
			}

			linkEnableAndLinkFrequencyB: nTimeout = 0;
			try {
				router6b->setLinkEnable(i);
				router6b->setLinkFrequency(i, 100);
			} catch (...) {
				if (nTimeout < nTimeoutMax) {
					nTimeout++;
					cerr << "Retrying (RouterB)" << endl;
					goto linkEnableAndLinkFrequencyB;
				}
				using namespace std;
				cerr << *(uint32_t*) 0 << endl;
			}
		}

		try {
			//virtual logical address
			router6a->setVirtualLogicalAddress(0, 0x20);
			router6a->setVirtualLogicalAddress(1, 0x21);
			router6a->setVirtualLogicalAddress(2, 0x22);

			router6b->setVirtualLogicalAddress(0, 0x30);
			router6b->setVirtualLogicalAddress(1, 0x31);
			router6b->setVirtualLogicalAddress(2, 0x32);
		} catch (...) {
			using namespace std;
			cerr << *(uint32_t*) 0 << endl;
		}

		//configuration table
		router6a->writeRoutingTable(0x20, portNumberVector(0));
		router6a->writeRoutingTable(0x21, portNumberVector(1));
		router6a->writeRoutingTable(0x22, portNumberVector(2));
		router6a->writeRoutingTable(0x30, portNumberVector(3));
		router6a->writeRoutingTable(0x31, portNumberVector(3));
		router6a->writeRoutingTable(0x32, portNumberVector(3));

		router6b->writeRoutingTable(0x30, portNumberVector(0));
		router6b->writeRoutingTable(0x31, portNumberVector(1));
		router6b->writeRoutingTable(0x32, portNumberVector(2));
		router6b->writeRoutingTable(0x20, portNumberVector(3));
		router6b->writeRoutingTable(0x21, portNumberVector(3));
		router6b->writeRoutingTable(0x22, portNumberVector(3));

		cout << "Done" << endl;
	}
};

class Jammer : public CxxUtilities::StoppableThread {
private:
	size_t length;

public:
	Jammer(size_t length){
		this->length=length;
	}

public:
	void run(){
		NonBlockingTest* jamming = new NonBlockingTest("192.168.1.100", 10031);
		jamming->doJamming(length);
	}
};

int main(int argc, char* argv[]) {
	std::string command(argv[1]);

	if (command == "c") {//router configuration
		RouterConfigurator* configurator = new RouterConfigurator("192.168.1.100", 10030);
		configurator->doConfigurationForNonBlockingTest();
		delete configurator;
	}
	//SpaceWire-to-GigabitEther connection is freed.

	if (command == "t") {//send/receive test
		size_t length = 1024;
		if (argc > 2) {
			length = atoi(argv[2]);
		}
		NonBlockingTest* nonBlockingTest = new NonBlockingTest("192.168.1.100", 10030);
		nonBlockingTest->doSendReceive(length);
	}

	if (command == "j") {//jamming
		size_t length = 1024;
		if (argc > 2) {
			length = atoi(argv[2]);
		}
		NonBlockingTest* nonBlockingTest = new NonBlockingTest("192.168.1.100", 10031);
		nonBlockingTest->doJamming(length);
	}

	if (command == "tj" || command == "jt") {//send/receive test + jamming
		size_t length = 1024;
		if (argc > 2) {
			length = atoi(argv[2]);
		}
		Jammer* jamer=new Jammer(length);
		jamer->start();
		CxxUtilities::Condition c;
		c.wait(1000);
		NonBlockingTest* nonBlockingTest = new NonBlockingTest("192.168.1.100", 10030);
		nonBlockingTest->doSendReceive(512);
		c.wait(5000);
	}


}
