/*
 * test_RMAPTarget.cc
 *
 *  Created on: Nov 14, 2011
 *      Author: yuasa
 */

#include "RMAPTargetNode.hh"

using namespace std;
int main(int argc, char* argv[]) {
	RMAPTargetNode node;
	node.setTargetLogicalAddress(0xfe);
	node.setInitiatorLogicalAddress(0x32);
	node.setDefaultKey(0x20);

	cout << node.toString() << endl;

}
