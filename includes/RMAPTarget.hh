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
};

class RMAPTargetAccessAction {
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

	}

	void setReplyWithStatus(RMAPTransaction* rmapTransaction, std::vector<uint8_t>* data, uint8_t status) {

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
		cout << "%1 " << "0x" << hex << right << setw(4) << setfill('0') << (uint32_t) addressFrom << " " << "0x"
				<< hex << right << setw(2) << setfill('0') << (uint32_t) addressTo << endl;
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
