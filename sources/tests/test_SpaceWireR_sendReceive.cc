/*
 * test_SpaceWireR_sendReceive.cc
 *
 *  Created on: Dec 10, 2012
 *      Author: yuasa
 */

#include "SpaceWireR.hh"
#include "CxxUtilities/CxxUtilities.hh"

class ToStringInterface {
public:
	virtual std::string toString() =0;
};

class Sender: public CxxUtilities::StoppableThread, public ToStringInterface {
private:
	uint8_t channelID;

public:
	SpaceWireRTransmitTEP* tep;

public:
	Sender(uint8_t channelID = 0x00) {
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
		SpaceWireIFOverTCP* spwif = new SpaceWireIFOverTCP("localhost", 10030);
		spwif->open();
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
		for (size_t i = 0; i < 1024; i++) {
			data.push_back(i);
		}

		sendloop: try {
			while (!stopped) {
				std::vector<uint8_t> data;
				for (size_t i = 0; i < 1024; i++) {
					data.push_back((uint8_t)i);
				}
				tep->send(&data);
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

class Receiver: public CxxUtilities::StoppableThread, public ToStringInterface {
private:
	uint8_t channelID;

public:
	SpaceWireRReceiveTEP* tep;

public:
	Receiver(uint8_t channelID = 0x00) {
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
		SpaceWireIFOverTCP* spwif = new SpaceWireIFOverTCP(10030);
		openLoop: try {
			spwif->open();
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

int main(int argc, char* argv[]) {
	if (argc != 1) {
		Receiver r(0x03);
		r.start();
		DumpThread dumper(&r);
		dumper.start();
		CxxUtilities::Condition c;
		c.wait();
	} else {
		Sender s(0x03);
		s.start();
		DumpThread dumper(&s);
		dumper.start();
		CxxUtilities::Condition c;
		c.wait();
	}
}
