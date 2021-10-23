/*
 * ConsumerManager.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef CONSUMERMANAGER_HH_
#define CONSUMERMANAGER_HH_

#include "../RMAPHandler.hh"

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManager {
public:
	//Addresses of Consumer Manager Module
	static const uint32_t InitialAddressOf_ConsumerMgr = 0x01010000;
	static const uint32_t ConsumerMgrBA = InitialAddressOf_ConsumerMgr;
	static const uint32_t AddressOf_DisableRegister = ConsumerMgrBA + 0x0100;
	static const uint32_t AddressOf_ReadPointerRegister_High = ConsumerMgrBA + 0x0102;
	static const uint32_t AddressOf_ReadPointerRegister_Low = ConsumerMgrBA + 0x0104;
	static const uint32_t AddressOf_WritePointerRegister_High = ConsumerMgrBA + 0x0106;
	static const uint32_t AddressOf_WritePointerRegister_Low = ConsumerMgrBA + 0x0108;
	static const uint32_t AddressOf_GuardBitRegister = ConsumerMgrBA + 0x010a;
	static const uint32_t AddressOf_AddressUpdateGoRegister = ConsumerMgrBA + 0x010c;
	static const uint32_t AddressOf_GateSize_FastGate_Register = ConsumerMgrBA + 0x010e;
	static const uint32_t AddressOf_GateSize_SlowGate_Register = ConsumerMgrBA + 0x0110;
	static const uint32_t AddressOf_NumberOf_BaselineSample_Register = ConsumerMgrBA + 0x0112;
	static const uint32_t AddressOf_ConsumerMgr_ResetRegister = ConsumerMgrBA + 0x0114;
	static const uint32_t AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
	static const uint32_t AddressOf_Writepointer_Semaphore_Register = ConsumerMgrBA + 0x0118;
	//Addresses of SDRAM
	static const uint32_t InitialAddressOf_Sdram_EventList = 0x00000000;
	static const uint32_t FinalAddressOf_Sdram_EventList = 0x00fffffe;

private:
	RMAPHandler* rmapHandler_;
	RMAPTargetNode* adcboxRMAPNode;
	SemaphoreRegister* writePointerSemaphore_;

private:
	//variables for ring buffer control
	uint32_t readPointer_, writePointer_;
	uint32_t nextReadFrom_;
	uint32_t guardBit_;

public:
	/** Constructor.
	 * @param rmaphandler a pointer to RMAPHandler which is connected to SpaceWire ADC Box
	 */
	ConsumerManager(RMAPHandler* handler) {
		this->rmapHandler_ = handler;
		adcboxRMAPNode = handler->getRMAPTargetNode("ADCBox");
		writePointerSemaphore_ = new SemaphoreRegister(this->rmapHandler_,
				ConsumerManager::AddressOf_Writepointer_Semaphore_Register);
		initialize();
	}

public:
	virtual  ~ConsumerManager(){}

public:
	/** Initialize internal variables.
	 */
	virtual  void initialize() {
		//ringbuffer pointers
		readPointer_ = 0;
		writePointer_ = 0;
		nextReadFrom_ = 0;
		guardBit_ = 0;
	}

public:
	/** Returns WritePointer which is managed by ConsumerManager module (on FPGA).
	 * The "value of the time" of the read access, and therefore, the pointer
	 * might be updated by ConsumerManager logic module (on FPGA).
	 * @return address of SDRAM which corresponds to WritePointer of the ring buffer
	 */
	virtual  uint32_t getWritePointer() {
		using namespace std;
		uint32_t writepointer;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::getWritePointer()...";
		}
		//semaphore request
		writePointerSemaphore_->request();
		std::vector<uint8_t> readdata;
		readdata.resize(4, 0);
		rmapHandler_->read(adcboxRMAPNode, AddressOf_WritePointerRegister_High, 4, &readdata[0]);
		writepointer = readdata[1] * 0x01000000 + readdata[0] * 0x00010000 + readdata[3] * 0x00000100
				+ readdata[2] * 0x00000001;
		writepointer += ConsumerManager::InitialAddressOf_Sdram_EventList;
		//semaphore release
		writePointerSemaphore_->release();

		if (Debug::consumermanager()) {
			cout << "done" << endl;
		}

		return writepointer;
	}

public:
	/** Returns ReadPointer maintained inside this module.
	 * @return address of SDRAM which corresponds to ReadPointer of the ring buffer.
	 */
	virtual  uint32_t getReadPointer() {
		return readPointer_;
	}

