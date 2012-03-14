/*
 * SpaceWireIF.hh
 *
 *  Created on: Aug 3, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIF_HH_
#define SPACEWIREIF_HH_

#include <CxxUtilities/CommonHeader.hh>
#include <CxxUtilities/Exception.hh>
#include "SpaceWireEOPMarker.hh"

class SpaceWireIFException: public CxxUtilities::Exception {
public:
	enum {
		OpeningConnectionFailed,
		Disconnected,
		Timeout,
		EEP,
		ReceiveBufferTooSmall,
		FunctionNotImplemented,
		LinkIsNotOpened
	};
public:
	SpaceWireIFException(uint32_t status) :
			CxxUtilities::Exception(status) {
	}
public:
	std::string toString() {
		std::string result;
		switch (status) {
		case OpeningConnectionFailed:
			result = "OpeningConnectionFailed";
			break;
		case Disconnected:
			result = "Disconnected";
			break;
		case Timeout:
			result = "Timeout";
			break;
		case EEP:
			result = "EEP";
			break;
		case ReceiveBufferTooSmall:
			result = "ReceiveBufferTooSmall";
			break;
		case FunctionNotImplemented:
			result = "FunctionNotImplemented";
			break;
		case LinkIsNotOpened:
			result = "LinkIsNotOpened";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

class SpaceWireIF;

/** An abstract super class for SpaceWireIF-related actions.
 */
class SpaceWireIFAction {
};

/** An abstract class which includes a method invoked when
 * Timecode is received.
 */
class SpaceWireIFActionTimecodeScynchronizedAction: public SpaceWireIFAction {
public:
	/** Performs action.
	 * @param[in] timecodeValue time code value
	 */
	virtual void doAction(unsigned char timecodeValue) = 0;
};

/** An abstract class which includes a method invoked when
 * a SpaceWire interface is closed.
 */
class SpaceWireIFActionCloseAction: public SpaceWireIFAction {
public:
	/** Performs action.
	 * @param[in] spwif parent SpaceWireIF instance
	 */
	virtual void doAction(SpaceWireIF* spwif) = 0;
};

/** An abstract class for encapsulation of a SpaceWire interface.
 *  This class provides virtual methods for opening/closing the interface
 *  and sending/receiving a packet via the interface.
 *  Physical or Virtual SpaceWire interface class should be created
 *  by inheriting this class.
 */
class SpaceWireIF {
private:
	std::vector<SpaceWireIFActionTimecodeScynchronizedAction*> timecodeSynchronizedActions;
	std::vector<SpaceWireIFActionCloseAction*> spacewireIFCloseActions;
	bool isTerminatedWithEEP_;
	bool isTerminatedWithEOP_;

public:
	enum OpenCloseState {
		Closed, Opened
	};

protected:
	enum OpenCloseState state;

protected:
	bool eepShouldBeReportedAsAnException_; //default false (no exception)

public:
	enum EOPType {
		EOP = 0x00, EEP = 0x01, Undefined = 0xffff
	};

public:
	SpaceWireIF() {
		state = Closed;
		isTerminatedWithEEP_ = false;
		isTerminatedWithEOP_ = false;
		eepShouldBeReportedAsAnException_ = false;
	}

public:
	enum OpenCloseState getState() {
		return state;
	}

public:
	virtual void open() throw (SpaceWireIFException) =0;

	virtual void close() throw (SpaceWireIFException) {
		invokeSpaceWireIFCloseActions();
	}

public:
	virtual void
	send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) =0;

	virtual void sendVectorReference(std::vector<uint8_t>& data, SpaceWireEOPMarker::EOPType eopType =
			SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
		if (data.size() == 0) {
			return;
		} else {
			send(&(data[0]), data.size(), eopType);
		}
	}

	virtual void send(std::vector<uint8_t>& data, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		if (data.size() == 0) {
			return;
		} else {
			send(&(data[0]), data.size(), eopType);
		}
	}

	virtual void send(std::vector<uint8_t>* data, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		if (data->size() == 0) {
			return;
		} else {
			send(&(data->at(0)), data->size(), eopType);
		}
	}

	virtual void sendVectorPointer(std::vector<uint8_t>* data, SpaceWireEOPMarker::EOPType eopType =
			SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
		if (data->size() == 0) {
			return;
		} else {
			send(&(data->at(0)), data->size(), eopType);
		}
	}

public:
	virtual void receive(uint8_t* buffer, SpaceWireEOPMarker::EOPType& eopType, size_t maxLength, size_t& length)
			throw (SpaceWireIFException) {
		std::vector<uint8_t>* packet = this->receive();
		size_t packetSize = packet->size();
		if (packetSize == 0) {
			length = 0;
		} else {
			length = packetSize;
			if (packetSize <= maxLength) {
				memcpy(buffer, &(packet->at(0)), packetSize);
			} else {
				memcpy(buffer, &(packet->at(0)), maxLength);
				delete packet;
				throw SpaceWireIFException(SpaceWireIFException::ReceiveBufferTooSmall);
			}
		}
		delete packet;
	}

	virtual std::vector<uint8_t>* receive() throw (SpaceWireIFException) {
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>();
		try {
			this->receive(buffer);
			return buffer;
		} catch (SpaceWireIFException e) {
			delete buffer;
			throw e;
		}
	}

	virtual void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) =0;

