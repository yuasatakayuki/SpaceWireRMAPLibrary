/*
 * test_RMAPInitiator_RMAPEngine.cc
 *
 *  Created on: Oct 21, 2011
 *      Author: yuasa
 */

#include "RMAPInitiator.hh"
#include "RMAPEngine.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireIFOverIPClient.hh"
#include "XMLLoader.hpp"

int main(int argc, char* argv[]) {
	using namespace std;

	string filename = "/Users/yuasa/Documents/workspace/SpaceWireRMAPLibrary/sources/node.xml";

	cout << "Opening SpaceWireIF...";
	SpaceWireIFOverIPClient* spwif = new SpaceWireIFOverIPClient("192.168.1.44", 10030);
	try {
		spwif->open();
	} catch (...) {
		cerr << "Connection timed out." << endl;
		exit(-1);
	}
	cout << "done" << endl;

	RMAPEngine* rmapEngine = new RMAPEngine(spwif);
	rmapEngine->start();
	RMAPInitiator* rmapInitiator = new RMAPInitiator(rmapEngine);
	rmapInitiator->setInitiatorLogicalAddress(0xFE);

	cout << "Constructing RMAPTargetNodes from " << filename << endl;
	XMLLoader xmlLoader(filename.c_str());
	vector<RMAPTargetNode*> rmapTargetNodes = RMAPTargetNode::constructFromXMLFile(xmlLoader.getTopNode());
	RMAPTargetNode* rmapTargetNode;
	if(rmapTargetNodes.size()==0){
		cerr << "No RMAPTargetNode instance was constructed..." << endl;
		exit(-1);
	}else{
		rmapTargetNode=rmapTargetNodes[0];
		cout << "RMAPTargetNode : " << endl;
		cout << "Target Logical Address   : 0x" << hex << setw(2) << setfill('0') << (uint32_t)rmapTargetNode->getTargetLogicalAddress() << endl;
		cout << "Target SpaceWire Address : ";
		for(size_t i=0;i<rmapTargetNode->getTargetSpaceWireAddress().size();i++){
			cout << "0x" << setw(2) << setfill('0') << (uint32_t)(rmapTargetNode->getTargetSpaceWireAddress()[i]) << " ";
		}
		cout << endl;
		cout << "Reply Address            : ";
		for(size_t i=0;i<rmapTargetNode->getReplyAddress().size();i++){
			cout << "0x" << setw(2) << setfill('0') << (uint32_t)(rmapTargetNode->getReplyAddress()[i]) << " ";
		}
		cout << endl;
		cout << dec;
	}
	//

	uint8_t registerValue[]={1,2,3,4};
	cout << "RMAP Read...";
	try {
		rmapInitiator->read(rmapTargetNode, 0x0012, 4, registerValue);
	} catch (...) {
		//timeout
		cout << "Timeout" << endl;
		exit(-1);
	}
	cout << "Done" << endl;

	cout << "Register value:";
	for (size_t i = 0; i < 4; i++) {
		cout << hex << setw(2) << setfill('0') << (uint32_t)registerValue[i];
	}
	cout << endl;

}
