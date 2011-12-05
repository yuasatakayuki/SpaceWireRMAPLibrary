/*
 * RMAPMemoryObject.hh
 *
 *  Created on: Dec 5, 2011
 *      Author: yuasa
 */

#ifndef RMAPMEMORYOBJECT_HH_
#define RMAPMEMORYOBJECT_HH_

#include <CxxUtilities/CommonHeader.hh>
#include <CxxUtilities/Exception.hh>
#include <CxxUtilities/String.hh>
#ifndef NO_XMLLODER
#include "XMLLoader.hpp"
#endif

class RMAPMemoryObjectException: public CxxUtilities::Exception {
public:
	enum {
		InvalidXMLEntry, FileNotFound
	};

public:
	RMAPMemoryObjectException(int status) :
		CxxUtilities::Exception(status) {

	}
};

class RMAPMemoryObject {
public:
	enum {
		NotAccessible = 0x0000, Readable = 0x0001, Writable = 0x0010, RMWable = 0x0100
	};
private:
	std::string id;
	uint32_t extendedAddress;
	uint32_t memoryAddress;
	uint32_t length;
	uint32_t accessMode;
	uint8_t key;
	//** add flags for verifyCapable, maxVerifiableBytes, replyCapable, and so on. */

private:
	bool isAccessModeSet_;
	bool isKeySet_;

public:
	RMAPMemoryObject() {
		accessMode = Readable | Writable | RMWable;
		isAccessModeSet_ = false;
		isKeySet_ = false;
	}
public:
	uint32_t getAccessMode() const {
		return accessMode;
	}

	uint32_t getExtendedAddress() const {
		return extendedAddress;
	}

	uint8_t getKey() const {
		return key;
	}

	uint32_t getLength() const {
		return length;
	}

	uint32_t getMemoryAddress() const {
		return memoryAddress;
	}

	void setAccessMode(uint32_t accessMode) {
		isAccessModeSet_ = true;
		this->accessMode = accessMode;
	}

	void setExtendedAddress(uint32_t extendedAddress) {
		this->extendedAddress = extendedAddress;
	}

	void setKey(uint8_t key) {
		this->key = key;
	}

	void setLength(uint32_t length) {
		this->length = length;
	}

	void setMemoryAddress(uint32_t memoryAddress) {
		this->memoryAddress = memoryAddress;
	}

	std::string getID() const {
		return id;
	}

	void setID(std::string id) {
		this->id = id;
	}
#ifndef NO_XMLLODER
public:
	static std::vector<RMAPMemoryObject*> constructFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException) {
		using namespace std;
		ifstream ifs(filename.c_str());
		if(!ifs.is_open()) {
			throw RMAPMemoryObjectException(RMAPMemoryObjectException::FileNotFound);
		}
		ifs.close();
		XMLLoader xmlLoader(filename.c_str());
		return constructFromXMLFile(xmlLoader.getTopNode());
	}

	static std::vector<RMAPMemoryObject*> constructFromXMLFile(XMLNode* topNode) throw (XMLLoader::XMLLoaderException) {
		using namespace std;
		vector<XMLNode*> nodes = topNode->getChildren("RMAPMemoryObject");
		vector<RMAPMemoryObject*> result;
		for (unsigned int i = 0; i < nodes.size(); i++) {
			try {
				result.push_back(RMAPMemoryObject::constructFromXMLNode(nodes[i]));
			} catch(...) {
				using namespace std;
				cerr << "#RMAPMemoryObject::constructFromXMLFile() error in the file" << endl;
			}
		}
		return result;
	}

	static RMAPMemoryObject* constructFromXMLNode(XMLNode* node) throw (XMLLoader::XMLLoaderException, RMAPMemoryObjectException) {
		using namespace std;
		using namespace CxxUtilities;

		/*
		 <RMAPMemoryObject id="EssentialHK">
		 <ExtendedAddress>0x00</ExtendedAddress>
		 <MemoryAddress>0xFF801100</MemoryAddress>
		 <Length>0x08</Length>
		 <Key>0x20</Key>
		 </RMAPMemoryObject>
		 */

		const char* tagNames[] = {"ExtendedAddress", "MemoryAddress", "Length"};
		const size_t nTagNames=3;
		for (size_t i = 0; i < nTagNames; i++) {
			if (node->getChild(string(tagNames[i])) == NULL) {
				//not all of necessary tags are defined in the node configration part
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
			}
		}

		RMAPMemoryObject* memoryObject=new RMAPMemoryObject;
		memoryObject->setID(node->getAttribute("id"));
		memoryObject->setExtendedAddress(String::toInteger(node->getChild("ExtendedAddress")->getValue()));
		memoryObject->setMemoryAddress(String::toUInt32(node->getChild("MemoryAddress")->getValue()));
		memoryObject->setLength(String::toUInt32(node->getChild("Length")->getValue()));

		//optional
		if (node->getChild("AccessMode") != NULL) {
			memoryObject->setAccessMode(String::toInteger(node->getChild("AccessMode")->getValue()));
		}
		if (node->getChild("Key") != NULL) {
			memoryObject->setKey(String::toInteger(node->getChild("Key")->getValue()));
		}
		return memoryObject;
	}
