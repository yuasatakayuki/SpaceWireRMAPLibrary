/*
 * test_SpaceWireR_sendReceive.cc
 *
 *  Created on: Dec 10, 2012
 *      Author: yuasa
 */

#include "SpaceWireR.hh"
#include "CxxUtilities/CxxUtilities.hh"


const uint16_t channelID=0x6342;
const size_t SendSize = 4096*1024;


class ToStringInterface {
public:
	virtual std::string toString() =0;
};

class SpaceWireIFInterface {
protected:
	SpaceWireIFOverTCP* spwif;
protected:
	std::string url;
	int port;
	bool isSpaceWireIFOverTCPClientMode;

public:
	SpaceWireIFInterface(std::string url, int port) {
		this->url = url;
		this->port = port;
		this->isSpaceWireIFOverTCPClientMode = isSpaceWireIFOverTCPClientMode = true;
	}

public:
	SpaceWireIFInterface(int port) {
		this->port = port;
		this->isSpaceWireIFOverTCPClientMode = isSpaceWireIFOverTCPClientMode = false;
	}

public:
	void openSpaceWireIF() throw (SpaceWireIFException) {
		if (isSpaceWireIFOverTCPClientMode) {
			spwif = new SpaceWireIFOverTCP(url, port);
		} else {
			spwif = new SpaceWireIFOverTCP(port);
		}
		spwif->open();
	}
};

class Sender: public CxxUtilities::StoppableThread, public ToStringInterface, public SpaceWireIFInterface {
private:
	uint8_t channelID;

public:
	SpaceWireRTransmitTEP* tep;

public:
	Sender(uint8_t channelID, std::string url, int port) :
			SpaceWireIFInterface(url, port) {
		this->channelID = channelID;
	}

public:
	Sender(uint8_t channelID, int port) :
			SpaceWireIFInterface(port) {
		this->channelID = channelID;
	}

public:
	~Sender() {
	}

public:
	void run() {
		using namespace std;

		//create SpaceWire IF
		cout << "Waiting for a new connection." << endl;
		this->openSpaceWireIF();
		cout << "Connected." << endl;

		//start SpaceWireR Engine
		SpaceWireREngine* spwREngine = new SpaceWireREngine(spwif);
		spwREngine->start();
		cout << "SpaceWireREngine stated" << endl;

		//create a TEP instance
		std::vector<uint8_t> emptyVector;
		tep = new SpaceWireRTransmitTEP(spwREngine, channelID, 0xFE, emptyVector, 0xFE, emptyVector);
		tep->open();
		cout << "SpaceWireRTransmitTEP opened." << endl;

		std::vector<uint8_t> data;
		for (size_t i = 0; i < SendSize; i++) {
			data.push_back(i);
		}

		tep->setSegmentSize(1024);
		sendloop: try {
			while (!stopped) {
				tep->send(&data,5000);
				cout << "Sent " << data.size() << " bytes." << endl;
			}
		} catch (SpaceWireRTEPException& e) {
			cout << "Sender: " << e.toString() << endl;
			if (e.getStatus() == SpaceWireRTEPException::Timeout) {
				goto sendloop;
			}
		}

		//finalize
		spwREngine->stop();
		spwREngine->waitUntilRunMethodComplets();
		delete spwREngine;
		spwif->close();
		delete spwif;
	}

	std::string toString() {
		if (tep != NULL) {
			return tep->toString();
		} else {
			return "";
		}
	}
};

class Receiver: public CxxUtilities::StoppableThread, public ToStringInterface, public SpaceWireIFInterface {
private:
	uint8_t channelID;

public:
	SpaceWireRReceiveTEP* tep;

public:
	Receiver(uint8_t channelID, std::string url, int port) :
			SpaceWireIFInterface(url, port) {
		this->channelID = channelID;
	}

public:
	Receiver(uint8_t channelID, int port) :
			SpaceWireIFInterface(port) {
		this->channelID = channelID;
	}

public:
	virtual ~Receiver() {
	}

public:
	void run() {
		using namespace std;

		//create SpaceWire IF
		cout << "Waiting for a new connection." << endl;
		openLoop: try {
			this->openSpaceWireIF();
		} catch (...) {
			cout << "Timeout " << endl;
			goto openLoop;
		}
		cout << "Connected." << endl;

		//start SpaceWireR Engine
		SpaceWireREngine* spwREngine = new SpaceWireREngine(spwif);
		spwREngine->start();
		cout << "SpaceWireREngine stated" << endl;

		//create receive TEP
		tep = new SpaceWireRReceiveTEP(spwREngine, channelID);
		tep->open();
		cout << "SpaceWireRReceiveTEP opened." << endl;

		receiveloop: try {
			while (!stopped) {
				std::vector<uint8_t>* data = tep->receive(1000);
				cout << "Received " << data->size() << " bytes." << endl;
			}
		} catch (SpaceWireRTEPException& e) {
			cout << "Receiver: " << e.toString() << endl;
			if (e.getStatus() == SpaceWireRTEPException::Timeout) {
				goto receiveloop;
			}
		}

		//finalize
		spwREngine->stop();
		spwREngine->waitUntilRunMethodComplets();
		delete spwREngine;
		spwif->close();
		delete spwif;
	}

	std::string toString() {
		if (tep != NULL) {
			return tep->toString();
		} else {
			return "";
		}
	}
};

class DumpThread: public CxxUtilities::StoppableThread {
private:
	ToStringInterface* toStringInterface;

public:
	DumpThread(ToStringInterface* toStringInterface) {
		this->toStringInterface = toStringInterface;
	}

public:
	void run() {
		using namespace std;
		CxxUtilities::Condition c;
		while (!stopped) {
			c.wait(1000);
			cout << toStringInterface->toString() << endl;
		}
	}
};

void showUsage() {
	using namespace std;
	cout << "test_SpaceWireR_sendReceive receivetep/transmittep client (ip address) (port)" << endl;
	cout << "test_SpaceWireR_sendReceive receivetep/transmittep server (port)" << endl;

}


int main(int argc, char* argv[]) {
	using namespace std;
	if (argc < 4) {
		showUsage();
		exit(-1);
	}

	std::string tepType(argv[1]);
	if (tepType != "receivetep" && tepType != "transmittep") {
		cerr << "the first argument should be either of receivetep/transmittep." << endl;
		exit(-1);
	}

	std::string clientOrServer(argv[2]);
	int port;
	std::string url;
	bool isSpaceWireIFOverTCPClientMode;
	if (clientOrServer == "client") {
		url = argv[3];
		port = atoi(argv[4]);
		isSpaceWireIFOverTCPClientMode = true;
	} else if (clientOrServer == "server") {
		port = atoi(argv[3]);
		isSpaceWireIFOverTCPClientMode = false;
	} else {
		cerr << "the second argument should be either of client/server." << endl;
	}

	if (tepType == "receivetep") {
		Receiver* r;
		if (isSpaceWireIFOverTCPClientMode) {
			r = new Receiver(channelID, url, port);
		} else {
			r = new Receiver(channelID, port);
		}
		r->start();
		DumpThread dumper(r);
		dumper.start();
		CxxUtilities::Condition c;
		c.wait();
	} else {
		Sender* s;
		if (isSpaceWireIFOverTCPClientMode) {
			s = new Sender(channelID, url, port);
		} else {
			s = new Sender(channelID, port);
		}
		s->start();
		DumpThread dumper(s);
		dumper.start();
		CxxUtilities::Condition c;
		c.wait();
	}
}
