/*
 * tutorial_RMAPLayer.cc
 *
 *  Created on: Jan 10, 2012
 *      Author: yuasa
 */

#include "SpaceWire.hh"
#include "RMAP.hh"

double readTimeoutDuration = 1000;//1000ms
double writeTimeoutDuration = 1000;//1000ms

int main(int argc, char* argv[]) {
	using namespace std;

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

	/* Construct and start RMAP Engine */
	RMAPEngine* rmapEngine = new RMAPEngine(spwif);
	rmapEngine->start();

	/* Construct an RMAP Initiator instance */
	RMAPInitiator* rmapInitiator = new RMAPInitiator(rmapEngine);
	rmapInitiator->setInitiatorLogicalAddress(0xFE);

	/////////////////////////////////////////////////////////////////////////////////////
	/* Example 1 */
	/* Manually sets RMAPTargetNode information */
	cout << "Example 1" << endl;

	RMAPTargetNode* rmapTargetNode1 = new RMAPTargetNode();
	rmapTargetNode1->setTargetLogicalAddress(0xfe);
	rmapTargetNode1->setDefaultKey(0x20);
	std::vector<uint8_t> targetSpaceWireAddress;
	targetSpaceWireAddress.push_back(0x01);
	targetSpaceWireAddress.push_back(0x0a);
	targetSpaceWireAddress.push_back(0x05);
	rmapTargetNode1->setTargetSpaceWireAddress(targetSpaceWireAddress);
	std::vector<uint8_t> replyAddress;
	replyAddress.push_back(0x08);
	replyAddress.push_back(0x03);
	replyAddress.push_back(0x0f);
	rmapTargetNode1->setReplyAddress(replyAddress);
	cout << rmapTargetNode1->toString() << endl;

	/* RMAP Read/Write with address/length */
	try {
		//case 1-1 : read using C-array as a read buffer
		uint32_t readLength = 1024;
		uint8_t* readData = new uint8_t[(size_t) readLength];
		uint32_t readAddress = 0xFF801100;
		rmapInitiator->read(rmapTargetNode1, readAddress, readLength, readData, readTimeoutDuration);

		//case 1-2 : read using std::vector<uint8_t> as a read buffer
		std::vector<uint8_t> readDataVector(readLength);
		rmapInitiator->read(rmapTargetNode1, readAddress, readLength, &(readDataVector.at(0)), readTimeoutDuration);

		//case 1-3 : write using C-array write data
		uint32_t writeAddress = 0xFF803800;
		uint32_t writeLength = 4;
		uint8_t* writeData = new uint8_t[writeLength];
		writeData[0] = 0xAB;
		writeData[1] = 0xCD;
		writeData[2] = 0x12;
		writeData[3] = 0x34;
		rmapInitiator->write(rmapTargetNode1, writeAddress, writeData, writeLength, writeTimeoutDuration);

		delete readData;
		delete writeData;
		delete rmapTargetNode1;

		cout << "RMAP Read/Write Example1 done" << endl;
	} catch (RMAPInitiatorException e) {
		cerr << "RMAPInitiatorException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPReplyException e) {
		cerr << "RMAPReplyException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPEngineException e) {
		cerr << "RMAPEngineException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (...) {
		cerr << "Unkown error" << endl;
		exit(-1);
	}
	/////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////
	/* Example 2 */
	/* Use RMAPTargetNodes constructed from an XML-like configuration file. */
	cout << "Example 2" << endl;

	if (argc < 2) {
		cerr << "Example2 requires an XML-like configuration file." << endl;
		exit(-1);
	}

	//check file existence
	if (!CxxUtilities::File::exists(argv[1])) {
		cerr << "File " << argv[1] << " does not exist." << endl;
		exit(-1);
	}

	//construct RMAPTargetNodes from the XML file
	std::string filename(argv[1]);
	cout << "Constructing RMAPTargetNodes from " << filename << endl;
	RMAPTargetNodeDB* rmapTargetNodeDB;
	try {
		rmapTargetNodeDB = new RMAPTargetNodeDB(filename);
	} catch (RMAPTargetNodeDBException e) {
		cerr << "An exception thrown while loading the XML file " << filename << endl;
		cerr << e.toString() << endl;
		exit(-1);
	}

	//check the number of entries
	if (rmapTargetNodeDB->getSize() == 0) {
		cerr << "No RMAPTargetNode instance was constructed..." << endl;
		exit(-1);
	}

	//set the db to RMAPInitiator
	rmapInitiator->setRMAPTargetNodeDB(rmapTargetNodeDB);

	/* RMAP Read/Write with address/length */
	try {
		//case 1-1 : read using C-array as a read buffer
		uint32_t readLength = 2;
		uint8_t* readData = new uint8_t[(size_t) readLength];
		rmapInitiator->read("SpaceWireDigitalIOBoard", "LEDRegister", readData, readTimeoutDuration);

		//case 1-2 : read using std::vector<uint8_t> as a read buffer
		std::vector<uint8_t> readDataVector(readLength);
		rmapInitiator->read("SpaceWireDigitalIOBoard", "LEDRegister", &(readDataVector.at(0)), readTimeoutDuration);

		//case 1-3 : write using C-array write data
		uint32_t writeLength = 2;
		uint8_t* writeData = new uint8_t[writeLength];
		writeData[0] = 0xFF;
		writeData[1] = 0xFF;
		rmapInitiator->write("SpaceWireDigitalIOBoard", "LEDRegister", writeData, writeTimeoutDuration);

		delete readData;
		delete writeData;

		cout << "RMAP Read/Write Example2 done" << endl;
	} catch (RMAPInitiatorException e) {
		cerr << "RMAPInitiatorException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPReplyException e) {
		cerr << "RMAPReplyException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (RMAPEngineException e) {
		cerr << "RMAPEngineException " << e.toString() << endl;
		cerr << "Continue to next example" << endl;
	} catch (...) {
		cerr << "Unkown error" << endl;
		exit(-1);
	}
	/////////////////////////////////////////////////////////////////////////////////////

}
