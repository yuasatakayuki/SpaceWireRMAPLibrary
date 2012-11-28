/*
 * test_periodic_packet_source.cc
 *
 *  Created on: Oct 21, 2011
 *      Author: yuasa
 */

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWire.hh"

const int PacketLength=47;
const double WaitDuration=1000;

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;

	vector<uint8_t> data;

	vector<uint8_t> path;
	path.push_back(1);

	SpaceWireIF* spwif = new SpaceWireIFOverIPClient("192.168.1.44", 10030);
	try {
		spwif->open();
		int value=0;
		while(true){
			data.clear();
			for(int i=0;i<path.size();i++){
				data.push_back(path[i]);
			}
			for(int i=0;i<PacketLength;i++){
				data.push_back(value+i);
			}
			spwif->send(&data);
			cout << "Sent packet " << PacketLength << "bytes (" << value << ")" << endl;
			value++;
			Condition c;
			c.wait(WaitDuration);
		}
	} catch (...) {
		cerr << "Exception occurred." << endl;
		exit(-1);
	}

}
