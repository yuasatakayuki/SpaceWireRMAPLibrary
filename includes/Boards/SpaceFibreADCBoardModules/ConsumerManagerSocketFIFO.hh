/*
 * ConsumerManager.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef CONSUMERMANAGERSOCKETFIFO_HH_
#define CONSUMERMANAGERSOCKETFIFO_HH_

#include "Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"
#include "CxxUtilities/CxxUtilities.hh"

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManagerSocketFIFO {
public:
	class ConsumerManagerSocketFIFODumpThread: public CxxUtilities::StoppableThread {

	private:
		ConsumerManagerSocketFIFO* parent;
		bool dumpEnabled = true;

	public:
		static constexpr double DumpPeriodInMilliSecond = 1000.0; //ms

	public:
		ConsumerManagerSocketFIFODumpThread(ConsumerManagerSocketFIFO* parent) {
			this->parent = parent;
		}
	public:
		void run() {
			using namespace std;
			CxxUtilities::Condition c;
			while (!stopped) {
				if (dumpEnabled) {
					cout << "ConsumerManagerSocketFIFO: Total " << parent->receivedBytes << " bytes received" << endl;
				}
				c.wait(DumpPeriodInMilliSecond);
			}
		}

	public:
		void enableDump() {
			dumpEnabled = true;
		}

	public:
		void disableDump() {
			dumpEnabled = false;
		}
	};

public:
	std::string ipAddress = "";
	static const int TCPPortNumber = 10031;
	CxxUtilities::TCPClientSocket* socket = NULL;

public:
	size_t receivedBytes = 0;

public:
	//Addresses of Consumer Manager Module
	static const uint32_t InitialAddressOf_ConsumerMgr = 0x01010000;
	static const uint32_t ConsumerMgrBA = InitialAddressOf_ConsumerMgr;
	static const uint32_t AddressOf_EventOutputDisableRegister = ConsumerMgrBA + 0x0100;
	static const uint32_t AddressOf_GateSize_FastGate_Register = ConsumerMgrBA + 0x010e;
	static const uint32_t AddressOf_GateSize_SlowGate_Register = ConsumerMgrBA + 0x0110;
	static const uint32_t AddressOf_NumberOf_BaselineSample_Register = ConsumerMgrBA + 0x0112;
	static const uint32_t AddressOf_ConsumerMgr_ResetRegister = ConsumerMgrBA + 0x0114;
	static const uint32_t AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
	static const uint32_t AddressOf_EventPacket_WaveformDownSampling_Register = ConsumerMgrBA + 0xFFFF;

	//Addresses of SDRAM
	static const uint32_t InitialAddressOf_Sdram_EventList = 0x00000000;
	static const uint32_t FinalAddressOf_Sdram_EventList = 0x00fffffe;

private:
	ConsumerManagerSocketFIFODumpThread* dumpThread;

public:
	static const uint32_t TCPPortOfEventDataPort = 10031;
	static constexpr double TCPSocketTimeoutDurationInMilliSec = 1000;

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;

public:
	/** Constructor.
	 * @param ipAddress IP Address of the SpaceFibre ADC Board
	 */
	ConsumerManagerSocketFIFO(std::string ipAddress, RMAPHandler* rmapHandler, RMAPTargetNode* adcRMAPTargetNode) {
		this->ipAddress = ipAddress;
		this->rmapHandler = rmapHandler;
		this->adcRMAPTargetNode = adcRMAPTargetNode;

		dumpThread = new ConsumerManagerSocketFIFODumpThread(this);
		dumpThread->start();
	}

public:
	virtual ~ConsumerManagerSocketFIFO() {
		closeSocket();
		this->stopDumpThread();
	}

public:
	/** Resets ConsumerManager module.
	 */
	void reset() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::reset()...";
		}
		rmapHandler->setRegister(AddressOf_ConsumerMgr_ResetRegister, 0x0001);
		if (Debug::consumermanager()) {
			cout << "done" << endl;
		}
	}

private:
	std::vector<uint8_t> receiveBuffer;
	static const size_t ReceiveBufferSize = 4096;

