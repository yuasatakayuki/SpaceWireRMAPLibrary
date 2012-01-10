/*
 * tutorial_SpaceWireLayer.cc
 *
 *  Created on: Jan 10, 2012
 *      Author: yuasa
 */

#include "SpaceWire.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;

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

	/* Send packet */
	try {
		cout << "Send packet1" << endl;
		uint8_t packet1[] = { 0x0a, 0x0b, 0x0c, 0x0d };
		size_t length1 = 4;
		spwif->send(packet1, length1, SpaceWireEOPMarker::EOP);

		cout << "Send packet2" << endl;
		std::vector<uint8_t> packet2;
		packet2.push_back(0xe);
		packet2.push_back(0xf);
		packet2.push_back(1);
		packet2.push_back(2);
		packet2.push_back(3);
		spwif->send(packet2, SpaceWireEOPMarker::EOP);
	} catch (SpaceWireIFException e) {
		cerr << "Exception when sending a packet." << endl;
		cerr << e.toString() << endl;
		exit(-1);
	}
	cout << "Send packet done" << endl;

	/* Set receive timeout */
	spwif->setTimeoutDuration(1e6);//1sec timeout duration


	/* Receive packet */
	cout << "Receive packet3" << endl;
	try {
		std::vector<uint8_t>* packet3 = spwif->receive();
		cout << "Receive packet3 done (" << packet3->size() << "bytes)" << endl;
		//delete packet3 instance (it was newly constructed by SpaceWireIF internally,
		//and user should delete it to avoid memory leak.
		delete packet3;
	} catch (SpaceWireIFException e) {
		if (e.getStatus() == SpaceWireIFException::Timeout) {
			cerr << "Receive timeout" << endl;
		} else {
			cerr << "Exception when receiving a packet." << endl;
			cerr << e.toString() << endl;
			exit(-1);
		}
	}

	cout << "Receive packet4" << endl;
	try {
		std::vector<uint8_t>* packet4 = new std::vector<uint8_t>();
		spwif->receive(packet4);
		cout << "Receive packet4 done (" << packet4->size() << "bytes)" << endl;
		delete packet4;
	} catch (SpaceWireIFException e) {
		if (e.getStatus() == SpaceWireIFException::Timeout) {
			cerr << "Receive timeout" << endl;
		} else {
			cerr << "Exception when receiving a packet." << endl;
			cerr << e.toString() << endl;
			exit(-1);
		}
	}

	/* Emit timecode */
	cout << "Emit timecode 64times" << endl;
	Condition c;
	try {
		for (uint8_t timecodeValue = 0; timecodeValue < 64; timecodeValue++) {
			cout << "Emitting timecode " << (uint32_t) timecodeValue << endl;
			spwif->emitTimecode(timecodeValue);
			c.wait(1.0 / 64.0); //wait 15.625ms
		}
	} catch (SpaceWireIFException e) {
		cerr << "Exception when receiving a packet." << endl;
		cerr << e.toString() << endl;
		exit(-1);
	}

	/* Close */
	spwif->close();

	delete spwif;
}
