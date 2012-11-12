/*
 * RouterConfigurationPort.hh
 *
 *  Created on: Nov 12, 2012
 *      Author: yuasa
 */

#ifndef ROUTERCONFIGURATIONPORT_HH_
#define ROUTERCONFIGURATIONPORT_HH_

#include "CxxUtilities/CxxUtilities.hh"

class RouterConfigurationPortException: public CxxUtilities::Exception {
public:
	RouterConfigurationPortException(int status) :
			CxxUtilities::Exception(status) {

	}

public:
	virtual ~RouterConfigurationPortException() {
	}

public:
	std::string toString() {
		std::string str;
		switch (status) {
		case NotImplemented:
			str = "NotImplemented";
			break;
		}
		return str;
	}
public:
	enum {
		NotImplemented
	};
};

class RouterConfigurationPort {
private:
	std::vector<uint8_t> additionalTargetSpaceWireAddress;
	std::vector<uint8_t> additionalReplyAddress;

public:
	virtual ~RouterConfigurationPort() {
	}

public:
	virtual size_t getTotalNumberOfPorts() = 0;

public:
	virtual size_t getNumberOfExternalPorts() = 0;

public:
	virtual size_t getNumberOfInternalPorts() = 0;

public:
	virtual std::string getConfigurationFileAsString() = 0;

public:
	virtual uint32_t getRoutingTableAddress(uint8_t logicalAddress) = 0;

public:
	virtual RMAPMemoryObject* getRoutingTableMemoryObject(uint8_t logicalAddress) = 0;

public:
	const std::vector<uint8_t>& getAdditionalReplyAddress() const {
		return additionalReplyAddress;
	}

public:
	void setAdditionalReplyAddress(const std::vector<uint8_t>& additionalReplyAddress) {
		this->additionalReplyAddress = additionalReplyAddress;
	}

public:
	const std::vector<uint8_t>& getAdditionalTargetSpaceWireAddress() const {
		return additionalTargetSpaceWireAddress;
	}

public:
	void setAdditionalTargetSpaceWireAddress(const std::vector<uint8_t>& additionalTargetSpaceWireAddress) {
		this->additionalTargetSpaceWireAddress = additionalTargetSpaceWireAddress;
	}
};

#endif /* ROUTERCONFIGURATIONPORT_HH_ */
