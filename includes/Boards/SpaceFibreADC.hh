/*
 * SpaceFibreADCBoard.hh
 *
 *  Created on: May 15, 2009
 *      Author: yuasa
 */

#ifndef SPACEFIBREADCBOARD_HH_
#define SPACEFIBREADCBOARD_HH_

#include "Boards/SpaceFibreADCBoardModules/Constants.hh"
#include "Boards/SpaceFibreADCBoardModules/Debug.hh"
#include "Boards/SpaceFibreADCBoardModules/SemaphoreRegister.hh"
#include "Boards/SpaceFibreADCBoardModules/ConsumerManagerSocketFIFO.hh"
#include "Boards/SpaceFibreADCBoardModules/EventDecoder.hh"
#include "Boards/SpaceFibreADCBoardModules/ChannelModule.hh"
#include "Boards/SpaceFibreADCBoardModules/ChannelManager.hh"
#include "Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"

enum class SpaceFibreADCException {
	InvalidChannelNumber, OpenDeviceFailed, CloseDeviceFailed,
};

/** A class which represents SpaceFibre ADC Board.
 * It controls start/stop of data acquisition.
 * It contains SpaceFibreADCBoard::NumberOfChannels instances of
 * ADCChannel class so that
 * a user can change individual registers in each module.
 */
class SpaceFibreADCBoard {
public:
	//Number of channelModules which SpaceFibre ADC Board has.
	static const int NumberOfChannels = 4;

	//Clock Frequency
	static constexpr double ClockFrequency = 50; //MHz

	//Clock Interval
	static constexpr double ClockInterval = 20e-9; //s

	//SpaceWire-to-GigabitEther RMAP TCP/IP Port number
	static const uint32_t TCPPortNumberForRMAPOverTCP = 10032;

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;
	ChannelManager* channelManger;
	ConsumerManagerSocketFIFO* consumerManager;
	ChannelModule* channelModules[NumberOfChannels];
	EventDecoder* eventDecoder;

public:
	/** Constructor.
	 * @param ipAddress IP address of the SpaceFibre ADC board
	 * @param tcpPortNumber TCP/IP Port number
	 */
	SpaceFibreADCBoard(std::string ipAddress) {
		using namespace std;
		this->rmapHandler = new RMAPHandler(ipAddress, TCPPortNumberForRMAPOverTCP);

		//construct RMAPTargetNode instance
		adcRMAPTargetNode = new RMAPTargetNode;
		adcRMAPTargetNode->setID("ADCBox");
		adcRMAPTargetNode->setDefaultKey(0x00);
		adcRMAPTargetNode->setReplyAddress( { });
		adcRMAPTargetNode->setTargetSpaceWireAddress( { });
		adcRMAPTargetNode->setTargetLogicalAddress(0xFE);
		adcRMAPTargetNode->setInitiatorLogicalAddress(0xFE);

		cout << "---------------------------------------------" << endl;
		cout << "RMAPTargetNode definition" << endl;
		cout << adcRMAPTargetNode->toString() << endl;

		//create an instance of ChannelManager
		channelManger = new ChannelManager(rmapHandler, adcRMAPTargetNode);

		//create an instance of ConsumerManager
		consumerManager = new ConsumerManagerSocketFIFO(ipAddress, rmapHandler, adcRMAPTargetNode);

		//create instances of ADCChannelRegister
		for (size_t i = 0; i < SpaceFibreADCBoard::NumberOfChannels; i++) {
			channelModules[i] = new ChannelModule(rmapHandler, adcRMAPTargetNode, i);
		}

		//event decoder
		eventDecoder = new EventDecoder();
	}

public:
	/** Returns a corresponding pointer ChannelModule.
	 * @param chNumber Channel number that should be returned.
	 * @return a pointer to an instance of ChannelModule.
	 */
	ChannelModule* getChannelRegister(int chNumber) {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "SpaceFibreADCBoard::getChannelRegister(" << chNumber << ")" << endl;
		}
		return channelModules[chNumber];
	}

public:
	/** Returns a pointer to ChannelManager.
	 * @return a pointer to ChannelManager.
	 */
	ChannelManager* getChannelManager() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "SpaceFibreADCBoard::getChannelManager()" << endl;
		}
		return channelManger;
	}

