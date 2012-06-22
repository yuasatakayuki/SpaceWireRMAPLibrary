/*
 * main_SpaceWire_packetSink.cc
 *
 *  Created on: Jun 22, 2012
 *      Author: yuasa
 */

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWire.hh"

using namespace CxxUtilities;
using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cerr << "Give IP address and port number of SpaceWire-to-GigabitEther" << endl;
		exit(-1);
	}

	string ipaddress(argv[1]);
	int portNumber = String::toInteger(argv[2]);

	SpaceWireIFOverTCP* spwif = new SpaceWireIFOverTCP(ipaddress, portNumber);
	try {
		spwif->open();
	} catch (SpaceWireIFException& e) {
		cerr << "Could not connect to " << ipaddress << "." << endl;
		exit(-1);
	}

	std::vector<uint8_t> buffer;
	while (true) {
		try {
			cout << "Waiting for a packet." << endl;
			spwif->receive(&buffer);
			SpaceWireUtilities::dumpPacket(buffer);
		} catch (SpaceWireIFException& e) {
			if (e.getStatus() == SpaceWireIFException::Timeout) {
				cout << "Timeout." << endl;
			} else {
				cerr << "Exception while receiving a packet (status=" << e.toString() << ")" << endl;
				goto finalize;
			}
		}
	}
	finalize: //
	spwif->close();
	delete spwif;
}

