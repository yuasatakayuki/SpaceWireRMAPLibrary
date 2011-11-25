#ifndef SPACEWIRESSDTPMODULE_HH_
#define SPACEWIRESSDTPMODULE_HH_

#include "CxxUtilities/CommonHeader.hh"
#include "CxxUtilities/Exception.hh"
#include "CxxUtilities/Mutex.hh"
#include "CxxUtilities/Condition.hh"
#include "CxxUtilities/TCPSocket.hh"

#include "SpaceWireIF.hh"

/** Exception class for SpaceWireSSDTPModule.
 */
class SpaceWireSSDTPException: public CxxUtilities::Exception {
public:
	enum {
		DataSizeTooLarge,
		Disconnected,
		Timeout,
		TimecodeReceived,
		TCPSocketError,
		EEP,
		SequenceError,
		NotImplemented,
		Undefined
	};

public:
	SpaceWireSSDTPException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}
};

/** This class performs synchronous data transfer via
 * TCP/IP network using "Simple- Synchronous- Data Transfer Protocol"
 * which is defined for this class.
 */
class SpaceWireSSDTPModule {
public:
	static const uint32_t BufferSize = 1024 * 1024;

private:
	CxxUtilities::TCPSocket* datasocket;
	uint8_t* sendbuffer;
	uint8_t* receivebuffer;
	std::stringstream ss;
	uint8_t internal_timecode;
	uint32_t latest_sentsize;
	CxxUtilities::Mutex sendmutex;
	CxxUtilities::Mutex receivemutex;
	TimecodeScynchronizedAction* timecodeaction;

private:
	/* for SSDTP2 */
	CxxUtilities::Mutex registermutex;
	CxxUtilities::Condition registercondition;
	std::map<uint32_t, uint32_t> registers;

private:
	uint8_t rheader[12];
	uint8_t r_tmp[30];
	uint8_t sheader[12];

public:
	size_t receivedsize;
	size_t rbuf_index;

public:
	SpaceWireSSDTPModule(CxxUtilities::TCPSocket* newdatasocket) {
		datasocket = newdatasocket;
		sendbuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		receivebuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		internal_timecode = 0x00;
		latest_sentsize = 0;
		timecodeaction = NULL;
	}

	~SpaceWireSSDTPModule() {
		free(sendbuffer);
	}

	void send(std::vector<uint8_t>& data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		send(&data, eopType);
		sendmutex.unlock();
	}

	void send(std::vector<uint8_t>* data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		uint32_t size = data->size();
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = 0x00;
		} else {
			sheader[0] = 0x01;
		}
		sheader[1] = 0x00;
		for (uint32_t i = 11; i > 1; i--) {
			sheader[i] = size % 0x100;
			size = size / 0x100;
		}
		try {
			datasocket->send(sheader, 12);
			datasocket->send(&(data->at(0)), data->size());
		} catch (...) {
			sendmutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
		}
		sendmutex.unlock();
	}

	void send(uint8_t* data, uint32_t size, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = 0x00;
		} else {
			sheader[0] = 0x01;
		}
		sheader[1] = 0x00;
		uint32_t asize = size;
		for (uint32_t i = 11; i > 1; i--) {
			sheader[i] = asize % 0x100;
			asize = asize / 0x100;
		}
		try {
			datasocket->send(sheader, 12);
			datasocket->send(data, size);
		} catch (...) {
			sendmutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
		}
		sendmutex.unlock();
	}

