/*
 * SpaceWireProtocol.hh
 *
 *  Created on: Aug 10, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREPROTOCOL_HH_
#define SPACEWIREPROTOCOL_HH_

/** A class that collects constant parameters related to the SpaceWire protocol. */
class SpaceWireProtocol {
public:
	static const uint8_t DefaultLogicalAddress=0xFE;

public:
	static const uint8_t MinimumLogicalAddress=0x20;
	static const uint8_t MaximumLogicalAddress=0xFF;

};
#endif /* SPACEWIREPROTOCOL_HH_ */