public:
	/** Returns a pointer to ConsumerManager.
	 * @return a pointer to ConsumerManager.
	 */
	ConsumerManagerSocketFIFO* getConsumerManager() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "SpaceFibreADCBoard::getConsumerManager()" << endl;
		}
		return consumerManager;
	}

public:
	/** Reset ChannelManager and ConsumerManager modules on
	 * VHDL.
	 */
	void reset() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "SpaceFibreADCBoard::reset()";
		}
		this->channelManger->reset();
		this->consumerManager->reset();
		if (Debug::adcbox()) {
			cout << "done" << endl;
		}
	}

public:
	/** Returns RMAPHandler instance which is
	 * used in this instance.
	 * @return an instance of RMAPHandler used in this instance
	 */
	RMAPHandler* getRMAPHandler() {
		return rmapHandler;
	}

	//=============================================
	// device-wide functions

public:
	/** Open the device by making a
	 * GigabitEthernet connection to SpaceFibre ADC board.
	 */
	void openDevice() throw (SpaceFibreADCException) {
		try {
			rmapHandler->connectoToSpaceWireToGigabitEther();
			consumerManager->openSocket();
		} catch (...) {
			throw SpaceFibreADCException::OpenDeviceFailed;
		}
	}

public:
	/** Closes the device.
	 */
	void closeDevice() {
		try {
			rmapHandler->disconnectSpWGbE();
			consumerManager->closeSocket();
		} catch (...) {
			throw SpaceFibreADCException::CloseDeviceFailed;
		}
	}

	//=============================================
public:
	std::vector<SpaceFibreADC::Event*> getEvent() {
		std::vector<SpaceFibreADC::Event*> events;
		std::vector<uint8_t> data = consumerManager->getEventData();
		if (data.size() != 0) {
			eventDecoder->decodeEvent(&data);
			events = eventDecoder->getDecodedEvents();
		}
		return events;
	}

public:
	/** Frees an event instance so that buffer area can be reused in the following commands.
	 * @param event event instance to be freed
	 */
	void freeEvent(SpaceFibreADC::Event* event) {
		eventDecoder->freeEvent(event);
	}

public:
	/** Frees event instances so that buffer area can be reused in the following commands.
	 * @param events a vector of event instance to be freed
	 */
	void freeEvents(std::vector<SpaceFibreADC::Event*>& events) {
		for (auto event : events) {
			eventDecoder->freeEvent(event);
		}
	}