public:
	virtual void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) =0;

public:
	virtual void setTxLinkRate(uint32_t linkRateType) throw (SpaceWireIFException) =0;

	virtual uint32_t getTxLinkRateType() throw (SpaceWireIFException) =0;

private:
	uint32_t txLinkRateType;
	enum LinkRates {
		LinkRate200MHz = 200000,
		LinkRate125MHz = 125000,
		LinkRate100MHz = 100000,
		LinkRate50MHz = 50000,
		LinkRate25MHz = 25000,
		LinkRate12_5MHz = 12500,
		LinkRate10MHz = 10000,
		LinkRate5MHz = 5000,
		LinkRate2MHz = 2000
	};

public:
	virtual void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) =0;

	double getTimeoutDurationInMicroSec() {
		return timeoutDurationInMicroSec;
	}

protected:
	//if timeoutDurationInMicroSec==0, timeout is disabled.
	double timeoutDurationInMicroSec; //us

public:
	/** A method sets (adds) an action against getting Timecode.
	 */
	void addTimecodeAction(SpaceWireIFActionTimecodeScynchronizedAction* action) {
		for (size_t i = 0; i < timecodeSynchronizedActions.size(); i++) {
			if (action == timecodeSynchronizedActions[i]) {
				return; //already registered
			}
		}
		timecodeSynchronizedActions.push_back(action);
	}

	void registerTimecodeAction(SpaceWireIFActionTimecodeScynchronizedAction* action) {
		addTimecodeAction(action);
	}

	void deleteTimecodeAction(SpaceWireIFActionTimecodeScynchronizedAction* action) {
		std::vector<SpaceWireIFActionTimecodeScynchronizedAction*> newActions;
		for (size_t i = 0; i < timecodeSynchronizedActions.size(); i++) {
			if (action != timecodeSynchronizedActions[i]) {
				newActions.push_back(timecodeSynchronizedActions[i]);
			}
		}
		timecodeSynchronizedActions = newActions;
	}

	void clearTimecodeSynchronizedActions() {
		timecodeSynchronizedActions.clear();
	}

	void invokeTimecodeSynchronizedActions(uint8_t tickOutValue) {
		for (size_t i = 0; i < timecodeSynchronizedActions.size(); i++) {
			timecodeSynchronizedActions[i]->doAction(tickOutValue);
		}
	}

public:
	void addSpaceWireIFCloseAction(SpaceWireIFActionCloseAction* spacewireIFCloseAction) {
		for (size_t i = 0; i < spacewireIFCloseActions.size(); i++) {
			if (spacewireIFCloseAction == spacewireIFCloseActions[i]) {
				return; //already registered
			}
		}
		spacewireIFCloseActions.push_back(spacewireIFCloseAction);
	}

	void deleteSpaceWireIFCloseAction(SpaceWireIFActionCloseAction* spacewireIFCloseAction) {
		std::vector<SpaceWireIFActionCloseAction*> newActions;
		for (size_t i = 0; i < spacewireIFCloseActions.size(); i++) {
			if (spacewireIFCloseAction != spacewireIFCloseActions[i]) {
				newActions.push_back(spacewireIFCloseActions[i]);
			}
		}
		spacewireIFCloseActions = newActions;
	}

	void clearSpaceWireIFCloseActions() {
		spacewireIFCloseActions.clear();
	}

	void invokeSpaceWireIFCloseActions() {
		for (size_t i = 0; i < spacewireIFCloseActions.size(); i++) {
			spacewireIFCloseActions[i]->doAction(this);
		}
	}

public:
	bool isTerminatedWithEEP() {
		return isTerminatedWithEEP_;
	}

	bool isTerminatedWithEOP() {
		return isTerminatedWithEOP_;
	}

	void setReceivedPacketEOPMarkerType(int eopType) {
		if (eopType == SpaceWireIF::EEP) {
			isTerminatedWithEEP_ = true;
			isTerminatedWithEEP_ = false;
		} else if (eopType == SpaceWireIF::EOP) {
			isTerminatedWithEEP_ = false;
			isTerminatedWithEEP_ = true;
		} else {
			isTerminatedWithEEP_ = false;
			isTerminatedWithEEP_ = false;
		}
	}

	int getReceivedPacketEOPMarkerType() {
		if (isTerminatedWithEEP_ == false && isTerminatedWithEEP_ == true) {
			return EOP;
		} else if (isTerminatedWithEEP_ == true && isTerminatedWithEEP_ == false) {
			return EEP;
		} else {
			return Undefined;
		}
	}

	void eepShouldBeReportedAsAnException() {
		eepShouldBeReportedAsAnException_ = true;
	}

	void eepShouldNotBeReportedAsAnException() {
		eepShouldBeReportedAsAnException_ = false;
	}

};
#endif /* SPACEWIREIF_HH_ */
