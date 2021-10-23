/* 
============================================================================
SpaceWire/RMAP Library is provided under the MIT License.
============================================================================

Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
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
#include "RMAPMemoryObject.hh"
#include "RMAPNode.hh"
#ifndef NO_XMLLODER
#include "XMLUtilities/XMLUtilities.hh"
#endif

class RMAPTargetNodeException: public CxxUtilities::Exception {
private:
	std::string errorFilename;
	bool isErrorFilenameSet_;

public:
	enum {
		FileNotFound, InvalidXMLEntry, NoSuchRMAPMemoryObject
	};

public:
	RMAPTargetNodeException(int status) :
			CxxUtilities::Exception(status) {
		isErrorFilenameSet_ = false;
	}

public:
	RMAPTargetNodeException(int status, std::string errorFilename) :
			CxxUtilities::Exception(status) {
		setErrorFilename(errorFilename);
	}

public:
	virtual ~RMAPTargetNodeException() {
	}

public:
	void setErrorFilename(std::string errorFilename) {
		this->errorFilename = errorFilename;
		isErrorFilenameSet_ = true;
	}

public:
	std::string getErrorFilename() {
		return errorFilename;
	}

public:
	bool isErrorFilenameSet() {
		return isErrorFilenameSet_;
	}

};

class RMAPTargetNode: public RMAPNode {
private:
	std::vector<uint8_t> targetSpaceWireAddress;
	std::vector<uint8_t> replyAddress;
	uint8_t targetLogicalAddress;
	uint8_t initiatorLogicalAddress;
	uint8_t defaultKey;

	bool isInitiatorLogicalAddressSet_;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;
	static const uint8_t DefaultKey = 0x20;

private:
	std::map<std::string, RMAPMemoryObject*> memoryObjects;

public:
	RMAPTargetNode() {
		targetLogicalAddress = 0xFE;
		initiatorLogicalAddress = 0xFE;
		defaultKey = DefaultKey;
		isInitiatorLogicalAddressSet_ = false;
	}

#ifndef NO_XMLLODER
public:
	static std::vector<RMAPTargetNode*> constructFromXMLString(std::string str) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		std::vector<RMAPTargetNode*> result;
		XMLNode *topNode;
		XMLLoader xmlLoader;
		try {
			xmlLoader.loadFromString(&topNode, str);
			if (topNode != NULL) {
				result = constructFromXMLFile(topNode);
				return result;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
			}
		} catch (RMAPTargetNodeException& e) {
			if (e.isErrorFilenameSet()) {
				throw e;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
			}
		} catch (...) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
		}
		return result;
	}

public:
	static std::vector<RMAPTargetNode*> constructFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		using namespace std;
		ifstream ifs(filename.c_str());
		if (!ifs.is_open()) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::FileNotFound);
		}
		ifs.close();
		XMLNode *topNode;
		XMLLoader(&topNode, filename.c_str());
		std::vector<RMAPTargetNode*> result;
		try {
			if (topNode != NULL) {
				result = constructFromXMLFile(topNode);
				return result;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry, filename);
			}
		} catch (RMAPTargetNodeException& e) {
			if (e.isErrorFilenameSet()) {
				throw e;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry, filename);
			}
		} catch (...) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry, filename);
		}
	}

public:
	static std::vector<RMAPTargetNode*> constructFromXMLFile(XMLNode* topNode) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		using namespace std;
		if (topNode == NULL) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
		}
		vector<XMLNode*> nodes = topNode->getChildren("RMAPTargetNode");
		vector<RMAPTargetNode*> result;
		for (unsigned int i = 0; i < nodes.size(); i++) {
			if (nodes[i] != NULL) {
				try {
					result.push_back(RMAPTargetNode::constructFromXMLNode(nodes[i]));
				} catch (RMAPMemoryObjectException& e) {
					throw e;
				} catch (RMAPTargetNodeException& e) {
					if (e.isErrorFilenameSet()) {
						throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry, e.getErrorFilename());
					} else {
						throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
					}
				} catch (...) {
					throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
				}
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
			}
		}
		return result;
	}

public:
	static RMAPTargetNode* constructFromXMLNode(XMLNode* node) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		using namespace std;
		using namespace CxxUtilities;

		const char* tagNames[] = { "TargetLogicalAddress", "TargetSpaceWireAddress", "ReplyAddress", "Key" };
		const size_t nTagNames = 4;
		for (size_t i = 0; i < nTagNames; i++) {
			if (node->getChild(string(tagNames[i])) == NULL) {
				//not all of necessary tags are defined in the node configration part
				return NULL;
			}
		}

		RMAPTargetNode* targetNode = new RMAPTargetNode();
		targetNode->setID(node->getAttribute("id"));
		targetNode->setTargetLogicalAddress(String::toInteger(node->getChild("TargetLogicalAddress")->getValue()));
		vector<unsigned char> targetSpaceWireAddress = String::toUnsignedCharArray(
				node->getChild("TargetSpaceWireAddress")->getValue());
		targetNode->setTargetSpaceWireAddress(targetSpaceWireAddress);
		vector<unsigned char> replyAddress = String::toUnsignedCharArray(node->getChild("ReplyAddress")->getValue());
		targetNode->setReplyAddress(replyAddress);
		targetNode->setDefaultKey(String::toInteger(node->getChild("Key")->getValue()));

		//optional
		if (node->getChild("InitiatorLogicalAddress") != NULL) {
			using namespace std;
			targetNode->setInitiatorLogicalAddress(node->getChild("InitiatorLogicalAddress")->getValueAsUInt8());
		}
		constructRMAPMemoryObjectFromXMLFile(node, targetNode);

		return targetNode;
	}

private:
	static void constructRMAPMemoryObjectFromXMLFile(XMLNode* node, RMAPTargetNode* targetNode)
			throw (XMLLoader::XMLLoaderException, RMAPTargetNodeException, RMAPMemoryObjectException) {
		using namespace std;
		vector<XMLNode*> nodes = node->getChildren("RMAPMemoryObject");
		for (unsigned int i = 0; i < nodes.size(); i++) {
			if (nodes[i] != NULL) {
				try {
					targetNode->addMemoryObject(RMAPMemoryObject::constructFromXMLNode(nodes[i]));
				} catch (RMAPMemoryObjectException& e) {
					throw e;
				} catch (...) {
					throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
				}
			} else {
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
			}
		}
	}

#endif

public:
	void pushLeadingTargetSpaceWireAddress(uint8_t spaceWireAddress) {
		std::vector<uint8_t> newList;
		newList.push_back(spaceWireAddress);
		for (size_t i = 0; i < targetSpaceWireAddress.size(); i++) {
			newList.push_back(targetSpaceWireAddress.at(i));
		}
		targetSpaceWireAddress = newList;
	}

public:
	void pushTrailingTargetSpaceWireAddress(uint8_t spaceWireAddress) {
		std::vector<uint8_t> newList;
		for (size_t i = 0; i < targetSpaceWireAddress.size(); i++) {
			newList.push_back(targetSpaceWireAddress.at(i));
		}
		newList.push_back(spaceWireAddress);
		targetSpaceWireAddress = newList;
	}

public:
	void pushLeadingReplyAddress(uint8_t spaceWireAddress) {
		std::vector<uint8_t> newList;
		newList.push_back(spaceWireAddress);
		for (size_t i = 0; i < replyAddress.size(); i++) {
			newList.push_back(replyAddress.at(i));
		}
		replyAddress = newList;
	}

public:
	void pushTrailingReplyAddress(uint8_t spaceWireAddress) {
		std::vector<uint8_t> newList;
		for (size_t i = 0; i < replyAddress.size(); i++) {
			newList.push_back(replyAddress.at(i));
		}
		newList.push_back(spaceWireAddress);
		replyAddress = newList;
	}

public:
	uint8_t getDefaultKey() const {
		return defaultKey;
	}

public:
	std::vector<uint8_t> getReplyAddress() const {
		return replyAddress;
	}

public:
	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

public:
	std::vector<uint8_t> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

public:
	void setDefaultKey(uint8_t defaultKey) {
		this->defaultKey = defaultKey;
	}

public:
	void setReplyAddress(std::vector<uint8_t> replyAddress) {
		this->replyAddress = replyAddress;
	}

public:
	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

public:
	void setTargetSpaceWireAddress(std::vector<uint8_t> targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

public:
	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->isInitiatorLogicalAddressSet_ = true;
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

public:
	void unsetInitiatorLogicalAddress() {
		this->isInitiatorLogicalAddressSet_ = false;
		this->initiatorLogicalAddress = 0xFE;
	}

public:
	bool isInitiatorLogicalAddressSet() {
		return isInitiatorLogicalAddressSet_;
	}

public:
	uint8_t getInitiatorLogicalAddress() {
		return initiatorLogicalAddress;
	}

public:
	void addMemoryObject(RMAPMemoryObject* memoryObject) {
		memoryObjects[memoryObject->getID()] = memoryObject;
	}

public:
	std::map<std::string, RMAPMemoryObject*>* getMemoryObjects() {
		return &memoryObjects;
	}

public:
	/** This method does not return NULL even when not found, but throws an exception. */
	RMAPMemoryObject* getMemoryObject(std::string memoryObjectID) throw (RMAPTargetNodeException) {
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.find(memoryObjectID);
		if (it != memoryObjects.end()) {
			return it->second;
		} else {
			throw RMAPTargetNodeException(RMAPTargetNodeException::NoSuchRMAPMemoryObject);
		}
	}