//=============================================
public:
	/** Set trigger mode of the specified channel.
	 * SpaceFibreADC::TriggerMode;<br>
	 * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
	 * Common Gate In = CommonGateIn<br>
	 * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
	 * CPU Trigger = CPUTrigger<br>
	 * Trigger Bus (OR) = TriggerBusSelectedOR<br>
	 * Trigger Bus (AND) = TriggerBusSelectedAND<br>
	 * @param chNumber channel number
	 * @param triggerMode trigger mode (see SpaceFibreADC::TriggerMode)
	 */
	void setTriggerMode(size_t chNumber, SpaceFibreADC::TriggerMode triggerMode) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->setTriggerMode(triggerMode);
		} else {
			using namespace std;
			cerr << "Error in setTriggerMode(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets number of samples after trigger.
	 * This method sets the same number to all the channels.
	 * @param nSamples number of adc samples per one waveform data
	 */
	void setNumberOfSamples(uint32_t nSamples) {
		for (size_t i = 0; i < NumberOfChannels; i++) {
			channelModules[i]->setNumberOfSamples(nSamples);
		}
	}

public:
	/** Sets Leading Trigger Threshold.
	 * @param threshold an adc value for leading trigger threshold
	 */
	void setStartingThreshold(size_t chNumber, uint32_t threshold) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->setStartingThreshold(threshold);
		} else {
			using namespace std;
			cerr << "Error in setStartingThreshold(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets Trailing Trigger Threshold.
	 * @param threshold an adc value for trailing trigger threshold
	 */
	void setClosingThreshold(size_t chNumber, uint32_t threshold) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->setClosingThreshold(threshold);
		} else {
			using namespace std;
			cerr << "Error in setClosingThreshold(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets TriggerBusMask which is used in TriggerMode==TriggerBus.
	 * @param enabledChannels array of enabled trigger bus channels.
	 */
	void setTriggerBusMask(size_t chNumber, std::vector<size_t> enabledChannels) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->setTriggerBusMask(enabledChannels);
		} else {
			using namespace std;
			cerr << "Error in setTriggerBusMask(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets depth of delay per trigger. When triggered,
	 * a waveform will be recorded starting from N steps
	 * before of the trigger timing.
	 * @param depthOfDelay number of samples retarded
	 */
	void setDepthOfDelay(size_t chNumber, uint32_t depthOfDelay) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->setDepthOfDelay(depthOfDelay);
		} else {
			using namespace std;
			cerr << "Error in setDepthOfDelay(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Gets Livetime.
	 * @return elapsed livetime in 10ms unit
	 */
	uint32_t getLivetime(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			return channelModules[chNumber]->getLivetime();
		} else {
			using namespace std;
			cerr << "Error in getLivetime(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Get current ADC value.
	 * @return temporal ADC value
	 */
	uint32_t getCurrentADCValue(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			return channelModules[chNumber]->getCurrentADCValue();
		} else {
			using namespace std;
			cerr << "Error in getCurrentADCValue(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Turn on ADC.
	 */
	void turnOnADCPower(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->turnADCPower(true);
		} else {
			using namespace std;
			cerr << "Error in turnOnADCPower(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Turn on ADC.
	 */
	void turnOffADCPower(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->turnADCPower(false);
		} else {
			using namespace std;
			cerr << "Error in turnOffADCPower(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

//=============================================

public:
	/** Starts data acquisition. Configuration of registers of
	 * individual channels should be completed before invoking this method.
	 * @param channelsToBeStarted vector of bool, true if the channel should be started
	 */
	void startAcquisition(std::vector<bool> channelsToBeStarted) {
		channelManger->startAcquisition(channelsToBeStarted);
	}

public:
	/** Checks if all data acquisition is completed in all channels.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted() {
		return channelManger->isAcquisitionCompleted();
	}

public:
	/** Checks if data acquisition of single channel is completed.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted(size_t chNumber) {
		return channelManger->isAcquisitionCompleted(chNumber);
	}

public:
	/** Stops data acquisition regardless of the preset mode of
	 * the acquisition.
	 */
	void stopAcquisition() {
		channelManger->stopAcquisition();
	}

public:
	void sendCPUTrigger(size_t chNumber) {
		if (chNumber < NumberOfChannels) {
			channelModules[chNumber]->sendCPUTrigger();
		} else {
			using namespace std;
			cerr << "Error in sendCPUTrigger(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets measurement preset mode.
	 * Available preset modes are:<br>
	 * PresetMode::Livetime<br>
	 * PresetMode::Number of Event<br>
	 * PresetMode::NonStop (Forever)
	 * @param presetMode preset mode value (see also enum class SpaceFibreADC::PresetMode )
	 */
	void setPresetMode(SpaceFibreADC::PresetMode presetMode) {
		channelManger->setPresetMode(presetMode);
	}

public:
	/** Sets Livetime preset value.
	 * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
	 */
	void setPresetLivetime(uint32_t livetimeIn10msUnit) {
		channelManger->setPresetLivetime(livetimeIn10msUnit);
	}

public:
	/** Sets PresetnEventsRegister.
	 * @param nEvents number of event to be taken
	 */
	void setPresetnEvents(uint32_t nEvents) {
		channelManger->setPresetnEvents(nEvents);
	}

public:
	/** Get Realtime which is elapsed time since the data acquisition was started.
	 * @return elapsed real time in 10ms unit
	 */
	double getRealtime() {
		return channelManger->getRealtime();
	}

//=============================================
	SpaceFibreADC::HouseKeepingData getHouseKeepingData() {
		SpaceFibreADC::HouseKeepingData hkData;
		//realtime
		hkData.realtime = channelManger->getRealtime();

		for (size_t i = 0; i < NumberOfChannels; i++) {
			//livetime
			hkData.livetime[i] = channelModules[i]->getLivetime();
			//acquisition status
			hkData.acquisitionStarted[i] = channelManger->isAcquisitionCompleted(i);
		}

		return hkData;
	}

};

#endif /* SPACEFIBREADCBoard_HH_ */
