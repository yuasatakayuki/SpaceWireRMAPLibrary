/*
 * RMAPTargetNode.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPTARGETNODE_HH_
#define RMAPTARGETNODE_HH_

#include <CxxUtilities/CommonHeader.hh>
#include "SpaceWireUtilities.hh"
#ifndef NO_XMLLODER
#include "XMLLoader.hpp"
#endif

class RMAPTargetNode {
private:
	std::string id;
	std::vector<uint8_t> targetSpaceWireAddress;
	std::vector<uint8_t> replyAddress;
	uint8_t targetLogicalAddress;
	uint8_t initiatorLogicalAddress;
	uint8_t defaultKey;

	bool isInitiatorLogicalAddressSet_;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;
	static const uint8_t DefaultKey = 0x20;

public:
	RMAPTargetNode() {
		targetLogicalAddress = 0xFE;
		initiatorLogicalAddress=0xFE;
		defaultKey = DefaultKey;
		isInitiatorLogicalAddressSet_ = false;
	}

#ifndef NO_XMLLODER
public:
	static std::vector<RMAPTargetNode*> constructFromXMLFile(XMLNode* topNode) throw (XMLLoader::XMLLoaderException) {
		using namespace std;
		vector<XMLNode*> nodes = topNode->getChildren("RMAPTargetNode");
		vector<RMAPTargetNode*> result;
		for (unsigned int i = 0; i < nodes.size(); i++) {
			result.push_back(RMAPTargetNode::constructFromXMLNode(nodes[i]));
		}
		return result;
	}

	static RMAPTargetNode* constructFromXMLNode(XMLNode* node) throw (XMLLoader::XMLLoaderException) {
		using namespace std;
		using namespace CxxUtilities;

		const char* tagNames[] = { "TargetLogicalAddress", "TargetSpaceWireAddress", "ReplyAddress", "Key" };
		for (size_t i = 0; i < 4; i++) {
			if (node->getChild(string(tagNames[i])) == NULL) {
				//not all of necessary tags are defined in the node configration part
				return NULL;
			}
		}

		RMAPTargetNode* targetNode = new RMAPTargetNode();
		targetNode->setID(node->getAttribute("id"));
		targetNode->setTargetLogicalAddress(String::toInteger(node->getChild("TargetLogicalAddress")->getValue()));
		vector<unsigned char> targetSpaceWireAddress = String::toUnsignedCharArray(node->getChild(
				"TargetSpaceWireAddress")->getValue());
		targetNode->setTargetSpaceWireAddress(targetSpaceWireAddress);
		vector<unsigned char> replyAddress = String::toUnsignedCharArray(node->getChild("ReplyAddress")->getValue());
		targetNode->setReplyAddress(replyAddress);
		targetNode->setDefaultKey(String::toInteger(node->getChild("Key")->getValue()));

		//optional
		if (node->getChild("InitiatorLogicalAddress") != NULL) {
			using namespace std;
			cout << "##InitiatorLogicalAddress is found" << endl;
			targetNode->setInitiatorLogicalAddress(node->getChild("InitiatorLogicalAddress")->getValueAsUInt8());
		}

		return targetNode;
	}
#endif

public:
	uint8_t getDefaultKey() const {
		return defaultKey;
	}

	std::vector<uint8_t> getReplyAddress() const {
		return replyAddress;
	}

	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

	std::vector<uint8_t> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

	void setDefaultKey(uint8_t defaultKey) {
		this->defaultKey = defaultKey;
	}

	void setReplyAddress(std::vector<uint8_t>& replyAddress) {
		this->replyAddress = replyAddress;
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTargetSpaceWireAddress(std::vector<uint8_t>& targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

	std::string getID() const {
		return id;
	}

	void setID(std::string id) {
		this->id = id;
	}

	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->isInitiatorLogicalAddressSet_ = true;
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

	void unsetInitiatorLogicalAddress() {
		this->isInitiatorLogicalAddressSet_ = false;
		this->initiatorLogicalAddress = 0xFE;
	}

	bool isInitiatorLogicalAddressSet() {
		return isInitiatorLogicalAddressSet_;
	}

	uint8_t getInitiatorLogicalAddress() {
		return initiatorLogicalAddress;
	}

	std::string toString() {
		using namespace std;
		stringstream ss;
		ss << "Initiator Logical Address : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t)initiatorLogicalAddress
				<< endl;
		ss << "Target Logical Address    : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t)targetLogicalAddress
				<< endl;
		ss << "Target SpaceWire Address  : " << SpaceWireUtilities::packetToString(&targetSpaceWireAddress) << endl;
		ss << "Reply Address             : " << SpaceWireUtilities::packetToString(&replyAddress) << endl;
		ss << "Default Key               : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t)defaultKey << endl;
		return ss.str();
	}
};

#endif /* RMAPTARGETNODE_HH_ */