public:
	/** This method can return NULL when not found.*/
	RMAPMemoryObject* findMemoryObject(std::string memoryObjectID) throw (RMAPTargetNodeException) {
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.find(memoryObjectID);
		if (it != memoryObjects.end()) {
			return it->second;
		} else {
			return NULL;
		}
	}

public:
	std::map<std::string, RMAPMemoryObject*> getAllMemoryObjects(){
		return memoryObjects;
	}

public:
	std::string toString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "ID                        : " << this->id << endl;
		if (isInitiatorLogicalAddressSet()) {
			ss << "Initiator Logical Address : 0x" << right << hex << setw(2) << setfill('0')
					<< (uint32_t) initiatorLogicalAddress << endl;
		}
		ss << "Target Logical Address    : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t) targetLogicalAddress
				<< endl;
		ss << "Target SpaceWire Address  : " << SpaceWireUtilities::packetToString(&targetSpaceWireAddress) << endl;
		ss << "Reply Address             : " << SpaceWireUtilities::packetToString(&replyAddress) << endl;
		ss << "Default Key               : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t) defaultKey << endl;
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.begin();
		for (; it != memoryObjects.end(); it++) {
			ss << it->second->toString(nTabs + 1);
		}
		stringstream ss2;
		while (!ss.eof()) {
			string line;
			getline(ss, line);
			for (int i = 0; i < nTabs; i++) {
				ss2 << "	";
			}
			ss2 << line << endl;
		}
		return ss2.str();
	}

