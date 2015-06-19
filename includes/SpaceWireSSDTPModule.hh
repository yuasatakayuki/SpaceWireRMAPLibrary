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
#ifndef SPACEWIRESSDTPMODULE_HH_
#define SPACEWIRESSDTPMODULE_HH_

#include "CxxUtilities/CommonHeader.hh"
#include "CxxUtilities/Exception.hh"
#include "CxxUtilities/Mutex.hh"
#include "CxxUtilities/Condition.hh"
#include "CxxUtilities/TCPSocket.hh"

#include "SpaceWireIF.hh"

/** An exception class used by SpaceWireSSDTPModule.
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

public:
	virtual ~SpaceWireSSDTPException() {
	}

public:
	std::string toString() {
		std::string result;
		switch (status) {

		case DataSizeTooLarge:
			result = "DataSizeTooLarge";
			break;
		case Disconnected:
			result = "Disconnected";
			break;
		case Timeout:
			result = "Timeout";
			break;
		case TimecodeReceived:
			result = "TimecodeReceived";
			break;
		case TCPSocketError:
			result = "TCPSocketError";
			break;
		case EEP:
			result = "EEP";
			break;
		case SequenceError:
			result = "SequenceError";
			break;
		case NotImplemented:
			result = "NotImplemented";
			break;
		case Undefined:
			result = "Undefined";
			break;

		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

/** A class that performs synchronous data transfer via
 * TCP/IP network using "Simple- Synchronous- Data Transfer Protocol"
 * which is defined for this class.
 */
class SpaceWireSSDTPModule {
public:
	static const uint32_t BufferSize = 10 * 1024 * 1024;

private:
	bool closed = false;

private:
	CxxUtilities::TCPSocket* datasocket;
	uint8_t* sendbuffer;
	uint8_t* receivebuffer;
	std::stringstream ss;
	uint8_t internal_timecode;
	uint32_t latest_sentsize;
	CxxUtilities::Mutex sendmutex;
	CxxUtilities::Mutex receivemutex;
	SpaceWireIFActionTimecodeScynchronizedAction* timecodeaction;

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
	/** Constructor. */
	SpaceWireSSDTPModule(CxxUtilities::TCPSocket* newdatasocket) {
		datasocket = newdatasocket;
		sendbuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		receivebuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		internal_timecode = 0x00;
		latest_sentsize = 0;
		timecodeaction = NULL;
		rbuf_index = 0;
		receivedsize = 0;
		closed = false;
	}

public:
	/** Destructor. */
	~SpaceWireSSDTPModule() {
		if (sendbuffer != NULL) {
			free(sendbuffer);
		}
		if (receivebuffer != NULL) {
			free(receivebuffer);
		}
	}

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(std::vector<uint8_t>& data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		send(&data, eopType);
		sendmutex.unlock();
	}

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(std::vector<uint8_t>* data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		if(this->closed){
			sendmutex.unlock();
			return;
		}
		size_t size = data->size();
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = DataFlag_Complete_EOP;
		} else if (eopType == SpaceWireEOPMarker::EEP) {
			sheader[0] = DataFlag_Complete_EEP;
		} else if (eopType == SpaceWireEOPMarker::Continued) {
			sheader[0] = DataFlag_Flagmented;
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

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] the length length of the packet.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(uint8_t* data, size_t length, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		if(this->closed){
			sendmutex.unlock();
			return;
		}
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = DataFlag_Complete_EOP;
		} else if (eopType == SpaceWireEOPMarker::EEP) {
			sheader[0] = DataFlag_Complete_EEP;
		} else if (eopType == SpaceWireEOPMarker::Continued) {
			sheader[0] = DataFlag_Flagmented;
		}
		sheader[1] = 0x00;
		size_t asize = length;
		for (size_t i = 11; i > 1; i--) {
			sheader[i] = asize % 0x100;
			asize = asize / 0x100;
		}
		try {
			datasocket->send(sheader, 12);
			datasocket->send(data, length);
		} catch (...) {
			sendmutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
		}
		sendmutex.unlock();
	}

