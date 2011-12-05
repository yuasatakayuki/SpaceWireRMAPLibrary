/*
 * test_RMAPTargetNode_fromXMLFile.cc
 *
 *  Created on: Dec 5, 2011
 *      Author: yuasa
 */

#include "RMAPTargetNode.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	if (argc != 2) {
		cout << "test_RMAPTargetNode_fromXMLFile (xmlfile)" << endl;
		exit(-1);
	}

	vector<RMAPTargetNode*> targetNodes = RMAPTargetNode::constructFromXMLFile(argv[1]);
	for (size_t i = 0; i < targetNodes.size(); i++) {
		cout << targetNodes[i]->toString() << endl;
	}
	cout << "--------------" << endl;
	for (size_t i = 0; i < targetNodes.size(); i++) {
		cout << targetNodes[i]->toXMLString() << endl;
	}
}
