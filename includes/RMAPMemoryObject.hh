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
private:
	std::string errorFilename;
	bool isErrorFilenameSet_;

public:
	enum {
		InvalidXMLEntry, FileNotFound
	};

public:
	RMAPMemoryObjectException(int status) :
		CxxUtilities::Exception(status) {
		isErrorFilenameSet_ = false;
		using namespace std;
	}

	RMAPMemoryObjectException(int status, std::string errorFilename) :
		CxxUtilities::Exception(status) {
		setErrorFilename(errorFilename);
	}

	void setErrorFilename(std::string errorFilename) {
		this->errorFilename = errorFilename;
		isErrorFilenameSet_ = true;
	}

	std::string getErrorFilename() {
		return errorFilename;
	}

	bool isErrorFilenameSet() {
		return isErrorFilenameSet_;
	}
};

class RMAPMemoryObject {
public:
	enum {
		NotAccessible = 0x0000, Readable = 0x0001, Writable = 0x0010, RMWable = 0x0100
	};
	enum {
		Increment = 0x01, NonIncrement = 0x00
	};
private:
	std::string id;
	uint32_t extendedAddress;
	uint32_t address;
	uint32_t length;
	uint32_t accessMode;
	uint8_t key;
	uint32_t increment;
	//** add flags for verifyCapable, maxVerifiableBytes, replyCapable, and so on. */

private:
	bool isAccessModeSet_;
	bool isKeySet_;
	bool isIncrementModeSet_;

public:
	RMAPMemoryObject() {
		accessMode = Readable | Writable | RMWable;
		isAccessModeSet_ = false;
		isKeySet_ = false;
		isIncrementModeSet_ = false;
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

	uint32_t getAddress() const {
		return address;
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

	void setAddress(uint32_t memoryAddress) {
		this->address = memoryAddress;
	}

	std::string getID() const {
		return id;
	}

	void setID(std::string id) {
		this->id = id;
	}

	uint32_t getIncrementMode() {
		return increment;
	}

	bool isIncrementMode() {
		if (increment == Increment) {
			return true;
		} else {
			return false;
		}
	}

#ifndef NO_XMLLODER
public:
	static std::vector<RMAPMemoryObject*> constructFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException, RMAPMemoryObjectException) {
		using namespace std;
		ifstream ifs(filename.c_str());
		if(!ifs.is_open()) {
			throw RMAPMemoryObjectException(RMAPMemoryObjectException::FileNotFound);
		}
		ifs.close();
		XMLNode *topnode;
		XMLLoader::XMLLoader(&topnode, filename.c_str());
		std::vector<RMAPMemoryObject*> result;
		try {
			result=constructFromXMLFile(topnode);
		} catch(...) {
			throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry,filename);
		}
		return result;
	}

