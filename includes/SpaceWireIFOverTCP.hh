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
class SpaceWireIFOverTCP: public SpaceWireIF, public TimecodeScynchronizedAction {
private:
	bool opened;
public:
	enum {
		ClientMode, ServerMode
	};

private:
	std::string iphostname;
	uint32_t ipportnumber;
	SpaceWireSSDTPModule* ssdtp;
	CxxUtilities::TCPSocket* datasocket;
	CxxUtilities::TCPServerSocket* serverSocket;

	uint32_t operationMode;

public:
	/** Constructor (client mode).
	 */
	SpaceWireIFOverTCP(std::string iphostname, uint32_t ipportnumber) :
		SpaceWireIF(), iphostname(iphostname), ipportnumber(ipportnumber) {
		setOperationMode(ClientMode);
	}

	/** Constructor (server mode).
	 */
	SpaceWireIFOverTCP(uint32_t ipportnumber) :
		SpaceWireIF(), ipportnumber(ipportnumber) {
		setOperationMode(ServerMode);
	}

	virtual ~SpaceWireIFOverTCP() {
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
				datasocket = new TCPClientSocket(iphostname, ipportnumber);
				((TCPClientSocket*) datasocket)->open(1000);
				((TCPClientSocket*) datasocket)->setTimeout(1000);
			} catch (CxxUtilities::TCPSocketException e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		} else { //server mode
			using namespace std;
			datasocket = NULL;
			ssdtp = NULL;
			try {
				serverSocket = new TCPServerSocket(ipportnumber);
				serverSocket->open();
				datasocket = serverSocket->accept();
				((TCPServerAcceptedSocket*) datasocket)->setTimeout(1000);
			} catch (CxxUtilities::TCPSocketException e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		}
		ssdtp = new SpaceWireSSDTPModule(datasocket);
		ssdtp->setTimeCodeAction(this);
	}

	void close() throw (SpaceWireIFException) {
		using namespace CxxUtilities;
		if (ssdtp != NULL) {
			delete ssdtp;
		}
		ssdtp = NULL;
		if (datasocket != NULL) {
			if (isClientMode()) {
				((TCPClientSocket*) datasocket)->close();
				delete (TCPClientSocket*)datasocket;
			} else {
				((TCPServerAcceptedSocket*) datasocket)->close();
				delete (TCPServerAcceptedSocket*)datasocket;
			}
		}
		datasocket = NULL;
		if (serverSocket != NULL) {
			serverSocket->close();
			delete serverSocket;
		}
	}

public:
	void send(uint8_t* data, size_t length, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
		using namespace std;
		try {
			ssdtp->send(data, length, eopType);
		} catch (SpaceWireSSDTPException e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			} else {
				throw SpaceWireIFException(SpaceWireIFException::Disconnected);
			}
		}
	}

public:
	void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) {
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
		} catch (SpaceWireSSDTPException e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) {
		timeIn = timeIn & 0x3F + controlFlagIn << 6;
		try {
			ssdtp->sendTimeCode(timeIn);
		} catch (SpaceWireSSDTPException e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
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
		try {
			ssdtp->setTxDivCount(txdivcount);
		} catch (SpaceWireSSDTPException e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) {
		datasocket->setTimeout(microsecond * 1000);
		timeoutDurationInMicroSec = microsecond;
	}

public:
	uint8_t getTimeCode() throw (SpaceWireIFException) {
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
};

/** History
 * 2008-08-26 file created (Takayuki Yuasa)
 * 2011-10-21 rewritten (Takayuki Yuasa)
 */

#endif /* SPACEWIREIFOVERTCP_HH_ */
