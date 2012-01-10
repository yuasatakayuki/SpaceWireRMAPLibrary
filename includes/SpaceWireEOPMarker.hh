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
	enum EOPType {
		EOP=0,
		EEP=1,
		Continued=0xFFFF
	};
};

#endif /* SPACEWIREEOPMARKER_HH_ */
