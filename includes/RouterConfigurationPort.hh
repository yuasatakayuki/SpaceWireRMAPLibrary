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
 * RouterConfigurationPort.hh
 *
 *  Created on: Nov 12, 2012
 *      Author: yuasa
 */

#ifndef ROUTERCONFIGURATIONPORT_HH_
#define ROUTERCONFIGURATIONPORT_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "RMAP.hh"

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
		case OperationFailed:
			str = "OperationFailed";
			break;
		case InvalidPortNumber:
			str = "InvalidPortNumber";
			break;
		case InvalidLinkFrequency:
			str = "InvalidLinkFrequency";
			break;
		case RMAPInitiatorIsNotAvailable:
			str = "RMAPInitiatorIsNotAvailable";
			break;
		}
		return str;
	}
public:
	enum {
		NotImplemented, //
		OperationFailed, //
		InvalidPortNumber, //
		InvalidLinkFrequency, //
		RMAPInitiatorIsNotAvailable
	};
};

class RouterConfigurationPort {

protected:
	RMAPInitiator* rmapInitiator;

public:
	virtual ~RouterConfigurationPort() {
	}

public:
	virtual RMAPTargetNode* getRMAPTargetNodeInstance() = 0;

public:
	virtual size_t getTotalNumberOfPorts() = 0;

public:
	virtual size_t getNumberOfExternalPorts() = 0;

public:
	virtual size_t getNumberOfInternalPorts() = 0;

public:
	virtual uint32_t getRoutingTableAddress(uint8_t logicalAddress) throw (RouterConfigurationPortException) = 0;

public:
	virtual RMAPMemoryObject* getRoutingTableMemoryObject(uint8_t logicalAddress) throw (RouterConfigurationPortException) = 0;

public:
	virtual uint32_t getLinkFrequencyRegisterAddress(uint8_t port) throw (RouterConfigurationPortException) = 0;

public:
	virtual RMAPMemoryObject* getLinkFrequencyRegisterMemoryObject(uint8_t port) throw (RouterConfigurationPortException) = 0;

public:
	virtual std::vector<double> getAvailableLinkFrequencies(uint8_t port) = 0;

public:
	virtual void setLinkFrequency(uint8_t port, double linkFrequency) throw (RouterConfigurationPortException,
			RMAPInitiatorException) = 0;

public:
	virtual void setLinkEnable(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual void unsetLinkEnable(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual bool isLinkEnabled(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual void setLinkStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual void unsetLinkStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual bool isLinkStarted(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual void setAutoStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual void unsetAutoStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	virtual bool isAutoStarted(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) = 0;

public:
	bool isLinkFrequencyValid(uint8_t port, double linkFrequency) {
		std::vector<double> linkFrequencies = getAvailableLinkFrequencies(port);
		for (size_t i = 0; i < linkFrequencies.size(); i++) {
			if (linkFrequencies[i] == linkFrequency) {
				return true;
			}
		}
		return false;
	}

public:
	virtual void applyNewPathAddresses(std::vector<uint8_t> targetSpaceWireAddress, std::vector<uint8_t> replyAddress) =0;

public:
	RMAPInitiator* getRMAPInitiator() const {
		return rmapInitiator;
	}

public:
	void setRMAPInitiator(RMAPInitiator* rmapInitiator) {
		this->rmapInitiator = rmapInitiator;
	}
};

#endif /* ROUTERCONFIGURATIONPORT_HH_ */
