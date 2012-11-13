/*
 * main_RMAP_readWriteRMAPTargetNode.cc
 *
 *  Created on: Dec 15, 2011
 *      Author: yuasa
 */

#include "RMAPTargetNode.hh"
#include "RMAPInitiator.hh"
#include "RMAPEngine.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireIFOverTCP.hh"
#include "CxxUtilities/CxxUtilities.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;

	if (argc < 6) {
		cerr << "usage : RMAP_readWriteRMAPTargetNode arg1 arg2 arg3 arg4 arg5...(optional)" << endl;
		cerr << "argv1 = IP Address of SpaceWire-to-GigabitEther" << endl;
		cerr << "argv2 = xml filename" << endl;
		cerr << "argv3 = RMAP Target Node ID" << endl;
		cerr << "argv4 = Memory Object ID" << endl;
		cerr << "argv5 = read/write" << endl;
		cerr << "argv6... = data (when write) e.g. 0x0a 0x12 0xbd 0x44" << endl;
		cerr << endl;
		exit(-1);
	}

	//initialize parameter
	string ipAddress(argv[1]);
	string xmlfilename(argv[2]);
	string rmapTargetNodeID(argv[3]);
	string memoryObjectID(argv[4]);
	string instruction(argv[5]);
	vector<uint8_t> data;
	for (int i = 6; i < argc; i++) {
		data.push_back(String::toUInt8(string(argv[i])));
	}
	if (instruction != "read" && instruction != "write") {
		cerr << "instruction should be either of read or write" << endl;
		exit(-1);
	}

	//load xml file
	cout << "Loading " << xmlfilename << endl;
	RMAPTargetNodeDB db;
	try {
		db.loadRMAPTargetNodesFromXMLFile(xmlfilename);
	} catch (...) {
		cerr << "Error in loading " << xmlfilename << endl;
		exit(-1);
	}

	//find RMAPTargetNode and MemoryObject
	RMAPTargetNode* targetNode;
	try {
		targetNode = db.getRMAPTargetNode(rmapTargetNodeID);
	} catch (...) {
		cerr << "It seems RMAPTargetNode named " << rmapTargetNodeID << " is not defined in " << xmlfilename << endl;
		exit(-1);
	}
	cout << "RMAPTargetNode named " << rmapTargetNodeID << " was found." << endl;
	RMAPMemoryObject* memoryObject;
	try {
		memoryObject = targetNode->getMemoryObject(memoryObjectID);
	} catch (...) {
		cerr << "It seems Mmeory Object named " << memoryObjectID << " of " << rmapTargetNodeID
				<< " is not defined in " << xmlfilename << endl;
		exit(-1);
	}
	cout << "Memory Object named " << memoryObjectID << " was found." << endl;

	//check size when write
	if (instruction == "write" && data.size() != memoryObject->getLength()) {
		cerr << "The size of provided data (" << data.size() << "bytes) does not match the size of the memory object ("
				<< memoryObject->getLength() << ")." << endl;
		exit(-1);
	}

	//SpaceWire part
	cout << "Opening SpaceWireIF...";
	SpaceWireIFOverTCP* spwif = new SpaceWireIFOverTCP(ipAddress, 10030);
	try {
		spwif->open();
	} catch (...) {
		cerr << "Connection timed out." << endl;
		exit(-1);
	}
	cout << "done" << endl;
	Condition spwifWait;
	spwifWait.wait(100);

	//RMAPEngine/RMAPInitiator part
	RMAPEngine* rmapEngine = new RMAPEngine(spwif);
	rmapEngine->start();
	RMAPInitiator* rmapInitiator = new RMAPInitiator(rmapEngine);
	rmapInitiator->setInitiatorLogicalAddress(0xFE);

	//perform read/write
	cout << "Performing " << instruction << " instruction." << endl;
	if (instruction == "read") {//do read
		uint8_t* buffer = new uint8_t(memoryObject->getLength());
		try {
			rmapInitiator->read(targetNode, memoryObjectID, buffer, 300000.0);
			cout << SpaceWireUtilities::packetToString(buffer, memoryObject->getLength()) << endl;
			rmapInitiator->read(targetNode, memoryObjectID, buffer, 300000.0);
			cout << SpaceWireUtilities::packetToString(buffer, memoryObject->getLength()) << endl;
			rmapInitiator->read(targetNode, memoryObjectID, buffer, 300000.0);
			cout << SpaceWireUtilities::packetToString(buffer, memoryObject->getLength()) << endl;
			rmapInitiator->read(targetNode, memoryObjectID, buffer, 300000.0);
			cout << SpaceWireUtilities::packetToString(buffer, memoryObject->getLength()) << endl;
		} catch (RMAPInitiatorException& e) {
			if (e.getStatus() == RMAPInitiatorException::Timeout) {
				cerr << "Timeout." << endl;
			} else {
				cerr << "Exception in RMAPInitiator::read()." << endl;
			}
			goto finalize;
		} catch (RMAPReplyException& e) {
			cout << "Exception : " << e.toString() << endl;
		}
		cout << "Read successfully done." << endl;
		cout << SpaceWireUtilities::packetToString(buffer, memoryObject->getLength()) << endl;
		for(int i=0;i<8;i++){
		cout << "0x" << hex << right << setw(2) << setfill('0')  << (uint32_t)buffer[i] << endl;
		}
		delete buffer;
	} else {//do write
		try {
			if (data.size() != 0) {
				rmapInitiator->write(targetNode, memoryObjectID, (uint8_t*) &(data[0]), (uint32_t) data.size());
			} else {
				rmapInitiator->write(targetNode, memoryObjectID, (uint8_t*) NULL, (uint32_t) 0);
			}
		} catch (RMAPInitiatorException& e) {
			if (e.getStatus() == RMAPInitiatorException::Timeout) {
				cerr << "Timeout." << endl;
			} else {
				cerr << "Exception in RMAPInitiator::read()." << endl;
			}
			goto finalize;
		} catch (RMAPReplyException& e) {
			cout << "Exception : " << e.toString() << endl;
		}
		cout << "Write successfully done." << endl;
	}

	finalize: rmapEngine->stop();
	delete rmapEngine;
	spwif->close();
	delete spwif;

}