	static std::vector<RMAPMemoryObject*> constructFromXMLFile(XMLNode* topNode) throw (XMLLoader::XMLLoaderException, RMAPMemoryObjectException) {
		using namespace std;
		vector<XMLNode*> nodes = topNode->getChildren("RMAPMemoryObject");
		vector<RMAPMemoryObject*> result;
		for (unsigned int i = 0; i < nodes.size(); i++) {
			try {
				result.push_back(RMAPMemoryObject::constructFromXMLNode(nodes[i]));
			} catch(RMAPMemoryObjectException e) {
				cerr << "##2 error in RMAPMemoryObject" << endl;
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
			} catch(...) {
				using namespace std;
				cerr << "#RMAPMemoryObject::constructFromXMLFile() error in the file" << endl;
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
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

		const char* tagNames[] = {"ExtendedAddress", "Address", "Length"};
		const size_t nTagNames=3;
		for (size_t i = 0; i < nTagNames; i++) {
			if (node->getChild(string(tagNames[i])) == NULL) {
				//not all of necessary tags are defined in the node configration part
				using namespace std;
				cerr << "## error in RMAPMemoryObject" << endl;
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
			}
		}

		RMAPMemoryObject* memoryObject=new RMAPMemoryObject;
		memoryObject->setID(node->getAttribute("id"));
		memoryObject->setAddress(String::toUInt32(node->getChild("Address")->getValue()));
		memoryObject->setLength(String::toUInt32(node->getChild("Length")->getValue()));

		//optional
		if (node->getChild("ExtendedAddress") != NULL) {
			memoryObject->setExtendedAddress(String::toInteger(node->getChild("ExtendedAddress")->getValue()));
		}
		if (node->getChild("AccessMode") != NULL) {
			memoryObject->setAccessMode(node->getChild("AccessMode")->getValue());
		}
		if (node->getChild("Key") != NULL) {
			memoryObject->setKey(String::toInteger(node->getChild("Key")->getValue()));
		}
		if (node->getChild("IncrementMode") != NULL) {
			memoryObject->setIncrementMode(node->getChild("IncrementMode")->getValue());
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
			if (mode.find("read") != string::npos) {
				accessMode = accessMode | Readable;
			}
			if (mode.find("writable") != string::npos) {
				accessMode = accessMode | Writable;
			}
			if (mode.find("write") != string::npos) {
				accessMode = accessMode | Writable;
			}
			if (mode.find("rmwable") != string::npos) {
				accessMode = accessMode | RMWable;
			}
		}
	}

	void setIncrementMode(std::string mode) {
		isIncrementModeSet_ = true;
		using namespace std;
		using namespace CxxUtilities;
		mode = String::toLowerCase(mode);
		if (mode.find("noincrement") != string::npos || mode.find("nonincrement") != string::npos || mode.find(
				"no increment") != string::npos || mode.find("non increment") != string::npos) {
			increment = NonIncrement;
		} else if (mode.find("increment") != string::npos) {
			increment = Increment;
		}
	}

	bool isReadable() {
		if ((accessMode & Readable) == 0) {
			return false;
		} else {
			return true;
		}
	}

	bool isWritable() {
		if ((accessMode & Writable) == 0) {
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

	bool isIncrementModeSet() {
		return isIncrementModeSet_;
	}

	std::string toString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "RMAPMemoryObject" << endl;
		ss << "ID              : " << id << endl;
		ss << "ExtendedAddress : " << hex << right << setw(2) << setfill('0') << (uint32_t) extendedAddress << endl;
		ss << "MemoryAddress   : " << hex << right << setw(8) << setfill('0') << (uint32_t) address << endl;
		ss << "Length          : " << hex << right << setw(2) << setfill('0') << (uint32_t) length << endl;
		if (isKeySet()) {
			ss << "Key             : " << hex << right << setw(2) << setfill('0') << (uint32_t) key << endl;
		}
		if (isAccessModeSet()) {
			ss << "AccessMode      : " << toStringAccessMode() << endl;
		}
		if (isIncrementModeSet()) {
			ss << "	IncrementMode  : " << toStringIncrementMode() << endl;
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

	std::string toXMLString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "<RMAPMemoryObject id=\"" << id << "\">" << endl;
		ss << "	<ExtendedAddress>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) extendedAddress
				<< "</ExtendedAddress>" << endl;
		ss << "	<MemoryAddress>" << "0x" << hex << right << setw(8) << setfill('0') << (uint32_t) address
				<< "</MemoryAddress>" << endl;
		ss << "	<Length>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) length << "</Length>"
				<< endl;
		if (isKeySet()) {
			ss << "	<Key>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) key << "</Key>" << endl;
		}
		if (isAccessModeSet()) {
			ss << "	<AccessMode>" << toStringAccessMode() << "</AccessMode>" << endl;
		}
		if (isIncrementModeSet()) {
			ss << "	<IncrementMode>" << toStringIncrementMode() << "</IncrementMode>" << endl;
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
		if ((accessMode & Readable) != 0) {
			ss << "Readable";
		}
		if ((accessMode & Writable) != 0) {
			ss << "Writable";
		}
		if ((accessMode & RMWable) != 0) {
			ss << "RMWable";
		}
		return ss.str();
	}

	std::string toStringIncrementMode() {
		using namespace std;
		stringstream ss;
		if (isIncrementMode()) {
			ss << "Increment";
		} else {
			ss << "NonIncrement";
		}
		return ss.str();
	}
};

#endif /* RMAPMEMORYOBJECT_HH_ */
