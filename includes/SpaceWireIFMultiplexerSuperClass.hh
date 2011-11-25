/*
 * SpaceWireIFMultiplexerSuperClass.hh
 *
 *  Created on: Nov 1, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIFMULTIPLEXERSUPERCLASS_HH_
#define SPACEWIREIFMULTIPLEXERSUPERCLASS_HH_

#include "SpaceWireIF.hh"

class SpaceWireIFMultiplexerSuperClass {
public:
	virtual void receive(std::vector<uint8_t>* buffer, SpaceWireIF* spwif) throw (SpaceWireIFException) {}
	virtual void cancelReceive(SpaceWireIF* spwif) throw (SpaceWireIFException) {}
};

#endif /* SPACEWIREIFMULTIPLEXERSUPERCLASS_HH_ */
