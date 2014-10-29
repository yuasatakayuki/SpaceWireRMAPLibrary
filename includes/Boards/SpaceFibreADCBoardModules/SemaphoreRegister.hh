/*
 * SemaphoreRegister.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef SEMAPHOREREGISTER_HH_
#define SEMAPHOREREGISTER_HH_

#include "Boards/SpaceFibreADCBoardModules/Types.hh"
#include "Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"

/** An internal class which represents a semaphore in
 * the VHDL logic.
 */
class SemaphoreRegister {
private:
	RMAPHandler* rmaphandler;
	RMAPTargetNode* adcboxRMAPNode;
	uint32_t address;

public:
	/** Constructor.
	 * @param rmaphandler instance of RMAPHandler which is used for this class to communicate SpaceWire ADC Box
	 * @param address an address of which semaphore this instance should handle
	 */
	SemaphoreRegister(RMAPHandler* handler, RMAPTargetNode* adcboxRMAPNode, uint32_t address) {
		using namespace std;
		this->rmaphandler = handler;
		this->address = address;
		this->adcboxRMAPNode = adcboxRMAPNode;
		if (Debug::semaphore()) {
			cout << "READING..." << endl;
			std::vector<uint8_t> data;
			data.push_back(0);
			data.push_back(0);
			rmaphandler->read(adcboxRMAPNode, address, 2, &data[0]);
			cout << "semaphore (" << hex << address << ") was created, and value is " << data[1] * 0x100 + data[0] << endl
					<< dec;
		}
		/* 20131214 comment out
		 request();
		 release();
		 */
	}

	/** Requests the semaphore. This method will wait for
	 * infinitely until it successfully gets the semaphore.
	 */
	void request() {
		using namespace std;
		if (Debug::semaphore()) {
			cout << " request semaphore(" << hex << setw(8) << setfill('0') << address << ")...";
		}

		bool flag = true;
		do {
			vector<uint8_t> writeData = { 0xff, 0xff };
			rmaphandler->write(adcboxRMAPNode, address, &writeData[0], 2);

			vector<uint8_t> readData = { 0x00, 0x00 };
			rmaphandler->read(adcboxRMAPNode, address, 2, &readData[0]);
			if (readData[0] != 0x00) {
				flag = false;
			}
		} while (flag);

		if (Debug::semaphore()) {
			cout << "got" << endl;
		}
	}

	/** Release the semaphore.
	 */
	void release() {
		using namespace std;
		if (Debug::semaphore()) {
			cout << " release semaphore(" << hex << setw(4) << setfill('0') << address << ")...";
		}
		vector<uint8_t> writeData = { 0x00, 0x00 };
		rmaphandler->write(adcboxRMAPNode, address, &writeData[0], 2);

		if (Debug::semaphore()) {
			cout << "released" << endl;
		}
	}
};

#endif /* SEMAPHOREREGISTER_HH_ */
