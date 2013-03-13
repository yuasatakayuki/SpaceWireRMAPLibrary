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
