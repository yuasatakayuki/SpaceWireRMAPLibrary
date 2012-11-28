/*
 * test_SpaceWireIFOverIPClient_timeout.cc
 *
 *  Created on: Oct 21, 2011
 *      Author: yuasa
 */

#ifndef TEST_SPACEWIREIFOVERIPCLIENT_TIMEOUT_CC_
#define TEST_SPACEWIREIFOVERIPCLIENT_TIMEOUT_CC_

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWire.hh"

using namespace std;
using namespace CxxUtilities;

int main(int argc, char* argv[]) {
	SpaceWireIFOverIPClient* spwif = new SpaceWireIFOverIPClient("192.168.30.100", 10030);
	spwif->open();
	spwif->setTimeoutDuration(100);
	try {
		std::vector<uint8_t> data;
		spwif->receive(&data);
	} catch (SpaceWireIFException e) {
		if (e.getStatus() == SpaceWireIFException::Timeout) {
			cerr << "Timeout" << endl;
			exit(-1);
		}
	}
	spwif->close();
	delete spwif;
}

#endif /* TEST_SPACEWIREIFOVERIPCLIENT_TIMEOUT_CC_ */
