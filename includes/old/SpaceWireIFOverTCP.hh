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
 * SpaceWireIFOverTCP.hh
 *
 *  Created on: Dec 12, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIFOVERTCP_HH_
#define SPACEWIREIFOVERTCP_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireIF.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireSSDTPModule.hh"

/** SpaceWire IF class which is connected to a real SpaceWire IF
 * via TCP/IP network and spw-tcpip bridge server running on SpaceCube.
 */
class SpaceWireIFOverTCP: public SpaceWireIF, public SpaceWireIFActionTimecodeScynchronizedAction {
private:
	bool opened;
public:
	enum {
		ClientMode, ServerMode
	};

private:
	std::string iphostname;
	uint32_t portnumber;
	SpaceWireSSDTPModule* ssdtp;
	CxxUtilities::TCPSocket* datasocket;
	CxxUtilities::TCPServerSocket* serverSocket;

	uint32_t operationMode;

public:
	/** Constructor (client mode).
	 */
	SpaceWireIFOverTCP(std::string iphostname, uint32_t portnumber) :
			SpaceWireIF(), iphostname(iphostname), portnumber(portnumber) {
		setOperationMode(ClientMode);
	}

	/** Constructor (server mode).
	 */
	SpaceWireIFOverTCP(uint32_t portnumber) :
			SpaceWireIF(), portnumber(portnumber) {
		setOperationMode(ServerMode);
	}

	/** Constructor. Server/client mode will be determined later via
	 * the setClientMode() or setServerMode() method.
	 */
	SpaceWireIFOverTCP() :
			SpaceWireIF() {

	}

	virtual ~SpaceWireIFOverTCP() {
	}

public:
	void setClientMode(std::string iphostname, uint32_t portnumber) {
		setOperationMode(ClientMode);
		this->iphostname = iphostname;
		this->portnumber = portnumber;
	}

	void setServerMode(uint32_t portnumber) {
		setOperationMode(ServerMode);
		this->portnumber = portnumber;
	}

public:
	void open() throw (SpaceWireIFException) {
		using namespace std;
		using namespace std;
		using namespace CxxUtilities;
		if (isClientMode()) { //client mode
			datasocket = NULL;
			ssdtp = NULL;
			try {
				datasocket = new TCPClientSocket(iphostname, portnumber);
				((TCPClientSocket*) datasocket)->open(1000);
				setTimeoutDuration(500000);
			} catch (CxxUtilities::TCPSocketException& e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		} else { //server mode
			using namespace std;
			datasocket = NULL;
			ssdtp = NULL;
			try {
				serverSocket = new TCPServerSocket(portnumber);
				serverSocket->open();
				datasocket = serverSocket->accept();
				setTimeoutDuration(500000);
			} catch (CxxUtilities::TCPSocketException& e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		}
		datasocket->setNoDelay();
		ssdtp = new SpaceWireSSDTPModule(datasocket);
		ssdtp->setTimeCodeAction(this);
		state = Opened;
	}

	void close() throw (SpaceWireIFException) {
		using namespace CxxUtilities;
		using namespace std;
		if (state == Closed) {
			return;
		}
		state = Closed;
		//invoke SpaceWireIFCloseActions to tell other instances
		//closing of this SpaceWire interface
		invokeSpaceWireIFCloseActions();
		if (ssdtp != NULL) {
			delete ssdtp;
		}
		ssdtp = NULL;
		if (datasocket != NULL) {
			if (isClientMode()) {
				((TCPClientSocket*) datasocket)->close();
				delete (TCPClientSocket*) datasocket;
			} else {
				((TCPServerAcceptedSocket*) datasocket)->close();
				delete (TCPServerAcceptedSocket*) datasocket;
			}
		}
		datasocket = NULL;
		if (isServerMode()) {
			if (serverSocket != NULL) {
				serverSocket->close();
				delete serverSocket;
			}
		}
	}

public:
	void send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		using namespace std;
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			ssdtp->send(data, length, eopType);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			} else {
				throw SpaceWireIFException(SpaceWireIFException::Disconnected);
			}
		}
	}

public:
	void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			uint32_t eopType;
			ssdtp->receive(buffer, eopType);
			if (eopType == SpaceWireEOPMarker::EEP) {
				this->setReceivedPacketEOPMarkerType(SpaceWireIF::EEP);
				if (this->eepShouldBeReportedAsAnException_) {
					throw SpaceWireIFException(SpaceWireIFException::EEP);
				}
			} else {
				this->setReceivedPacketEOPMarkerType(SpaceWireIF::EOP);
			}
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			using namespace std;
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			using namespace std;
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) {
		using namespace std;
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		timeIn = timeIn % 64 + (controlFlagIn << 6);
		try {
			//emit timecode via SSDTP module
			ssdtp->sendTimeCode(timeIn);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
		//invoke timecode synchronized action
		if (timecodeSynchronizedActions.size() != 0) {
			this->invokeTimecodeSynchronizedActions(timeIn);
		}
	}

public:
	virtual void setTxLinkRate(uint32_t linkRateType) throw (SpaceWireIFException) {
		using namespace std;
		cerr << "SpaceWireIFOverIPClient::setTxLinkRate() is not implemented." << endl;
		cerr << "Please use SpaceWireIFOverIPClient::setTxDivCount() instead." << endl;
		throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
	}

	virtual uint32_t getTxLinkRateType() throw (SpaceWireIFException) {
		using namespace std;
		cerr << "SpaceWireIFOverIPClient::getTxLinkRate() is not implemented." << endl;
		throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
	}

public:
	void setTxDivCount(uint8_t txdivcount) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			ssdtp->setTxDivCount(txdivcount);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) {
		datasocket->setTimeout(microsecond / 1000.);
		timeoutDurationInMicroSec = microsecond;
	}

public:
	uint8_t getTimeCode() throw (SpaceWireIFException) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		return ssdtp->getTimeCode();
	}

	void doAction(uint8_t timecode) {
		this->invokeTimecodeSynchronizedActions(timecode);
	}

public:
	SpaceWireSSDTPModule* getSSDTPModule() {
		return ssdtp;
	}

	uint32_t getOperationMode() const {
		return operationMode;
	}

	void setOperationMode(uint32_t operationMode) {
		this->operationMode = operationMode;
	}

	bool isClientMode() {
		if (operationMode == ClientMode) {
			return true;
		} else {
			return false;
		}
	}

	bool isServerMode() {
		if (operationMode != ClientMode) {
			return true;
		} else {
			return false;
		}
	}

public:
	/** Cancels ongoing receive() method if any exist.
	 */
	void cancelReceive(){}
};

/** History
 * 2008-08-26 file created (Takayuki Yuasa)
 * 2011-10-21 rewritten (Takayuki Yuasa)
 */

#endif /* SPACEWIREIFOVERTCP_HH_ */