public:
	/** Sets ReadPointer which is managed by ConsumerManager module (on FPGA).
	 * @param readpointer address of SDRAM which corresponds to ReadPointer of the ring buffer
	 */
	virtual  uint32_t setReadPointer(uint32_t readpointer) {
		using namespace std;
		uint32_t addressH[2], addressL[2];
		uint32_t readpointer_tmp;
		vector<uint8_t> writedata;

		if (Debug::consumermanager()) {
			cout << "ConsumerManager::setReadPointer()...";
			std::vector<uint8_t> readdata;
			readdata.resize(4, 0);
			rmapHandler_->read(adcboxRMAPNode, AddressOf_ReadPointerRegister_High, 4, &readdata[0]);
			readpointer_tmp = readdata[1] * 0x01000000 + readdata[0] * 0x00010000 + readdata[3] * 0x0100
					+ readdata[2] * 0x0001;
		}
		readpointer -= InitialAddressOf_Sdram_EventList;
		addressH[0] = (readpointer << 8 >> 24);
		addressH[1] = (readpointer >> 24);
		addressL[0] = (readpointer << 24 >> 24);
		addressL[1] = (readpointer << 16 >> 24);
		readpointer += InitialAddressOf_Sdram_EventList;

		writedata.push_back(addressH[0]);
		writedata.push_back(addressH[1]);
		writedata.push_back(addressL[0]);
		writedata.push_back(addressL[1]);
		rmapHandler_->write(adcboxRMAPNode, AddressOf_ReadPointerRegister_High, &writedata[0], 4);

		writedata.clear();
		writedata.push_back(0x01);
		writedata.push_back(0x02);
		rmapHandler_->write(adcboxRMAPNode, AddressOf_AddressUpdateGoRegister, &writedata[0], 2);

		if (Debug::consumermanager()) {
			cout << "done" << endl;
			cout << "readpointer:" << setfill('0') << setw(8) << hex << readpointer_tmp << "->" << setfill('0') << setw(8)
					<< hex << readpointer << endl;
		}
		return 1;
	}

public:
	/** Returns GuardBit value which is managed by ConsumerManager module (on FPGA).
	 * @return guard bit value
	 */
	virtual  uint32_t getGuardBit() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::getGuardBit()...";
		}
		std::vector<uint8_t> readdata;
		readdata.resize(2, 0);
		rmapHandler_->read(adcboxRMAPNode, AddressOf_GuardBitRegister, 2, &readdata[0]);
		uint32_t guardbit = readdata[0];

		if (Debug::consumermanager()) {
			cout << "guard bit=" << guardbit << endl;
		}
		return guardbit;
	}

public:
	/** Resets ConsumerManager module.
	 */
	virtual  void reset() {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::reset()...";
		}
		vector<uint8_t> writedata;
		writedata.push_back(0x01);
		writedata.push_back(0x00);
		rmapHandler_->write(adcboxRMAPNode, AddressOf_ConsumerMgr_ResetRegister, &writedata[0], 2);
		if (Debug::consumermanager()) {
			cout << "done" << endl;
		}
		initialize();
	}

