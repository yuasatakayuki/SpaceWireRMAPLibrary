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

class SpaceWireTimecodeAction_DumpTimecode : public SpaceWireIFActionTimecodeScynchronizedAction {
public:
	void doAction(uint8_t timecode){
		cout << CxxUtilities::Time::getCurrentTimeAsString() << " Timecode " << hex << setw(2) << setfill('0') << (uint32_t)timecode << " was received." << dec << endl;
	}
};

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
	SpaceWireTimecodeAction_DumpTimecode* timecodeAction_DumpTimecode=new SpaceWireTimecodeAction_DumpTimecode();
	spwif->addTimecodeAction(timecodeAction_DumpTimecode);

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

