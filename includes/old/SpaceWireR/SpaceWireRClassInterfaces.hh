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
 * SpaceWireRClassInterfaces.hh
 *
 *  Created on: Feb 22, 2013
 *      Author: yuasa
 */

#ifndef SPACEWIRERCLASSINTERFACES_HH_
#define SPACEWIRERCLASSINTERFACES_HH_

#include "SpaceWireR/SpaceWireRPacket.hh"

class SpaceWireRTEPInterface {
public:
	virtual ~SpaceWireRTEPInterface() {
	}

public:
	std::list<SpaceWireRPacket*> receivedPackets;
	CxxUtilities::Condition packetArrivalNotifier; //used to notify arrival of SpaceWire-R packets by SpaceWire-R Engine
	uint16_t channel;

public:
	virtual void closeDueToSpaceWireIFFailure() = 0;

private:
	CxxUtilities::Mutex mutexReceivedPackets;

public:
	void pushReceivedSpaceWireRPacket(SpaceWireRPacket* packet) {
		mutexReceivedPackets.lock();
		receivedPackets.push_back(packet);
		mutexReceivedPackets.unlock();
	}

protected:
	SpaceWireRPacket* popReceivedSpaceWireRPacket() {
		mutexReceivedPackets.lock();
		SpaceWireRPacket* result = *(receivedPackets.begin());
		receivedPackets.pop_front();
		mutexReceivedPackets.unlock();
		return result;
	}
};

#endif /* SPACEWIRERCLASSINTERFACES_HH_ */