public:
	/** Retrieve data stored in the SDRAM (via ConsumerManager module).
	 * The size of data varies depending on the event data in the SDRAM.
	 * @param maximumsize maximum data size to be returned (in bytes)
	 */
	virtual  std::vector<uint8_t> read(uint32_t maximumsize = 4000) {
		using namespace std;
		//vector to be returned
		vector<uint8_t> returneddata;

		//temporary addresses
		uint32_t addressFrom, addressTo;

		//if readpointer==writepointer, then update writepointer.
		if (readPointer_ == writePointer_) {
			if (Debug::ringbuffer()) {
				cout << "ConsumerManager::read() writepointer is updated" << endl;
			}
			writePointer_ = getWritePointer();
			if (Debug::ringbuffer()) {
				cout << "ConsumerManager::read() guardbit is updated" << endl;
			}
			guardBit_ = getGuardBit();
		} else {
//			cout << "Catching up.." << endl;
		}

		//read data stored in ring buffer
		if (Debug::ringbuffer()) {
			cout << "nextreadfrom:" << setfill('0') << setw(8) << hex << nextReadFrom_ << endl;
			cout << "writepointer:" << setfill('0') << setw(8) << hex << writePointer_ << endl;
			cout << "guardbit:" << guardBit_ << endl;
		}

		if (writePointer_ <= nextReadFrom_ && guardBit_ == 0) {
			//there is no data to be read out from SDRAM
//			cout << "NO DATA" << endl;
		} else if (writePointer_ >= nextReadFrom_) {
			//there is still remaining data to be read out
			if (Debug::ringbuffer()) {
				cout << "read mode : (writepointer>=nextreadfrom)" << endl;
			}
			addressFrom = (nextReadFrom_);
			if (nextReadFrom_ + maximumsize < writePointer_) {
				addressTo = (nextReadFrom_ + maximumsize);
				readPointer_ = addressTo;
			} else {
				addressTo = (writePointer_);
				readPointer_ = writePointer_;
			}
			if (Debug::ringbuffer()) {
				cout << setfill('0') << setw(8) << hex << addressFrom << "-" << setfill('0') << setw(8) << hex << addressTo
						<< endl;
			}
			returneddata.clear();
			returneddata.resize((addressTo - addressFrom + 2), 0);
//			cout << "SOME DATA..." << hex << addressFrom << " " <<  (addressTo - addressFrom + 2);
			try {
				rmapHandler_->read(adcboxRMAPNode, addressFrom, (addressTo - addressFrom + 2), &returneddata[0]);
//				cerr << hex << (uint32_t)addressFrom << " " << (uint32_t)(addressTo - addressFrom + 2) << endl;
				nextReadFrom_ = increment_address(readPointer_);
			} catch (...) {
				cerr << "Reading 0 banchi" << endl;
				rmapHandler_->read(adcboxRMAPNode, 0, 2, &returneddata[0]);
				cerr << hex << (uint32_t) returneddata[0] << " " << (uint32_t) returneddata[1] << endl;
				cerr << "Could not retrieve... Try later." << endl;
				rmapHandler_->read(adcboxRMAPNode, addressFrom, 2, &returneddata[0]);
				cerr << hex << (uint32_t) addressFrom << " " << (uint32_t) (addressTo - addressFrom + 2) << endl;
			}
//			cout << "DONE";
		} else {
			if (Debug::ringbuffer()) {
				cout << "read mode : (writepointer<nextreadfrom && guardbit==1)" << endl;
			}
			addressFrom = (nextReadFrom_);
			if (nextReadFrom_ + maximumsize < FinalAddressOf_Sdram_EventList) {
				addressTo = (nextReadFrom_ + maximumsize);
				readPointer_ = addressTo;
			} else {
				addressTo = (FinalAddressOf_Sdram_EventList);
				readPointer_ = FinalAddressOf_Sdram_EventList;
			}
			if (Debug::ringbuffer()) {
				cout << setfill('0') << setw(8) << hex << addressFrom << "-" << setfill('0') << setw(8) << hex << addressTo
						<< endl;
			}
//			returneddata = *rmaphandler->read(adcboxRMAPNode, addressFrom, (addressTo - addressFrom + 2));
			try {
//				cout << "rmaphandler->read()ing." << endl;
				returneddata.resize((addressTo - addressFrom + 2), 0);
				rmapHandler_->read(adcboxRMAPNode, addressFrom, (addressTo - addressFrom + 2), &returneddata[0]);
			} catch (...) {
				cout << "caught error when rmaphandler->read()ing." << endl;
			}
			nextReadFrom_ = increment_address(readPointer_);
		}
		//read data stored in ring buffer
		if (Debug::ringbuffer()) {
			cout << "nextreadfrom:" << setfill('0') << setw(8) << hex << nextReadFrom_ << endl;
			cout << "writepointer:" << setfill('0') << setw(8) << hex << writePointer_ << endl;
			cout << "guardbit:" << guardBit_ << endl;
			cout.flush();
		}
		if ((returneddata.size() != 0 && readPointer_ == writePointer_) || readPointer_ == FinalAddressOf_Sdram_EventList) {
			setReadPointer(readPointer_);
		}
		return returneddata;
	}

public:
	/** Sets NumberOfBaselineSample_Register
	 * @param numberofsamples number of data points to be sampled
	 */
	virtual  void setNumberOfBaselineSamples(uint32_t numberofsamples) {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::setNumberOfBaselineSamples(" << numberofsamples << ")...";
		}
		vector<uint8_t> writedata;
		writedata.push_back((uint8_t) numberofsamples);
		writedata.push_back(0x00);
		rmapHandler_->write(adcboxRMAPNode, AddressOf_NumberOf_BaselineSample_Register, &writedata[0], 2);

		if (Debug::consumermanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets EventPacket_NumberOfWaveform_Register
	 * @param numberofsamples number of data points to be recorded in an event packet
	 */
	virtual  void setEventPacket_NumberOfWaveform(uint32_t numberofsamples) {
		using namespace std;
		if (Debug::consumermanager()) {
			cout << "ConsumerManager::setEventPacket_NumberOfWaveform(" << numberofsamples << ")...";
		}
		vector<uint8_t> writedata;
		writedata.push_back((uint8_t) (numberofsamples << 24 >> 24));
		writedata.push_back((uint8_t) (numberofsamples << 16 >> 24));
		rmapHandler_->write(adcboxRMAPNode, AddressOf_EventPacket_NumberOfWaveform_Register, &writedata[0], 2);

		if (Debug::consumermanager()) {
			cout << "done" << endl;
			std::vector<uint8_t> readdata;
			readdata.resize(2, 0);
			rmapHandler_->read(adcboxRMAPNode, AddressOf_EventPacket_NumberOfWaveform_Register, 2, &readdata[0]);
			cout << "readdata[0]:" << (uint32_t) readdata[0] << endl;
		}
	}

private:
	virtual  uint32_t increment_address(uint32_t address) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "increment_address(" << hex << setw(8) << setfill('0') << address << dec << ")...";
		}
		if (address == FinalAddressOf_Sdram_EventList) {
			address = InitialAddressOf_Sdram_EventList;
		} else {
			address += 2;
		}
		if (Debug::channelmanager()) {
			cout << "done(" << hex << setw(8) << setfill('0') << address << dec << ")" << endl;
		}
		return address;
	}
};

#endif /* CONSUMERMANAGER_HH_ */