public:
	/** Tries to receive a pcket from the SpaceWire interface.
	 * This method will block the thread for a certain length of time.
	 * Timeout can happen via the TCPSocket provided to the instance.
	 * The code below shows how the timeout duration can be changed.
	 * @code
	 * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
	 * socket->open();
	 * socket->setTimeoutDuration(1000); //ms
	 * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
	 * try{
	 * 	ssdtpModule->receive();
	 * }catch(SpaceWireSSDTPException& e){
	 * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
	 * 	 std::cout << "Timeout" << std::endl;
	 * 	}else{
	 * 	 throw e;
	 * 	}
	 * }
	 * @endcode
	 * @returns packet content.
	 */
	std::vector<uint8_t> receive() throw (SpaceWireSSDTPException) {
		receivemutex.lock();
		if(this->closed){
			receivemutex.unlock();
			return {};
		}
		std::vector<uint8_t> data;
		uint32_t eopType;
		receive(&data, eopType);
		receivemutex.unlock();
		return data;
	}

public:
	/** Tries to receive a pcket from the SpaceWire interface.
	 * This method will block the thread for a certain length of time.
	 * Timeout can happen via the TCPSocket provided to the instance.
	 * The code below shows how the timeout duration can be changed.
	 * @code
	 * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
	 * socket->open();
	 * socket->setTimeoutDuration(1000); //ms
	 * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
	 * try{
	 * 	ssdtpModule->receive();
	 * }catch(SpaceWireSSDTPException& e){
	 * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
	 * 	 std::cout << "Timeout" << std::endl;
	 * 	}else{
	 * 	 throw e;
	 * 	}
	 * }
	 * @endcode
	 * @param[out] data a vector instance which is used to store received data.
	 * @param[out] eopType contains an EOP marker type (SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP).
	 */
	int receive(std::vector<uint8_t>* data, uint32_t& eopType) throw (SpaceWireSSDTPException) {
		size_t size = 0;
		size_t hsize = 0;
		size_t flagment_size = 0;
		size_t received_size = 0;

		try {
			using namespace std;
//		cout << "#1" << endl;
			receivemutex.lock();
			//header
			receive_header: //
			rheader[0] = 0xFF;
			rheader[1] = 0x00;
			while (rheader[0] != DataFlag_Complete_EOP && rheader[0] != DataFlag_Complete_EEP) {
//			cout << "#2" << endl;

//			cout << "#2-1" << endl;
				hsize = 0;
				flagment_size = 0;
				received_size = 0;
				//flag and size part
				try {
//				cout << "#2-2" << endl;
					while (hsize != 12) {
						if(this->closed){
							return 0;
						}
						if(this->receiveCanceled){
							//reset receiveCanceled
							this->receiveCanceled = false;
							//return with no data
							return 0;
						}
//					cout << "#2-3" << endl;
						long result = datasocket->receive(rheader + hsize, 12 - hsize);
						hsize += result;
//					cout << "#2-4" << endl;
					}
				} catch (CxxUtilities::TCPSocketException e) {
//				cout << "#2-5" << endl;
					if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
//					cout << "#2-6" << endl;
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
					} else {
//					cout << "#2-7" << endl;
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
					}
				} catch (...) {
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
				}

//			cout << "#3" << endl;
				//data or control code part
				if (rheader[0] == DataFlag_Complete_EOP || rheader[0] == DataFlag_Complete_EEP
						|| rheader[0] == DataFlag_Flagmented) {
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
						long result;
						_loop_receiveDataPart: //
						try {
							result = datasocket->receive(data_pointer + size + received_size, flagment_size - received_size);
						} catch (CxxUtilities::TCPSocketException e) {
							if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
								goto _loop_receiveDataPart;
							}
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
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
					}
					switch (rheader[0]) {
					case ControlFlag_SendTimeCode:
						internal_timecode = timecode_and_reserved[0];
						gotTimeCode(internal_timecode);
						break;
					case ControlFlag_GotTimeCode:
						internal_timecode = timecode_and_reserved[0];
						gotTimeCode(internal_timecode);
						break;
					}
//				cout << "#11" << endl;
				} else {
					cout << "SSDTP fatal error with flag value of 0x" << hex << (uint32_t) rheader[0] << dec << endl;
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
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
		} catch (SpaceWireSSDTPException& e) {
			receivemutex.unlock();
			throw e;
		} catch (CxxUtilities::TCPSocketException& e) {
			using namespace std;
//			cout << "#SpaceWireSSDTPModule::receive() caught TCPSocketException(" << e.toString() << ")" << endl;
			receivemutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
		}/* catch (...) {
		 using namespace std;
		 cout << "#SpaceWireSSDTPModule::receive() caught an unexpected exception" << endl;
		 cout << "Receive Header: ";
		 for (size_t i = 0; i < 12; i++) {
		 cout << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) rheader[i] << " ";
		 }
		 cout << endl;
		 cout << "size=" << size << endl;
		 cout << "hsize=" << hsize << endl;
		 cout << "flagment_size=" << flagment_size << endl;
		 cout << "received_size=" << received_size << endl;
		 receivemutex.unlock();
		 throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
		 }*/
	}

