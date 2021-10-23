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
 * RMAPTarget.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPTARGET_HH_
#define RMAPTARGET_HH_

#include "RMAPTransaction.hh"

class RMAPAddressRange {
public:
	uint32_t addressFrom;
	uint32_t addressTo;

public:
	uint32_t getAddressFrom() const {
		return addressFrom;
	}

	uint32_t getAddressTo() const {
		return addressTo;
	}

	void setAddressFrom(uint32_t addressFrom) {
		this->addressFrom = addressFrom;
	}

	void setAddressTo(uint32_t addressTo) {
		this->addressTo = addressTo;
	}

public:
	void setByLength(uint32_t addressFrom, uint32_t lengthInBytes) {
		this->addressFrom = addressFrom;
		this->addressTo = addressFrom + lengthInBytes;
	}

public:
	RMAPAddressRange(uint32_t addressFrom, uint32_t addressTo) :
			addressFrom(addressFrom), addressTo(addressTo) {

	}

	RMAPAddressRange() {
		addressFrom = 0;
		addressTo = 0;
	}

public:
	bool contains(RMAPAddressRange& addressRange) {
		if (addressRange.addressTo < addressFrom || addressTo < addressRange.addressFrom) {
			return false;
		}
		if (addressFrom <= addressRange.addressFrom && addressRange.addressTo <= addressTo) {
			return true;
		} else {
			return false;
		}
	}
};

class RMAPTargetAccessActionException: public CxxUtilities::Exception {
public:
	enum {
		AccessToUndefinedAddressRange
	};

public:
	RMAPTargetAccessActionException(int status) :
			CxxUtilities::Exception(status) {

	}

	virtual ~RMAPTargetAccessActionException() {
	}
};

class RMAPTargetAccessAction {
public:
	virtual ~RMAPTargetAccessAction() {
	}

public:
	virtual void processTransaction(RMAPTransaction* rmapTransaction) throw (RMAPTargetAccessActionException)= 0;

	virtual void transactionWillComplete(RMAPTransaction* rmapTransaction) throw (RMAPTargetAccessActionException) {
		delete rmapTransaction->replyPacket;
	}

	virtual void transactionReplyCouldNotBeSent(RMAPTransaction* rmapTransaction)
			throw (RMAPTargetAccessActionException) {
		delete rmapTransaction->replyPacket;
	}

public:
	void setReplyWithDataWithStatus(RMAPTransaction* rmapTransaction, std::vector<uint8_t>* data, uint8_t status) {
		rmapTransaction->replyPacket = RMAPPacket::constructReplyForCommand(rmapTransaction->commandPacket, status);
		rmapTransaction->replyPacket->setData(*data);
	}

	void setReplyWithStatus(RMAPTransaction* rmapTransaction, uint8_t status) {
		rmapTransaction->replyPacket = RMAPPacket::constructReplyForCommand(rmapTransaction->commandPacket, status);
		rmapTransaction->replyPacket->clearData();
	}

};

class RMAPTargetException: public CxxUtilities::Exception {
public:
	enum {
		AccessToUndefinedAddressRange
	};

public:
	RMAPTargetException(int status) :
			CxxUtilities::Exception(status) {
	}

	virtual ~RMAPTargetException() {
	}

};

class RMAPTarget {
private:
	std::map<RMAPAddressRange*, RMAPTargetAccessAction*> actions;
	std::vector<RMAPAddressRange*> addressRanges;

public:
	RMAPTarget() {

	}

	~RMAPTarget() {

	}

public:
	void addAddressRangeAndAssociatedAction(RMAPAddressRange* addressRange, RMAPTargetAccessAction* action) {
		actions[addressRange] = action;
		addressRanges.push_back(addressRange);
	}

	bool doesAcceptAddressRange(RMAPAddressRange addressRange) {
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return true;
			}
		}
		return false;
	}

	bool doesAcceptTransaction(RMAPTransaction* rmapTransaction) {
		uint32_t addressFrom = rmapTransaction->commandPacket->getAddress();
		uint32_t addressTo = addressFrom + rmapTransaction->commandPacket->getLength();
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return true;
			}
		}
		return false;
	}

	void processTransaction(RMAPTransaction* rmapTransaction) throw (RMAPTargetException) {
		uint32_t addressFrom = rmapTransaction->commandPacket->getAddress();
		uint32_t addressTo = addressFrom + rmapTransaction->commandPacket->getLength();
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				try {
					actions[addressRanges[i]]->processTransaction(rmapTransaction);
					return;
				} catch (...) {
					throw RMAPTargetException(RMAPTargetException::AccessToUndefinedAddressRange);
				}
			}
		}
		throw RMAPTargetException(RMAPTargetException::AccessToUndefinedAddressRange);
	}

	/** Can return NULL. */
	RMAPTargetAccessAction* getCorrespondingRMAPTargetAccessAction(RMAPTransaction* rmapTransaction) {
		using namespace std;
		uint32_t addressFrom = rmapTransaction->commandPacket->getAddress();
		uint32_t addressTo = addressFrom + rmapTransaction->commandPacket->getLength() - 1;
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return actions[addressRanges[i]];
			}
		}
		return NULL;
	}

};

#endif /* RMAPTARGET_HH_ */
