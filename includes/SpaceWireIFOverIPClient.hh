#ifndef SPACEWIREIFOVERIPCLIENT_HH_
#define SPACEWIREIFOVERIPCLIENT_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireIFOverTCP.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireSSDTPModule.hh"

/** SpaceWire IF class which is connected to a real SpaceWire IF
 * via TCP/IP network and spw-tcpip bridge server running on SpaceCube.
 */
class SpaceWireIFOverIPClient: public SpaceWireIFOverTCP {
public:
	SpaceWireIFOverIPClient(std::string iphostname, uint32_t ipportnumber) :
		SpaceWireIFOverTCP(iphostname, ipportnumber) {
	}

	~SpaceWireIFOverIPClient() {
	}
};

#endif /*SPACEWIREIFOVERIPCLIENT_HH_*/

/** History
 * 2008-08-26 file created (Takayuki Yuasa)
 * 2011-10-21 rewritten (Takayuki Yuasa)
 */