public:
	std::vector<uint8_t> receive() throw (SpaceWireSSDTPException) {
		receivemutex.lock();
		std::vector<uint8_t> data;
		uint32_t eopType;
		receive(&data, eopType);
		receivemutex.unlock();
		return data;
	}

	int receive(std::vector<uint8_t>* data, uint32_t& eopType) throw (SpaceWireSSDTPException) {
		using namespace std;
		//		cout << "#1" << endl;
		uint32_t size = 0;
		uint32_t hsize = 0;
		uint32_t flagment_size = 0;
		uint32_t received_size = 0;
		//header
		receive_header: rheader[0] = 0xFF;
		rheader[1] = 0x00;
		while (rheader[0] != DataFlag_Complete_EOP && rheader[0] != DataFlag_Complete_EEP) {
//			cout << "#2" << endl;
			receivemutex.lock();
//			cout << "#2-1" << endl;
			hsize = 0;
			flagment_size = 0;
			received_size = 0;
			//flag and size part
			try {
//				cout << "#2-2" << endl;
				while (hsize != 12) {
//					cout << "#2-3" << endl;
					int result = datasocket->receive(rheader + hsize, 12 - hsize);
					hsize += result;
//					cout << "#2-4" << endl;
				}
			} catch (CxxUtilities::TCPSocketException e) {
//				cout << "#2-5" << endl;
				receivemutex.unlock();
				if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
//					cout << "#2-6" << endl;
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
				} else {
//					cout << "#2-6" << endl;
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
				}
			} catch (...) {
				receivemutex.unlock();
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}

//			cout << "#3" << endl;
			//data or control code part
			if (rheader[0] == DataFlag_Complete_EOP || rheader[0] == DataFlag_Complete_EEP || rheader[0]
					== DataFlag_Flagmented) {
//				cout << "#4" << endl;
				//data
				for (uint32_t i = 2; i < 12; i++) {
					flagment_size = flagment_size * 0x100 + rheader[i];
				}
				//uint8_t* data_pointer=&(data->at(0));
				uint8_t* data_pointer = receivebuffer;
//				cout << "#5" << endl;
				while (received_size != flagment_size) {
//					cout << "#6" << endl;
					int result;
					try {
						result
								= datasocket->receive(data_pointer + size + received_size, flagment_size
										- received_size);
					} catch (CxxUtilities::TCPSocketException e) {
						cout << "SSDTPModule::receive() exception when receiving data" << endl;
						cout << "rheader[0]=0x" << setw(2) << setfill('0') << hex << (uint32_t) rheader[0] << endl;
						cout << "rheader[1]=0x" << setw(2) << setfill('0') << hex << (uint32_t) rheader[1] << endl;
						cout << "size=" << dec << size << endl;
						cout << "flagment_size=" << dec << flagment_size << endl;
						cout << "received_size=" << dec << received_size << endl;
						exit(-1);
					}
					received_size += result;
				}
//				cout << "#7" << endl;
				size += received_size;
			} else if (rheader[0] == ControlFlag_SendTimeCode || rheader[0] == ControlFlag_GotTimeCode) {
				//control
				uint8_t timecode_and_reserved[2];
				uint32_t tmp_size = 0;
				try {
					while (tmp_size != 2) {
						int result = datasocket->receive(timecode_and_reserved + tmp_size, 2 - tmp_size);
						tmp_size += result;
					}
				} catch (...) {
					receivemutex.unlock();
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
				}
				switch (rheader[0]) {
				case ControlFlag_SendTimeCode:
					internal_timecode = timecode_and_reserved[0];
					receivemutex.unlock();
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::TimecodeReceived);
					break;
				case ControlFlag_GotTimeCode:
					internal_timecode = timecode_and_reserved[0];
					receivemutex.unlock();
					gotTimeCode(internal_timecode);
					break;
				}
//				cout << "#11" << endl;
			} else {
				cout << "SSDTP fatal error with flag value of 0x" << hex << (uint32_t) rheader[0] << dec << endl;
				exit(-1);
			}
		}
//		cout << "#8 " << size << endl;
		data->resize(size);
		if (size != 0) {
			memcpy(&(data->at(0)), receivebuffer, size);
		} else {
			goto receive_header;
		}
		if (rheader[0] == DataFlag_Complete_EOP) {
			eopType = SpaceWireEOPMarker::EOP;
		} else if (rheader[0] == DataFlag_Complete_EEP) {
			eopType = SpaceWireEOPMarker::EEP;
		} else {
			eopType = SpaceWireEOPMarker::Continued;
		}
