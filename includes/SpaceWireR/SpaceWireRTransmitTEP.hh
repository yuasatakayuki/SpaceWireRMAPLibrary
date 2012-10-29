/*
 * SpaceWireRTransmitTEP.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERTRANSMITTEP_HH_
#define SPACEWIRERTRANSMITTEP_HH_

#include "SpaceWireR/SpaceWireREngine.hh"
#include "SpaceWireR/SpaceWireRPacket.hh"
#include "CxxUtilities/Exception.hh"

class SpaceWireRTransmitTEPException: public CxxUtilities::Exception {
public:
	SpaceWireRTransmitTEPException(int status) :
			CxxUtilities::Exception(status) {
	}

public:
	virtual ~SpaceWireRTransmitTEPException() {
	}

public:
	enum {
		OpenFailed, IlleagalOpenDirectionWhileClosingASocket,
	};
};

class SpaceWireRTEPType {
public:
	enum {
		TransmitTEP, //
		ReceiveTEP
	};
};

class SpaceWireRTEPState {
public:
	enum {
		Closed, Enabled, Open, Closing
	};
};

class SpaceWireRTEP {
private:
	bool openCommandReceived;
	uint32_t tepType;

private:
	//routing information
	std::vector<uint8_t> destinationSpaceWireAddress;
	std::vector<uint8_t> sourceSpaceWireAddress;
	uint8_t destinationLogicalAddress;
	uint8_t sourceLogicalAddress;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;

protected:
	SpaceWireRTEP(uint32_t tepType) {
		this->tepType = tepType;
		this->openCommandReceived = false;
		this->sourceLogicalAddress = DefaultLogicalAddress;
		this->destinationLogicalAddress = DefaultLogicalAddress;
	}

public:
	virtual ~SpaceWireRTEP() {
	}

public:
	bool isTransmitTEP() {
		if (tepType == SpaceWireRTEPType::TransmitTEP) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isReceiveTEP() {
		if (tepType == SpaceWireRTEPType::ReceiveTEP) {
			return true;
		} else {
			return false;
		}
	}

protected:
	void sendOpenCommandPacket() {
		if (isTransmitTEP()) {

		}
	}

protected:
	void sendOpenCommandAckPacket() {

	}

protected:
	void sendCloseCommandPacket() {

	}

protected:
	void sendCloseCommandAckPacket() {

	}

protected:
	void sendKeepAlivePacket() {

	}

protected:
	void sendKeepAliveAckPacket() {

	}

protected:
	void sendFlowControlPacket() {

	}

public:
	void open() = 0;

public:
	void close() = 0;

public:
	virtual void openCommandPacketHasBeenReceived() = 0;

public:
	virtual void openCommandAckPacketHasBeenReceived() = 0;

public:
	virtual void closeCommandPacketHasBeenReceived() = 0;

public:
	virtual void closeCommandAckPacketHasBeenReceived() = 0;

public:
	virtual void keepAlivePacketHasBeenReceived() = 0;

public:
	virtual void keepAliveAckPacketHasBeenReceived() = 0;

public:
	virtual void flowControlPacketHasBeenReceived() = 0;

};

class SpaceWireRTransmitTEP: public SpaceWireRTEP {

private:
	uint32_t state;
	SpaceWireREngine* spwrEngine;

public:
	static const double DefaultTimeoutDurationInMilliSec = 1000; //ms

public:
	SpaceWireRTransmitTEP(SpaceWireREngine* spwrEngine) {
		this->spwrEngine = spwrEngine;
		this->state = Closed;
	}

public:
	void open(double timeoutDrationInMilliSec = DefaultTimeoutDurationInMilliSec) throw (SpaceWireRTransmitTEPException) {
		if (state == Closed) {

		} else if (state == Closing) {
			throw SpaceWireRTransmitTEPException(
					SpaceWireRTransmitTEPException::IlleagalOpenDirectionWhileClosingASocket);
		}
	}

public:
	void close() throw (SpaceWireRTransmitTEPException) {
		if (state == Enabled) {
			//todo
		} else if (state == Open) {
			//todo
		}
	}

public:
	TransmitTEPState getCurrentState() {
		return state;
	}

public:
	bool isOpen() {
		if (state == Open) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isEnabled() {
		if (state == Enabled) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosed() {
		if (state == Closed) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosing() {
		if (state == Closing) {
			return true;
		} else {
			return false;
		}
	}

};

#endif /* SPACEWIRERTRANSMITTEP_HH_ */
