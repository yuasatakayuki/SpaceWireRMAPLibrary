/*
 * test_RMAPMemoryObject_fromXMLFile.cc
 *
 *  Created on: Dec 5, 2011
 *      Author: yuasa
 */

#include "RMAPMemoryObject.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	if (argc != 2) {
		cout << "test_RMAPMemoryObject_fromXMLFile (xmlfile)" << endl;
		exit(-1);
	}

	vector<RMAPMemoryObject*> memoryObjects = RMAPMemoryObject::constructFromXMLFile(argv[1]);
	for (size_t i = 0; i < memoryObjects.size(); i++) {
		cout << memoryObjects[i]->toString() << endl;
	}
	cout << "------------" << endl;
	for (size_t i = 0; i < memoryObjects.size(); i++) {
		cout << memoryObjects[i]->toXMLString() << endl;
	}

}
