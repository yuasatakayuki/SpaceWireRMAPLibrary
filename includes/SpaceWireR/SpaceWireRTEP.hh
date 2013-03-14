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
 * SpaceWireRTEP.hh
 *
 *  Created on: Jan 14, 2013
 *      Author: yuasa
 */

#ifndef SPACEWIRERTEP_HH_
#define SPACEWIRERTEP_HH_

#include "SpaceWireR/SpaceWireRClassInterfaces.hh"
#include "SpaceWireR/SpaceWireREngine.hh"
#include "SpaceWireR/SpaceWireRPacket.hh"
#include "SpaceWireR/SpaceWireRTEPExceptions.hh"

class SpaceWireRTEPType {
public:
	enum Type {
		TransmitTEP, //
		ReceiveTEP
	};
};

class SpaceWireRTEPState {
public:
	enum State {
		Closed, Enabled, Open, Closing
	};

public:
	static std::string toString(State state) {
		std::string str;
		switch (state) {
		case Closed:
			str = "Closed";
			break;
		case Closing:
			str = "Closing";
			break;
		case Enabled:
			str = "Enabled";
			break;
		case Open:
			str = "Open";
			break;
		default:
			str = "Undefined status (malfunctioning!)";
			break;
		}
		return str;
	}
};

class SpaceWireRTEP: public SpaceWireRTEPInterface {
protected:
	SpaceWireRTEPType::Type tepType;

protected:
	SpaceWireRTEPState::State state;

protected:
	//routing information
	std::vector<uint8_t> destinationSpaceWireAddress;
	std::vector<uint8_t> sourceSpaceWireAddress;
	uint8_t destinationLogicalAddress;
	uint8_t sourceLogicalAddress;

protected:
	SpaceWireREngine* spwREngine;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;

protected:
	std::vector<SpaceWireRPacket*> slidingWindowBuffer;
	uint8_t slidingWindowFrom;
	uint8_t slidingWindowSize;

protected:
	CxxUtilities::Condition stateTransitionNotifier;

public:
	static const size_t DefaultSlidingWindowSize = 8;
	static const size_t MaxOfSlidingWindow = 256;

public:
	static const double WaitDurationInMsForClosedLoop = 100; //m1s
	static const double WaitDurationInMsForReceive = 1000; //ms
	static const double WaitDurationForPacketReceiveLoop = 100; //ms
	static const double WaitDurationForTransitionFromClosingToClosed = 1000; //ms

public:
	SpaceWireRTEP(SpaceWireRTEPType::Type tepType, SpaceWireREngine* spwREngine, uint16_t channel) {
		this->spwREngine = spwREngine;
		this->state = SpaceWireRTEPState::Closed;
		this->tepType = tepType;
		this->channel = channel;
		this->sourceLogicalAddress = DefaultLogicalAddress;
		this->destinationLogicalAddress = DefaultLogicalAddress;
		initializeSlidingWindow();
	}

public:
	virtual ~SpaceWireRTEP() {
	}

protected:
	void initializeSlidingWindow() {
		slidingWindowBuffer.clear();
		slidingWindowBuffer.resize(MaxOfSlidingWindow, NULL);
		slidingWindowSize = DefaultSlidingWindowSize;
		slidingWindowFrom = 0;
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

public:
	SpaceWireRTEPState::State getState() {
		return state;
	}

public:
	bool isOpen() {
		if (state == SpaceWireRTEPState::Open) {
			return true;
		} else {
			return false;
		}
	}

public:
	void setEnabled() {
		state = SpaceWireRTEPState::Enabled;
	}

public:
	bool isEnabled() {
		if (state == SpaceWireRTEPState::Enabled) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosed() {
		if (state == SpaceWireRTEPState::Closed) {
			return true;
		} else {
			return false;
		}
	}

public:
	bool isClosing() {
		if (state == SpaceWireRTEPState::Closing) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline uint8_t getSlidingWindowFrom() const {
		return slidingWindowFrom;
	}

public:
	inline void setSlidingWindowFrom(uint8_t slidingWindowFrom) {
		this->slidingWindowFrom = slidingWindowFrom;
	}

public:
	inline uint8_t getSlidingWindowSize() const {
		return slidingWindowSize;
	}

public:
	inline void setSlidingWindowSize(uint8_t slidingWindowSize) {
		this->slidingWindowSize = slidingWindowSize;
	}

protected:
	inline void discardReceivedPackets() {
		while (receivedPackets.size() != 0) {
			delete receivedPackets.front();
			receivedPackets.pop_front();
		}
	}

protected:
	inline bool insideForwardSlidingWindow(uint8_t sequenceNumber) {
		uint8_t n = this->slidingWindowFrom;
		uint8_t k = this->slidingWindowSize;
		if ((uint8_t) (n) < (uint8_t) (n + k - 1)) {
			// n -- n + k - 1
			if (n <= sequenceNumber && sequenceNumber <= (uint8_t) (n + k - 1)) {
				return true;
			} else {
				return false;
			}
		} else {
			// n -- 255, 0 -- n + k - 1
			if ((n <= sequenceNumber && sequenceNumber <= 255)
					|| (0 <= sequenceNumber && sequenceNumber <= (uint8_t) (n + k - 1))) {
				return true;
			} else {
				return false;
			}
		}
	}

protected:
	inline bool insideBackwardSlidingWindow(uint8_t sequenceNumber) {
		uint8_t n = this->slidingWindowFrom;
		uint8_t k = this->slidingWindowSize;
		if ((uint8_t) (n - k) < (uint8_t) (n - 1)) {
			//n-k -- n-1
			if ((uint8_t) (n - k) <= sequenceNumber && sequenceNumber <= (uint8_t) (n - 1)) {
				return true;
			} else {
				return false;
			}
		} else {
			//n-k -- 255, 0 -- n - 1
			if (((uint8_t) (n - k) <= sequenceNumber && sequenceNumber <= 255)
					|| (0 <= sequenceNumber && sequenceNumber <= (uint8_t) (n - 1))) {
				return true;
			} else {
				return false;
			}
		}
	}

public:
	virtual std::string toString() = 0;

public:
	void closeDueToSpaceWireIFFailure() {
		this->state = SpaceWireRTEPState::Closed;
	}


};

#endif /* SPACEWIRERTEP_HH_ */
