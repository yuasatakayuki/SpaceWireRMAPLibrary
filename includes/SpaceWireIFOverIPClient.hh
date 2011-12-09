#ifndef SPACEWIREIFOVERIPCLIENT_HH_
#define SPACEWIREIFOVERIPCLIENT_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireIF.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireSSDTPModule.hh"

/** SpaceWire IF class which is connected to a real SpaceWire IF
 * via TCP/IP network and spw-tcpip bridge server running on SpaceCube.
 */
class SpaceWireIFOverIPClient: public SpaceWireIF, public TimecodeScynchronizedAction {
private:
	bool opened;
private:
	std::string iphostname;
	uint32_t ipportnumber;
	SpaceWireSSDTPModule* ssdtp;
	CxxUtilities::TCPClientSocket* datasocket;

public:
	SpaceWireIFOverIPClient(std::string newiphostname, uint32_t newipportnumber) :
		SpaceWireIF(), iphostname(newiphostname), ipportnumber(newipportnumber) {
	}

	~SpaceWireIFOverIPClient() {
	}

public:
	void open() throw (SpaceWireIFException) {
		datasocket = NULL;
		ssdtp = NULL;
		try {
			datasocket = new CxxUtilities::TCPClientSocket(iphostname, ipportnumber);
			datasocket->open(1000);
			datasocket->setTimeout(1000);
		} catch (CxxUtilities::TCPSocketException e) {
			throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
		} catch (...) {
			throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
		}

		ssdtp = new SpaceWireSSDTPModule(datasocket);
		ssdtp->setTimeCodeAction(this);
	}

	void close() throw (SpaceWireIFException) {
		if (datasocket != NULL) {
			datasocket->close();
			delete datasocket;
		}
		datasocket = NULL;
		if (ssdtp != NULL) {
			delete ssdtp;
		}
		ssdtp = NULL;
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
			}else{
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
		timeoutDurationInMicroSec=microsecond;
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
};

#endif /*SPACEWIREIFOVERIPCLIENT_HH_*/

/** History
 * 2008-08-26 file created (Takayuki Yuasa)
 * 2011-10-21 rewritten (Takayuki Yuasa)
 */