#endif

public:
	void setAccessMode(std::string mode) {
		isAccessModeSet_ = true;
		using namespace std;
		using namespace CxxUtilities;
		mode = String::toLowerCase(mode);
		if (mode.find("ro") != string::npos) {
			accessMode = Readable;
		} else if (mode.find("wo") != string::npos) {
			accessMode = Writable;
		} else if (mode.find("rw") != string::npos) {
			accessMode = Readable | Writable;
		} else if (mode.find("readwrite") != string::npos) {
			accessMode = Readable | Writable;
		} else if (mode.find("readonly") != string::npos) {
			accessMode = Readable;
		} else if (mode.find("writeonly") != string::npos) {
			accessMode = Writable;
		} else {
			accessMode = NotAccessible;
			if (mode.find("readable") != string::npos) {
				accessMode = accessMode | Readable;
			}
			if (mode.find("writable") != string::npos) {
				accessMode = accessMode | Writable;
			}
			if (mode.find("RMWable") != string::npos) {
				accessMode = accessMode | RMWable;
			}
		}
	}

	bool isReadable() {
		if (accessMode & Readable == 0) {
			return false;
		} else {
			return true;
		}
	}

	bool isWritable() {
		if (accessMode & Writable == 0) {
			return false;
		} else {
			return true;
		}
	}

	bool isAccessModeSet() {
		return isAccessModeSet_;
	}

	bool isKeySet() {
		return isKeySet_;
	}

	std::string toString() {
		using namespace std;
		stringstream ss;
		ss << "RMAPMemoryObject" << endl;
		ss << "ID              : " << id << endl;
		ss << "ExtendedAddress : " << hex << right << setw(2) << setfill('0') << (uint32_t) extendedAddress << endl;
		ss << "MemoryAddress   : " << hex << right << setw(8) << setfill('0') << (uint32_t) memoryAddress << endl;
		ss << "Length          : " << hex << right << setw(2) << setfill('0') << (uint32_t) length << endl;
		if (isKeySet()) {
			ss << "Key             : " << hex << right << setw(2) << setfill('0') << (uint32_t) key << endl;
		}
		if (isAccessModeSet()) {
			ss << "AccessMode      : " << toStringAccessMode() << endl;
		}
		return ss.str();
	}

	std::string toXMLString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "<RMAPMemoryObject id=\"" << id << "\">" << endl;
		ss << "	<ExtendedAddress>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) extendedAddress
				<< "</ExtendedAddress>" << endl;
		ss << "	<MemoryAddress>" << "0x" << hex << right << setw(8) << setfill('0') << (uint32_t) memoryAddress
				<< "</MemoryAddress>" << endl;
		ss << "	<Length>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) length << "</Length>"
				<< endl;
		if (isKeySet()) {
			ss << "	<Key>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) key << "</Key>" << endl;
		}
		if (isAccessModeSet()) {
			ss << "	<AccessMode>" << toStringAccessMode() << "</AccessMode>" << endl;
		}
		ss << "</RMAPMemoryObject>";

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

private:
	std::string toStringAccessMode() {
		using namespace std;
		stringstream ss;
		if (accessMode & Readable != 0) {
			ss << "Readable";
		}
		if (accessMode & Writable != 0) {
			ss << "Writable";
		}
		if (accessMode & RMWable != 0) {
			ss << "RMWable";
		}
		return ss.str();
	}
};

#endif /* RMAPMEMORYOBJECT_HH_ */