//		cout << "#9" << endl;
		receivemutex.unlock();
		return size;
	}

	void sendEEP() throw (SpaceWireSSDTPException) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

	void sendTimeCode(uint8_t timecode) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		sendbuffer[0] = SpaceWireSSDTPModule::ControlFlag_SendTimeCode;
		sendbuffer[1] = 0x00; //Reserved
		for (uint32_t i = 0; i < LengthOfSizePart - 1; i++) {
			sendbuffer[i + 2] = 0x00;
		}
		sendbuffer[11] = 0x02; //2bytes = 1byte timecode + 1byte reserved
		sendbuffer[12] = timecode;
		sendbuffer[13] = 0;
		try {
			sendmutex.lock();
			datasocket->send(sendbuffer, 14);
			sendmutex.unlock();
		} catch (CxxUtilities::TCPSocketException e) {
			sendmutex.unlock();
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
		sendmutex.unlock();
	}

	uint8_t getTimeCode() throw (SpaceWireSSDTPException) {
		return internal_timecode;
	}

	void setTimeCodeAction(TimecodeScynchronizedAction* action) {
		timecodeaction = action;
	}

	void setTxFrequency(double frequencyInMHz) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

	void gotTimeCode(uint8_t timecode) {
		using namespace std;
		if (timecodeaction != NULL) {
			timecodeaction->doAction(timecode);
		} else {
			cout << "SSDTPModule::gotTimeCode(): Got TimeCode : " << hex << setw(2) << setfill('0')
					<< (uint32_t) timecode << dec << endl;
		}
	}

	void registerRead(uint32_t address) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

	void registerWrite(uint32_t address, std::vector<uint8_t> data) {
		//send command
		sendmutex.lock();
		sendbuffer[0] = ControlFlag_RegisterAccess_WriteCommand;
		sendmutex.unlock();
	}

	void setTxDivCount(uint8_t txdivcount) {
		sendmutex.lock();
		sendbuffer[0] = SpaceWireSSDTPModule::ControlFlag_ChangeTxSpeed;
		sendbuffer[1] = 0x00; //Reserved
		for (uint32_t i = 0; i < LengthOfSizePart - 1; i++) {
			sendbuffer[i + 2] = 0x00;
		}
		sendbuffer[11] = 0x02; //2bytes = 1byte txdivcount + 1byte reserved
		sendbuffer[12] = txdivcount;
		sendbuffer[13] = 0;
		try {
			datasocket->send(sendbuffer, 14);
		} catch (CxxUtilities::TCPSocketException e) {
			sendmutex.unlock();
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
		sendmutex.unlock();
	}


public:
	void sendRawData(uint8_t* data,size_t length) throw (SpaceWireSSDTPException){
		sendmutex.lock();
		try {
			datasocket->send(data, length);
		} catch (CxxUtilities::TCPSocketException e) {
			sendmutex.unlock();
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
		sendmutex.unlock();
	}

public:
	/* for SSDTP2 */
	static const uint8_t DataFlag_Complete_EOP = 0x00;
	static const uint8_t DataFlag_Complete_EEP = 0x01;
	static const uint8_t DataFlag_Flagmented = 0x02;
	static const uint8_t ControlFlag_SendTimeCode = 0x30;
	static const uint8_t ControlFlag_GotTimeCode = 0x31;
	static const uint8_t ControlFlag_ChangeTxSpeed = 0x38;
	static const uint8_t ControlFlag_RegisterAccess_ReadCommand = 0x40;
	static const uint8_t ControlFlag_RegisterAccess_ReadReply = 0x41;
	static const uint8_t ControlFlag_RegisterAccess_WriteCommand = 0x50;
	static const uint8_t ControlFlag_RegisterAccess_WriteReply = 0x51;
	static const uint32_t LengthOfSizePart = 10;
};

#endif /*SPACEWIRESSDTPMODULE_HH_*/

/** History
 * 2008-06-xx file created (Takayuki Yuasa)
 * 2008-12-17 TimeCode implemented (Takayuki Yuasa)
 */
