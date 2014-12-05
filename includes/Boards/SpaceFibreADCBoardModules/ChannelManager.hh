/*
 * ChannelManager.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef CHANNELMANAGER_HH_
#define CHANNELMANAGER_HH_

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Types.hh"

/** A class which represents ChannelManager module on VHDL logic.
 * This module controls start/stop, preset mode, livetime, and
 * realtime of data acquisition.
 */
class ChannelManager {
public:
	//Addresses of Channel Manager Module
	static const uint32_t InitialAddressOf_ChMgr = 0x01010000;
	static const uint32_t ChMgrBA = InitialAddressOf_ChMgr; //Base Address of ChMgr
	static const uint32_t AddressOf_StartStopRegister = ChMgrBA + 0x0002;
	static const uint32_t AddressOf_StartStopSemaphoreRegister = ChMgrBA + 0x0004;
	static const uint32_t AddressOf_PresetModeRegister = ChMgrBA + 0x0006;
	static const uint32_t AddressOf_PresetLivetimeRegisterL = ChMgrBA + 0x0008;
	static const uint32_t AddressOf_PresetLivetimeRegisterH = ChMgrBA + 0x000a;
	static const uint32_t AddressOf_RealtimeRegisterL = ChMgrBA + 0x000c;
	static const uint32_t AddressOf_RealtimeRegisterM = ChMgrBA + 0x000e;
	static const uint32_t AddressOf_RealtimeRegisterH = ChMgrBA + 0x0010;
	static const uint32_t AddressOf_ResetRegister = ChMgrBA + 0x0012;
	static const uint32_t AddressOf_ADCClock_Register = ChMgrBA + 0x0014;
	static const uint32_t AddressOf_PresetnEventsRegisterL = ChMgrBA + 0x0020;
	static const uint32_t AddressOf_PresetnEventsRegisterH = ChMgrBA + 0x0022;
	static const uint32_t NumberOfChannels = 8;

public:
	static constexpr double LivetimeCounterInterval = 1e-2; //s (=10ms)

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;
	SemaphoreRegister* startStopSemaphore;

public:
	/** Constructor.
	 * @param rmapHandler a pointer to RMAPHandler which is connected to SpaceWire ADC Box
	 */
	ChannelManager(RMAPHandler* handler, RMAPTargetNode* adcRMAPTargetNode) {
		this->rmapHandler = handler;
		this->adcRMAPTargetNode = adcRMAPTargetNode;
		startStopSemaphore = new SemaphoreRegister(this->rmapHandler, this->adcRMAPTargetNode,
				ChannelManager::AddressOf_StartStopSemaphoreRegister);
	}

public:
	/** Starts data acquisition. Configuration of registers of
	 * individual channels should be completed before invoking this method.
	 * @param channelsToBeStarted vector of bool, true if the channel should be started
	 */
	void startAcquisition(std::vector<bool> channelsToBeStarted) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::startAcquisition()...";
		}
		//prepare register value for StartStopRegister
		vector<uint8_t> writeData = { 0x00, 0x00 };
		uint8_t tmp = 1;
		for (uint32_t i = 0; i < channelsToBeStarted.size(); i++) {
			//check if channel i should be started.
			if (channelsToBeStarted.at(i) == true) {
				writeData[1] = writeData[1] + tmp;
			}
			tmp = tmp * 2;
		}
		//write the StartStopRegister (semaphore is needed)
		startStopSemaphore->request();
		rmapHandler->write(adcRMAPTargetNode, ChannelManager::AddressOf_StartStopRegister, &writeData[0], writeData.size());
		startStopSemaphore->release();
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Checks if all data acquisition is completed in all channels.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted() {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::isAcquisitionCompleted()...";
		}
		std::vector<uint8_t> readData(2);
		rmapHandler->read(adcRMAPTargetNode, ChannelManager::AddressOf_StartStopRegister, 2, &readData[0]);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
		if (readData[0] == 0x00 && readData[1] == 0x00) {
			return true;
		} else {
			return false;
		}
	}

