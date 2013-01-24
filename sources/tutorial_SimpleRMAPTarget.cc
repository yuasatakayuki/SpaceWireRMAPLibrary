/*
 * tutorial_RMAPTarget.cc
 *
 *  Created on: Jan 24, 2013
 *      Author: yuasa
 */

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWire.hh"
#include "RMAP.hh"

class SimpleRMAPTargetNode: public CxxUtilities::StoppableThread {
public:
	SpaceWireIF* spwif;
	static const int tcpPort = 10030;
public:
	SimpleRMAPTargetNode() {

	}

public:
	virtual ~SimpleRMAPTargetNode() {

	}

public:
	void initializeSpaceWireIF() {
		using namespace std;
		cout << "Waiting for a new connection." << endl;
		spwif = new SpaceWireIFOverTCP(tcpPort);
		spwif->open();
		cout << "Connected." << endl;
	}

public:
	void finalizeSpaceWireIF() {
		try {
			spwif->close();
		} catch (...) {

		}
		delete spwif;
	}

public:
	RMAPPacket* createReplyPacketFor(std::vector<uint8_t>* packet) {
		using namespace std;
		RMAPPacket rmapPacket;

		//try to interpret the received packet
		try {
			rmapPacket.interpretAsAnRMAPPacket(packet);
		} catch (RMAPPacketException& e) {
			cout << "The received packet is not an RMAP packet, and discarded." << endl;
			return NULL;
		}

		//if the received packet is an RMAP Command packet,
		//prepare an RMAP Reply packet
		if (!rmapPacket.isCommand()) {
			//if reply packet is received, this program discards it, and reply nothing.
			return NULL;

		}

		/* Add necessary parameter checks here.
		 * For example, Target Logical Address and Key may be checked id valid.
		 * These parameters can be retrieved from the received command packet via,
		 * rmapPacket.getTargetLogicalAddress() and rmapPacket.getKey().
		 * See RMAPPacket definition for details.
		 */

		//construct reply packet
		uint8_t replyStatus;
		std::vector<uint8_t> replyData;

		switch (rmapPacket.getAddress()) {
		case 0x00000000: //
			replyStatus = RMAPReplyStatus::CommandExcecutedSuccessfully;
			replyData.push_back(0x01);
			replyData.push_back(0x23);
			replyData.push_back(0x45);
			replyData.push_back(0x67);
			replyData.push_back(0x89);
			replyData.push_back(0xab);
			replyData.push_back(0xcd);
			replyData.push_back(0xef);
			break;
		case 0xaBadCafe: //
			replyStatus = RMAPReplyStatus::CommandExcecutedSuccessfully;
			replyData.push_back(0xAA);
			replyData.push_back(0x55);
			break;
		default: //
			replyStatus = RMAPReplyStatus::CommandNotImplementedOrNotAuthorized;
			break;
		}

		RMAPPacket* replyPacket = rmapPacket.constructReplyForCommand(&rmapPacket, replyStatus);
		if (replyStatus == RMAPReplyStatus::CommandExcecutedSuccessfully) {
			replyPacket->setData(replyData);
			replyPacket->setLength(replyData.size());
		}

		return replyPacket;
	}

public:
	void run() {
		using namespace std;
		while (!stopped) {
			initializeSpaceWireIF();

			//transaction loop
			while (!stopped) {
				std::vector<uint8_t>* packet;
				try {
					//try to receive SpaceWire packet
					packet = spwif->receive();

					//try to process the received packet as an RMAP Command packet
					RMAPPacket* replyPacket = createReplyPacketFor(packet);

					//if an RMAP Reply packet is prepared successfully, send it via the SpaceWire IF
					if (replyPacket != NULL) {
						spwif->send(replyPacket->getPacketBufferPointer());
						delete replyPacket;
					}
				} catch (SpaceWireIFException& e) {
					if (e.getStatus() == SpaceWireIFException::Timeout) {
						continue;
					} else {
						cout << "SpaceWire IF detected disconnection." << endl;
						goto finalize;
					}
				}
				delete packet;
			}
			finalize: //
			finalizeSpaceWireIF();
		}
	}
};

int main(int argc, char* argv[]) {
	SimpleRMAPTargetNode* simpleRMAPTargetNode = new SimpleRMAPTargetNode;
	simpleRMAPTargetNode->start();

	CxxUtilities::Condition c;
	c.wait();
}