public:
	std::string toXMLString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "<RMAPTargetNode>" << endl;
		if (isInitiatorLogicalAddressSet()) {
			ss << "	<InitiatorLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
					<< (uint32_t) initiatorLogicalAddress << "</InitiatorLogicalAddress>" << endl;
		}
		ss << "	<TargetLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) targetLogicalAddress << "</TargetLogicalAddress>" << endl;
		ss << "	<TargetSpaceWireAddress>" << SpaceWireUtilities::packetToString(&targetSpaceWireAddress)
				<< "</TargetSpaceWireAddress>" << endl;
		ss << "	<ReplyAddress>" << SpaceWireUtilities::packetToString(&replyAddress) << "</ReplyAddress>" << endl;
		ss << "	<DefaultKey>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) defaultKey << "</DefaultKey>"
				<< endl;
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.begin();
		for (; it != memoryObjects.end(); it++) {
			ss << it->second->toXMLString(nTabs + 1);
		}
		ss << "</RMAPTargetNode>";

		stringstream ss2;
		while (!ss.eof()) {
			string line;
			getline(ss, line);
			for (int i = 0; i < nTabs; i++) {
				ss2 << "	";
			}
			ss2 << line << endl;
		}
		return ss2.str();
	}
};

class RMAPTargetNodeDBException: public CxxUtilities::Exception {
public:
	enum {
		NoSuchRMAPTargetNode
	};

public:
	RMAPTargetNodeDBException(int status) :
			CxxUtilities::Exception(status) {

	}

public:
	std::string toString() {
		std::string result;
		switch (status) {
		case NoSuchRMAPTargetNode:
			result = "NoSuchRMAPTargetNode";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

class RMAPTargetNodeDB {
private:
	std::map<std::string, RMAPTargetNode*> db; //targetNodeID-RMAPTargetNodeInstance

public:
	RMAPTargetNodeDB() {

	}

public:
	RMAPTargetNodeDB(std::vector<RMAPTargetNode*> rmapTargetNodes) {
		addRMAPTargetNodes(rmapTargetNodes);
	}

public:
	RMAPTargetNodeDB(std::string filename) {
		try {
			addRMAPTargetNodes(RMAPTargetNode::constructFromXMLFile(filename));
		} catch (...) {
			//empty DB will be constructed if the specified xml file contains any error.
		}
	}

public:
	~RMAPTargetNodeDB() {
		//deletion of RMAPTargetNode* will be done outside this class.
	}

public:
	void addRMAPTargetNode(RMAPTargetNode* rmapTargetNode) {
		db[rmapTargetNode->getID()] = rmapTargetNode;
	}

public:
	void addRMAPTargetNodes(std::vector<RMAPTargetNode*> rmapTargetNodes) {
		for (size_t i = 0; i < rmapTargetNodes.size(); i++) {
			addRMAPTargetNode(rmapTargetNodes[i]);
		}
	}

public:
	void loadRMAPTargetNodesFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		addRMAPTargetNodes(RMAPTargetNode::constructFromXMLFile(filename));
	}

public:
	size_t getSize() {
		return db.size();
	}

public:
	/** This method does not return NULL when not found, but throws an exception.
	 *
	 */
	RMAPTargetNode* getRMAPTargetNode(std::string id) throw (RMAPTargetNodeDBException) {
		std::map<std::string, RMAPTargetNode*>::iterator it = db.find(id);
		if (it != db.end()) {
			return it->second;
		} else {
			throw RMAPTargetNodeDBException(RMAPTargetNodeDBException::NoSuchRMAPTargetNode);
		}
	}

public:
	/** This method can return NULL when an RMAPTargetNode with a specified ID is not found.
	 */
	RMAPTargetNode* findRMAPTargetNode(std::string id) {
		std::map<std::string, RMAPTargetNode*>::iterator it = db.find(id);
		if (it != db.end()) {
			return it->second;
		} else {
			return NULL;
		}
	}

public:
	/** This method can return NULL when an RMAPTargetNode with a specified logical address is not found.
	 */
	RMAPTargetNode* findRMAPTargetNode(uint8_t logicalAddress) {
		std::map<std::string, RMAPTargetNode*>::iterator it = db.begin();
		for (; it != db.end(); it++) {
			if (it->second->getTargetLogicalAddress() == logicalAddress) {
				return it->second;
			}
		}
		return NULL;
	}

public:
	std::vector<RMAPTargetNode*> getAllRMAPTargetNodes() {
		std::vector<RMAPTargetNode*> targetNodes;
		std::map<std::string, RMAPTargetNode*>::iterator it = db.begin();
		while (it != db.end()) {
			targetNodes.push_back(it->second);
			it++;
		}
		return targetNodes;
	}

public:
	std::string toString(){
		using namespace std;
		std::stringstream ss;
		std::map<std::string, RMAPTargetNode*>::iterator it=db.begin();
		while(it!=db.end()){
			RMAPTargetNode* rmapTargetNode=it->second;
			std::vector<uint8_t> targetSpaceWireAddress=rmapTargetNode->getTargetSpaceWireAddress();
			std::vector<uint8_t> replyAddress=rmapTargetNode->getReplyAddress();
			ss << rmapTargetNode->getID() //
					<<
					" Logical Address = " << "0x" << hex << right << setw(2) << setfill('0')  << (uint32_t)rmapTargetNode->getTargetLogicalAddress() //
					<< "Target SpaceWire Address = [" << CxxUtilities::Array<uint8_t >::toString(targetSpaceWireAddress,"hex",128) << "]"//
					<< "Reply Address = [" << CxxUtilities::Array<uint8_t >::toString(replyAddress,"hex",128) << "]"//
					<< endl;
			it++;
		}
		return ss.str();
	}
};

#endif /* RMAPTARGETNODE_HH_ */