public:
	/** Checks if data acquisition of single channel is completed.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted(size_t chNumber) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::isAcquisitionCompleted(" << chNumber << ")...";
		}
		std::vector<uint8_t> readData(2);
		rmapHandler->read(adcRMAPTargetNode, ChannelManager::AddressOf_StartStopRegister, 2, &readData[0]);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
		if (chNumber < NumberOfChannels) {
			std::bitset<16> bits(readData[0] * 0x100 + readData[1]);
			if (bits[chNumber] == 0) {
				return true;
			} else {
				return false;
			}
		} else {
			//if chNumber is out of the allowed range
			return false;
		}
	}

public:
	/** Stops data acquisition regardless of the preset mode of
	 * the acquisition.
	 */
	void stopAcquisition() {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::stopAcquisition()...";
		}
		vector<uint8_t> writeData = { 0x00, 0x00 };
		startStopSemaphore->request();
		rmapHandler->write(adcRMAPTargetNode, ChannelManager::AddressOf_StartStopRegister, &writeData[0], 2);
		startStopSemaphore->release();
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets Preset Mode.
	 * Available preset modes are:<br>
	 * PresetMode::Livetime<br>
	 * PresetMode::Number of Event<br>
	 * PresetMode::NonStop (Forever)
	 * @param presetMode preset mode value (see also enum class PresetMode )
	 */
	void setPresetMode(SpaceFibreADC::PresetMode presetMode) {
		using namespace std;
		uint32_t presetmode = static_cast<uint32_t>(presetMode);
		if (Debug::channelmanager()) {
			cout << "ChannelManager::setPresetMode()...";
		}
		vector<uint8_t> writeData = { 0x00, static_cast<uint8_t>(presetmode) };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_PresetModeRegister, &writeData[0], 2);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets ADC Clock.
	 * - ADCClockFrequency::ADCClock200MHz <br>
	 * - ADCClockFrequency::ADCClock100MHz <br>
	 * - ADCClockFrequency::ADCClock50MHz <br>
	 * @param adcClockFrequency enum class ADCClockFrequency
	 */
	void setAdcClock(SpaceFibreADC::ADCClockFrequency adcClockFrequency) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::setAdcClock(" << static_cast<uint16_t>(adcClockFrequency) / 100 << "MHz)...";
		}
		rmapHandler->setRegister(AddressOf_ADCClock_Register, static_cast<uint16_t>(adcClockFrequency));
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets Livetime preset value.
	 * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
	 */
	void setPresetLivetime(uint32_t livetimeIn10msUnit) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::setLivetime(" << livetimeIn10msUnit << ")...";
		}
		vector<uint8_t> writeData;
		writeData.push_back(livetimeIn10msUnit << 16 >> 24);
		writeData.push_back(livetimeIn10msUnit << 24 >> 24);
		rmapHandler->write(adcRMAPTargetNode, AddressOf_PresetLivetimeRegisterL, &writeData[0], 2);
		writeData.clear();
		writeData.push_back(livetimeIn10msUnit >> 24);
		writeData.push_back(livetimeIn10msUnit << 8 >> 24);
		rmapHandler->write(adcRMAPTargetNode, AddressOf_PresetLivetimeRegisterH, &writeData[0], 2);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Get Realtime which is elapsed time since the data acquisition was started.
	 * @return elapsed real time in 10ms unit
	 */
	double getRealtime() {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::getRealtime()...";
		}
		std::vector<uint8_t> readData(6);
		rmapHandler->read(adcRMAPTargetNode, AddressOf_RealtimeRegisterL, 6, &readData[0]);
		double low = (double) (readData[0]) * 0x100 + (double) (readData[1]);
		double mid = (double) (readData[2]) * 0x1000000 + (double) (readData[3]) * 0x10000;
		double high = ((double) (readData[4]) * 0x1000000 + (double) (readData[5]) * 0x10000) * 0x10000;
		double realtime = low + mid + high;
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
		return realtime;
	}

public:
	/** Resets ChannelManager module on VHDL logic.
	 * This method clear all internal registers in the logic module.
	 */
	void reset() {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::reset()...";
		}
		vector<uint8_t> writeData = { 0x00, 0x01 };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_ResetRegister, &writeData[0], 2);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets PresetnEventsRegister.
	 * @param nEvents number of event to be taken
	 */
	void setPresetnEvents(uint32_t nEvents) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::setPresetnEvents(" << nEvents << ")...";
		}
		vector<uint8_t> writeData = { //
				static_cast<uint8_t>(nEvents << 16 >> 24), //
						static_cast<uint8_t>(nEvents << 24 >> 24), //
						static_cast<uint8_t>(nEvents >> 24), //
						static_cast<uint8_t>(nEvents << 8 >> 24) //
				};
		rmapHandler->write(adcRMAPTargetNode, AddressOf_PresetLivetimeRegisterL, &writeData[0], 4);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets ADC clock counter.<br>
	 * ADC Clock = 50MHz/(ADC Clock Counter+1)<br>
	 * example:<br>
	 * ADC Clock 50MHz when ADC Clock Counter=0<br>
	 * ADC Clock 25MHz when ADC Clock Counter=1<br>
	 * ADC Clock 10MHz when ADC Clock Counter=4
	 * @param adcClockCounter ADC Clock Counter value
	 */
	void setadcClockCounter(uint32_t adcClockCounter) {
		using namespace std;
		if (Debug::channelmanager()) {
			cout << "ChannelManager::setadcClockCounter(" << adcClockCounter << ")...";
		}
		vector<uint8_t> writeData = { //
				static_cast<uint8_t>(adcClockCounter / 0x100), //
						static_cast<uint8_t>(adcClockCounter % 0x100) //
				};
		rmapHandler->write(adcRMAPTargetNode, AddressOf_PresetLivetimeRegisterL, &writeData[0], 2);
		if (Debug::channelmanager()) {
			cout << "done" << endl;
		}
	}
};

#endif /* CHANNELMANAGER_HH_ */
