/*
 * main_RouterConfigurator.cc
 *
 *  Created on: Nov 16, 2012
 *      Author: yuasa
 */

#include "SpaceWire.hh"
#include "RMAP.hh"
#include "ConfigurationPorts/ShimafujiElectricSpaceWireToGigabitEthernetStandalone.hh"

class RouterConfigurator {
private:
	RMAPTargetNodeDB db;
	RMAPTargetNode* rmapTargetRouterConfigurationPort;

	ShimafujiElectricSpaceWireToGigabitEthernetStandalone* router;

private:
	SpaceWireIFOverTCP* spwif;
	RMAPEngine* rmapEngine;
	RMAPInitiator* rmapInitiator;

private:
	std::string ipAddress;

public:
	RouterConfigurator(std::string ipAddress) {
		//set instance variables
		this->ipAddress = ipAddress;

		router = new ShimafujiElectricSpaceWireToGigabitEthernetStandalone();

		findRMAPTargetNode();
		initializeSpaceWireAndRMAP();
		router->setRMAPInitiator(rmapInitiator);
		checkIRouterIsAccessible();
	}

public:
	~RouterConfigurator() {
		finalizeSpaceWireAndRMAP();
		using namespace std;
		cout << "Bye." << endl;
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
	}

public:
	void checkIRouterIsAccessible() {
		using namespace std;
		uint8_t value[4];
		try {
			router->readLinkControlStatusRegister(1, value);
		} catch (RouterConfigurationPortException& e) {
			cerr << e.toString() << endl;
			cerr << "Router Configuration Port is not accessible. Reset SpaceWire-to-GigabitEther." << endl;
			exit(-1);
		} catch (...) {
			cerr << "Router Configuration Port is not accessible. Reset SpaceWire-to-GigabitEther." << endl;
			exit(-1);
		}
		cout << "Router Configuration Port is accessible." << endl;
	}

public:
	void initializeSpaceWireAndRMAP() {
		using namespace std;
		using namespace CxxUtilities;

		cout << "Opening SpaceWireIF...";
		spwif = new SpaceWireIFOverTCP(ipAddress, 10030);
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
	void findRMAPTargetNode() {
		using namespace std;
		rmapTargetRouterConfigurationPort = router->getRMAPTargetNodeInstance();
		if (rmapTargetRouterConfigurationPort == NULL) {
			cerr << "Configuration port information is missing in the SpaceWire RMAP Library internal DB." << endl;
			exit(-1);
		}
		cout << "RMAP Target information of Router Configuration Port was found." << endl;
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

public:
	void dumpAllLinkCSRRegisters() {
		using namespace std;
		cout << "---------------------------------------------" << endl;
		cout << "Link Control Registers" << endl;
		for (size_t i = 0; i < ShimafujiElectricSpaceWireToGigabitEthernetStandalone::NumberOfExternalPorts; i++) {
			readLinkCSRThenDump(i + 1);
		}
		cout << "Port 6(0x06) " << setw(8) << setfill(' ') << left << "Enabled" << " TCPPort = 10030" << endl;
		cout << "Port 7(0x07) " << setw(8) << setfill(' ') << left << "Enabled" << " TCPPort = 10031" << endl;
		cout << "---------------------------------------------" << endl;
	}
public:
	void readLinkCSRThenDump(uint8_t port) {
		if (isPortNumberValid(port)) {
			using namespace std;
			cout << router->getLinkCSRAsString(port) << endl;
		}
	}

public:
	void setLinkRate(uint32_t port, double frequency) {
		using namespace std;
		if (router->isSpecifiedLinkFrequencyAvailable(frequency) == false) {
			cerr << "Specified link rate is not available on this port." << endl;
			return;
		}
		if (isPortNumberValid(port)) {
			using namespace std;
			router->setLinkFrequency(port, frequency);
			cout << "Link rate of Port " << (uint32_t) port << " was changed to " << frequency << " MHz." << endl;
			readLinkCSRThenDump(port);
		}
	}

public:
	void setLinkEnable(uint32_t port) {
		if (isPortNumberValid(port)) {
			router->setLinkEnable(port);
			using namespace std;
			cout << "Port " << (uint32_t) port << " was enabled." << endl;
			readLinkCSRThenDump(port);
		}
	}

public:
	void setLinkDisable(uint32_t port) {
		if (isPortNumberValid(port)) {
			router->setLinkDisable(port);
			using namespace std;
			cout << "Port " << (uint32_t) port << " was disabled." << endl;
			readLinkCSRThenDump(port);
		}
	}

public:
	void dumpRevisionRegisters() {
		using namespace std;
		cout << "---------------------------------------------" << endl;
		cout << router->getRevisionsAsString() << endl;
		cout << "---------------------------------------------" << endl;
	}

public:
	void showHelp() {
		using namespace std;
		cout << "---------------------------------------------" << endl;
		cout << "Available commands are:" << endl;
		cout << "  help                                  : print this help message" << endl;
		cout << "  quit                                  : quit the program" << endl;
		cout << "  linkCSRAll                            : read all Link Control and Status Registers" << endl;
		cout << "  all                                   : the same as above" << endl;
		cout << "  linkCSR [port]                        : read a Link Control and Status Register" << endl;
		cout << "  csr     [port]                        : the same as above" << endl;
		cout << "  linkRate [port] [freq MHz]            : set the link speed of a port" << endl;
		cout << "  rate     [port] [freq MHz]            : the same as above" << endl;
		cout << "  linkEnable [port] [enable/disable]    : enable/disable link" << endl;
		cout << "  enable [port]                         : enable link" << endl;
		cout << "  disable [port]                        : disable link" << endl;
		cout << "  revisions                             : read revision registers" << endl;
		cout << "---------------------------------------------" << endl;
	}

public:
	void showMessageForInvalidCommand() {
		using namespace std;
		cerr << "Invalid command." << endl;
		showHelp();
	}

public:
	void interactiveSession() {
		using namespace std;
		cout << "RouterConfigurator interactive session starts." << endl << endl;

		cout << "============================================" << endl;
		cout << "SpaceWire-to-GigabitEther Internal Router" << endl;
		cout << "============================================" << endl;
		dumpRevisionRegisters();
		dumpAllLinkCSRRegisters();

		std::string command;
		bool stopped = false;

		while (!stopped) {
			cout << "Command (or type help) > ";
			cin >> command;
			command = CxxUtilities::String::toLowerCase(command);
			if (command == "help" || command == "h" || command == "?") {
				showHelp();
			} else if (command == "quit" || command == "q" || command == "exit" || command == "ex") {
				stopped = true;
				cout << "Quitting." << endl;
			} else if (command == "linkcsrall" || command == "all") {
				dumpAllLinkCSRRegisters();
			} else if (command == "linkcsr" || command == "csr") {
				uint32_t port;
				cin >> port;
				readLinkCSRThenDump(port);
			} else if (command == "linkrate" || command == "rate") {
				uint32_t port;
				double frequency;
				cin >> port >> frequency;
				setLinkRate(port, frequency);
			} else if (command == "linkenable") {
				uint32_t port;
				string enadis;
				cin >> port >> enadis;
				if (enadis == "enable" || enadis == "ena") {
					setLinkEnable(port);
				} else if (enadis == "disable" || enadis == "dis") {
					setLinkDisable(port);
				} else {
					showMessageForInvalidCommand();
				}
			} else if (command == "enable" || command == "ena") {
				uint32_t port;
				cin >> port;
				setLinkEnable(port);
			} else if (command == "disable" || command == "dis") {
				uint32_t port;
				cin >> port;
				setLinkDisable(port);
			} else if (command == "revisions") {
				dumpRevisionRegisters();
			} else {
				showMessageForInvalidCommand();
			}
		}
	}
};

int main(int argc, char* argv[]) {
	using namespace std;

	RouterConfigurator configurator("192.168.1.100");
	configurator.interactiveSession();

}