public:
	/** Retrieve data stored in the SDRAM (via ConsumerManager module).
	 * The size of data varies depending on the event data in the SDRAM.
	 * @param maximumsize maximum data size to be returned (in bytes)
	 */
	std::vector<uint8_t> getEventData(uint32_t maximumsize = 4000) throw (CxxUtilities::TCPSocketException) {
		using namespace std;

		receiveBuffer.resize(ReceiveBufferSize);

		//open socket if necessary
		if (socket == NULL) {
			try {
				cerr << "ConsumerManagerSocketFIFO::read(): trying to open a TCP socket" << endl;
				this->openSocket();
				cerr << "ConsumerManagerSocketFIFO::read(): socket connected" << endl;
			} catch (CxxUtilities::TCPSocketException& e) {
				if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
					cerr << "ConsumerManagerSocketFIFO::read(): timeout" << endl;
				} else {
					cerr << "ConsumerManagerSocketFIFO::read(): TCPSocketException " << e.toString() << endl;
				}
				throw e;
			}
		}

		//receive via TCP socket (ConsumerManagerSocketFIFO in the FPGA will send event packets byte-by-byte)
		try {
			if (Debug::consumermanager()) {
				cout << "ConsumerManagerSocketFIFO::getEventData(): trying to receive data" << endl;
			}
			socket->setTimeout(TCPSocketTimeoutDurationInMilliSec);
			size_t receivedSize = socket->receive(&(receiveBuffer[0]), ReceiveBufferSize);
			if (Debug::consumermanager()) {
				cout << "ConsumerManagerSocketFIFO::getEventData(): received " << receivedSize << " bytes" << endl;
			}
			receiveBuffer.resize(receivedSize);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				cerr << "ConsumerManagerSocketFIFO::getEventData(): timeout" << endl;
				receiveBuffer.resize(0);
			} else {
				cerr << "ConsumerManagerSocketFIFO::getEventData(): TCPSocketException on receive()" << e.toString() << endl;
				throw e;
			}
		}

		//return result
		receivedBytes += receiveBuffer.size();
		return receiveBuffer;
	}

public:
	void openSocket() throw (CxxUtilities::TCPSocketException) {
		using namespace std;
		if (ipAddress == "") {
			using namespace std;
			cerr << "ConsumerManagerSocketFIFO::openSocket(): IP address is empty" << endl;
		}
		socket = new CxxUtilities::TCPClientSocket(ipAddress, TCPPortNumber);
		try {
			socket->open(1000);
			if (Debug::consumermanager()) {
				cout << "ConsumerManagerSocketFIFO::openSocket(): socket opened" << endl;
			}
			socket->setTimeout(TCPSocketTimeoutDurationInMilliSec);
			if (Debug::consumermanager()) {
				cout << "ConsumerManagerSocketFIFO::openSocket(): timeout duration of " << TCPSocketTimeoutDurationInMilliSec
						<< " ms set" << endl;
			}
		} catch (CxxUtilities::TCPSocketException& e) {
			cerr << e.toString() << endl;
			exit(-1);
		}

	}

public:
	void closeSocket() {
		using namespace std;
		if (socket != NULL) {
			cerr << "ConsumerManagerSocketFIFO::closeSocket(): socket closed" << endl;
			socket->close();
			delete socket;
			socket = NULL;
		}
	}

public:
	void enableStatusDump() {
		dumpThread->enableDump();
	}

public:
	void disableStatusDump() {
		dumpThread->disableDump();
	}

public:
	/** Enables event data output via TCP/IP.
	 */
	void enableEventDataOutput() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "Enabling event data output...";
		}
		rmapHandler->setRegister(AddressOf_EventOutputDisableRegister, 0x0000);
		if (Debug::consumermanager()) {
			uint16_t eventOutputDisableRegister=rmapHandler->getRegister(AddressOf_EventOutputDisableRegister);
			cout << "done(disableRegister=" << hex << setw(4) << eventOutputDisableRegister << dec << ")" << endl;
		}
	}

public:
	/** Disables event data output via TCP/IP.
	 * This is used to temporarily stop event data output via
	 * TCP/IP, and force processed event data to be buffered
	 * in the CosumerManager module. By doing these, the user application
	 * software can handle the memory usage especially in high event rate
	 * case.
	 */
	void disableEventDataOutput() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "Disabling event data output...";
		}
		rmapHandler->setRegister(AddressOf_EventOutputDisableRegister, 0x0001);
		if (Debug::consumermanager()) {
			uint16_t eventOutputDisableRegister=rmapHandler->getRegister(AddressOf_EventOutputDisableRegister);
			cout << "done(disableRegister=" << hex << setw(4) << eventOutputDisableRegister << dec << ")" << endl;
		}
	}

public:
	void startDumpThread() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::stopDumpThread(): starting status dump thread" << endl;
		}
		dumpThread->start();
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::stopDumpThread(): status dump thread started" << endl;
		}
	}

public:
	void stopDumpThread() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::stopDumpThread(): stopping status dump thread" << endl;
		}
		dumpThread->stop();
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::stopDumpThread(): waiting for status dump thread to actually stop" << endl;
		}
		dumpThread->waitUntilRunMethodComplets();
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::stopDumpThread(): status dump thread stopped" << endl;
		}
	}

public:
	/** Sets EventPacket_NumberOfWaveform_Register
	 * @param numberofsamples number of data points to be recorded in an event packet
	 */
	virtual void setEventPacket_NumberOfWaveform(uint16_t nSamples) {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManagerSocketFIFO::setEventPacket_NumberOfWaveform(" << nSamples << ")...";
		}
		rmapHandler->setRegister(AddressOf_EventPacket_NumberOfWaveform_Register, nSamples);

		if (Debug::consumermanager()) {
			cout << "done" << endl;
			uint16_t nSamplesRead = rmapHandler->getRegister(AddressOf_EventPacket_NumberOfWaveform_Register);
			cout << "readdata[0]:" << (uint32_t) nSamplesRead << endl;
		}
	}
};

#endif /* CONSUMERMANAGERSOCKETFIFO_HH_ */
