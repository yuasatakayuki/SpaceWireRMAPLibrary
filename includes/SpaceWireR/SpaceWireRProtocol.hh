/*
 * SpaceWireRProtocol.hh
 *
 *  Created on: Apr 21, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERPROTOCOL_HH_
#define SPACEWIRERPROTOCOL_HH_

class SpaceWireRProtocol {
public:
	static uint8_t ProtocolID;
	static const size_t SizeOfSlidingWindow = 256;
};

uint8_t SpaceWireRProtocol::ProtocolID=0x10;

#endif /* SPACEWIRERPROTOCOL_HH_ */
