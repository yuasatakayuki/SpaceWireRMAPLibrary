/*
 * main_SpaceWire_sendreceivePacketsWithScenario.cc
 *
 *  Created on: Dec 21, 2011
 *      Author: yuasa
 */

#include "SpaceWire.hh"
#include "SpaceWireUtilities.hh"
#include "CxxUtilities/CxxUtilities.hh"

using namespace std;
using namespace CxxUtilities;

int nLine = 0;
SpaceWireIFOverTCP* spwif;

void processSend(std::string& line) throw (string) {
	line = String::replace(line, "send", "");
	vector<uint8_t> data = String::toUInt8Array(line);
	if (data.size() != 0) {
		try {
			spwif->send(&(data[0]), data.size());
			cerr << "Line " << nLine << ": sent" << size << "bytes." << endl;
		} catch (...) {
			throw string("senderror");
		}
	} else {
		cerr << "Line " << nLine << ": send skipped (0byte)" << endl;
	}
}

void processReceive(std::string& line) throw (string) {
	vector<uint8_t> data;
	try{
		spwif->receive(&data);
	}catch(...){
		throw string("receiveerror");
	}
	cerr << "Line " << nLine << ": received " << data.size() << "bytes" << endl;
	SpaceWireUtilities::dumpPacket(data);
}

void processLine(std::string& line) throw (string) {
	if (String::contains("send")) {
		processSend(line);
	} else if (String::contains("receive")) {
		processReceive();
	} else {
		cerr << "Line " << nLine << " was skipped (no valid command contained)." << endl;
	}
}

int main(int argc, char* argv[]) {

	if (argc != 4) {
		cerr << "usage : SpaceWire_sendreceivePacketsWithScenario (IP address) (Port Number) (Scenario file)" << endl;
		cerr << "example of scenario file:" << endl;
		cerr << "--------------------------------" << endl;
		cerr << "send 0x12 0x34 0x45 0x67 eop" << endl;
		cerr << "receive" << endl;
		cerr << "send 0x12 0x34 0x45 0x67 eop" << endl;
		cerr << "send 0x12 0x34 0x45 0x67 eop" << endl;
		cerr << "send 0x12 0x34 0x45 0x67 eop" << endl;
		cerr << "receive" << endl;
		cerr << "--------------------------------" << endl;
		exit(-1);
	}

	string ipaddress(argv[1]);
	int portNumber = String::toInteger(argv[2]);
	string filename(argv[3]);

	//check file existence
	if (!File::exists(filename)) {
		cerr << "Scenario file " << filename << " was not found." << endl;
	}

	//initialize SpaceWire interface
	spwif = new SpaceWireIFOverTCP(ipaddress, portNumber);
	spwif->open();

	//execute scenario
	ifstream ifs(filename);
	while (!ifs.eof()) {
		nLine++;
		string line;
		ifs.getline(line);
		try {
			processLine(line);

		} catch (string s) {
			cerr << s << endl;
			cerr << "Error at line " << nLine << endl;
			goto filename;
		} catch (...) {
			cerr << "Error at line " << nLine << endl;
			goto filename;
		}
	}

	finalize: spwif->close();
}
