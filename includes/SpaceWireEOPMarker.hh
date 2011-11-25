/*
 * SpaceWireEOPMarker.hh
 *
 *  Created on: Oct 21, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREEOPMARKER_HH_
#define SPACEWIREEOPMARKER_HH_

class SpaceWireEOPMarker {
public:
	enum {
		EOP=0,
		EEP=1,
		Continued=0xFFFF
	} EOPTypes;
};

#endif /* SPACEWIREEOPMARKER_HH_ */