public:
	/** Emits a TimeCode.
	 * @param[in] timecode timecode value.
	 */
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
			datasocket->send(sendbuffer, 14);
			sendmutex.unlock();
		} catch (CxxUtilities::TCPSocketException& e) {
			sendmutex.unlock();
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
	}

public:
	/** Returns a time-code counter value.
	 * The time-code counter will be updated when a TimeCode is
	 * received from the SpaceWire interface. Receive of TimeCodes
	 * is not automatic, but performed in received() method silently.
	 * Therefore, to continuously receive TimeCodes, it is necessary to
	 * invoke receive() method repeatedly in, for example, a dedicated
	 * thread.
	 * @returns a locally stored TimeCode value.
	 */
	uint8_t getTimeCode() throw (SpaceWireSSDTPException) {
		return internal_timecode;
	}

public:
	/** Registers an action instance which will be invoked when a TimeCode is received.
	 * @param[in] SpaceWireIFActionTimecodeScynchronizedAction an action instance.
	 */
	void setTimeCodeAction(SpaceWireIFActionTimecodeScynchronizedAction* action) {
		timecodeaction = action;
	}

public:
	/** Sets link frequency.
	 * @attention This method is deprecated.
	 * @attention 4-port SpaceWire-to-GigabitEther does not provides this function since
	 * Link Speeds of external SpaceWire ports can be changed by updating
	 * dedicated registers available at Router Configuration port via RMAP.
	 */
	void setTxFrequency(double frequencyInMHz) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

public:
	/** An action method which is internally invoked when a TimeCode is received.
	 * This method is automatically called by SpaceWireSSDTP::receive() methods,
	 * and users do not need to use this method.
	 * @param[in] timecode a received TimeCode value.
	 */
	void gotTimeCode(uint8_t timecode) {
		using namespace std;
		if (timecodeaction != NULL) {
			timecodeaction->doAction(timecode);
		} else {
			/*	cout << "SSDTPModule::gotTimeCode(): Got TimeCode : " << hex << setw(2) << setfill('0')
			 << (uint32_t) timecode << dec << endl;*/
		}
	}

private:
	void registerRead(uint32_t address) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

private:
	void registerWrite(uint32_t address, std::vector<uint8_t> data) {
		//send command
		sendmutex.lock();
		sendbuffer[0] = ControlFlag_RegisterAccess_WriteCommand;
		sendmutex.unlock();
	}

public:
	/** @attention This method can be used only with 1-port SpaceWire-to-GigabitEther
	 * (i.e. open-source version of SpaceWire-to-GigabitEther running with the ZestET1 FPGA board).
	 * @param[in] txdivcount link frequency will be 200/(txdivcount+1) MHz.
	 */
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
		} catch (CxxUtilities::TCPSocketException& e) {
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
	/** Sends raw byte array via the socket.
	 * @attention Users should not use this method.
	 */
	void sendRawData(uint8_t* data, size_t length) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		try {
			datasocket->send(data, length);
		} catch (CxxUtilities::TCPSocketException& e) {
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
	/** Finalizes the instance.
	 * If another thread is waiting in receive(), it will return with 0 (meaning 0 byte received),
	 * enabling the thread to stop.
	 */
	void close(){
		this->closed=true;
	}

public:
	/** Cancels ongoing receive() method if any exist.
	 */
	void cancelReceive() {
		this->receiveCanceled = true;
	}

private:
	bool receiveCanceled = false;

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
