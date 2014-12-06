/*
 * SpaceFibreADCBoard.hh
 *
 *  Created on: May 15, 2009
 *      Author: yuasa
 */

#ifndef SPACEFIBREADCBOARD_HH_
#define SPACEFIBREADCBOARD_HH_

/** @mainpage SpaceFibre ADC C++ API
 * # Overview
 * SpaceFibre ADC board carries 4-ch 200-Msps pipeline ADC,
 * GigabitEthernet interface, and 1GB SDRAM.
 * SpaceFibreADC.hh and related header files provide API for
 * communicating with the SpaceFibre ADC board using C++.
 *
 * # Structure
 * SpaceFibreADCBoard is the top-level class which provides
 * all user-level methods to control and configure the board,
 * to set/start/stop the evnet acquisition mode, and
 * to decode recorded event data and waveform.
 * For available methods,
 * see SpaceFibreADCBoard API page.
 *
 * # Typical usage
 * -# Include the following header files.
 *   - SpaceWireRMAPLibrary/SpaceWire.hh
 *   - SpaceWireRMAPLibrary/RMAP.hh
 *   - SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh
 * -# Instantiate the SpaceFibreADCBoard class.
 * -# Do configuration:
 *   - Trigger mode.
 *   - Threshold.
 *   - Waveform record length.
 *   - Preset mode (to automatically stop measurement).
 *     - Livetime preset.
 *     - Event count preset.
 *     - No preset (measurement continues forever unless manually stopped).
 * -# Start measurement.
 * -# Read event data. Loop until measurement completes.
 * -# Stop measurement.
 * -# Close device.
 *
 * # Contact
 * - Takayuki Yuasa
 *   - email: takayuki.yuasa at riken.jp
 *
 * # Example user application
 * @code
 //---------------------------------------------
 // Include SpaceWireRMAPLibrary
 #include "SpaceWireRMAPLibrary/SpaceWire.hh"
 #include "SpaceWireRMAPLibrary/RMAP.hh"

 //---------------------------------------------
 // Include SpaceFibreADC
 #include "SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh"

 const size_t nSamples = 200;
 const size_t nCPUTrigger=10;

 int main(int argc, char* argv[]){
 using namespace std;

 //---------------------------------------------
 // Construct an instance of SpaceFibreADCBoard
 // The default IP address is 192.168.1.100 .
 // If your board has set a specific IP address,
 // modify the following line.
 SpaceFibreADCBoard* adc=new SpaceFibreADCBoard("192.168.1.100");

 //---------------------------------------------
 // Device open
 cout << "Opening device" << endl;
 adc->openDevice();
 cout << "Opened" << endl;

 //---------------------------------------------
 // Set ADC Clock
 adc->setAdcClock(SpaceFibreADC::ADCClockFrequency::ADCClock200MHz);

 //---------------------------------------------
 // Trigger Mode
 size_t ch=0;
 cout << "Setting trigger mode" << endl;

 // Trigger Mode: CPU Trigger
 adc->setTriggerMode(ch, SpaceFibreADC::TriggerMode::CPUTrigger);

 // Trigger Mode: StartingThreshold-NSamples-ClosingThreshold
 // uint16_t startingThreshold=2300;
 // uint16_t closingThreshold=2100;
 // adc->setStartingThreshold(ch, startingThreshold);
 // adc->setClosingThreshold(ch, closingThreshold);
 //adc->setTriggerMode(ch, SpaceFibreADC::TriggerMode::StartThreshold_NSamples_CloseThreshold);

 //---------------------------------------------
 // Waveform record length
 cout << "Setting number of samples" << endl;
 adc->setNumberOfSamples(200);
 cout << "Setting depth of delay" << endl;
 adc->setDepthOfDelay(ch, 20);
 cout << "Setting ADC power" << endl;
 adc->turnOnADCPower(ch);


 //---------------------------------------------
 // Start data acquisition
 cout << "Starting acquisition" << endl;
 adc->startAcquisition({true,false,false,false});


 //---------------------------------------------
 // Send CPU Trigger
 cout << "Sending CPU Trigger " << dec << nCPUTrigger << " times" << endl;
 for(size_t i=0;i<nCPUTrigger;i++){
 adc->sendCPUTrigger(ch);
 }


 //---------------------------------------------
 // Read event data
 std::vector<SpaceFibreADC::Event*> events;

 cout << "Reading events" << endl;
 events=adc->getEvent();
 cout << "nEvents = " << events.size() << endl;

 size_t i=0;
 for(auto event : events){
 cout << dec;
 cout << "=============================================" << endl;
 cout << "Event " << i << endl;
 cout << "---------------------------------------------" << endl;
 cout << "timeTag = " << event->timeTag << endl;
 cout << "triggerCount = " << event->triggerCount << endl;
 cout << "phaMax = " << (uint32_t)event->phaMax << endl;
 cout << "nSamples = " << (uint32_t)event->nSamples << endl;
 cout << "waveform = ";
 for(size_t i=0;i<event->nSamples;i++){
 cout << dec << (uint32_t)event->waveform[i] << " ";
 }
 cout << endl;
 i++;
 adc->freeEvent(event);
 }

 //---------------------------------------------
 // Reads HK data
 SpaceFibreADC::HouseKeepingData hk=adc->getHouseKeepingData();
 cout << "livetime = " << hk.livetime[0] << endl;
 cout << "realtime = " << hk.realtime << endl;
 cout << dec;

 //---------------------------------------------
 // Reads current ADC value (un-comment out the following lines to execute)
 // cout << "Current ADC values:"  << endl;
 // for(size_t i=0;i<10;i++){
 // 	cout << adc->getCurrentADCValue(ch) << endl;
 // }

 //---------------------------------------------
 // Stop data acquisition
 cout << "Stopping acquisition" << endl;
 adc->stopAcquisition();

 //---------------------------------------------
 // Device close
 cout << "Closing device" << endl;
 adc->closeDevice();
 cout << "Closed" << endl;

 }
 * @endcode
 */

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Constants.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Debug.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/SemaphoreRegister.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/ConsumerManagerSocketFIFO.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/EventDecoder.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/ChannelModule.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/ChannelManager.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"

enum class SpaceFibreADCException {
	InvalidChannelNumber, OpenDeviceFailed, CloseDeviceFailed,
};

/** A class which represents SpaceFibre ADC Board.
 * It controls start/stop of data acquisition.
 * It contains SpaceFibreADCBoard::SpaceFibreADC::NumberOfChannels instances of
 * ADCChannel class so that
 * a user can change individual registers in each module.
 */
class SpaceFibreADCBoard {
public:
	class SpaceFibreADCBoardDumpThread: public CxxUtilities::StoppableThread {
	private:
		SpaceFibreADCBoard* parent;
		EventDecoder* eventDecoder;
	public:
		SpaceFibreADCBoardDumpThread(SpaceFibreADCBoard* parent) {
			this->parent = parent;
			this->eventDecoder = parent->getEventDecoder();
		}

	public:
		static constexpr double WaitDurationInMilliSec = 1000.0;

	public:
		void run() {
			CxxUtilities::Condition c;
			using namespace std;
			size_t nReceivedEvents_previous = 0;
			size_t delta = 0;
			size_t nReceivedEvents_latch;
			while (!this->stopped) {
				c.wait(WaitDurationInMilliSec);
				nReceivedEvents_latch = parent->nReceivedEvents;
				delta = nReceivedEvents_latch - nReceivedEvents_previous;
				nReceivedEvents_previous = nReceivedEvents_latch;
				cout << "SpaceFibreADCBoard received " << dec << parent->nReceivedEvents << " events (delta=" << dec << delta
						<< ")." << endl;
				cout << "SpaceFibreADC::EventDecoder available Event instances = " << dec
						<< this->eventDecoder->getNAllocatedEventInstances() << endl;
			}
		}
	};

public:
	//Clock Frequency
	static constexpr double ClockFrequency = 50; //MHz

	//Clock Interval
	static constexpr double ClockInterval = 20e-9; //s

	//SpaceWire-to-GigabitEther RMAP TCP/IP Port number
	static const uint32_t TCPPortNumberForRMAPOverTCP = 10032;

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;
	ChannelManager* channelManager;
	ConsumerManagerSocketFIFO* consumerManager;
	ChannelModule* channelModules[SpaceFibreADC::NumberOfChannels];
	EventDecoder* eventDecoder;
	SpaceFibreADCBoardDumpThread* dumpThread;

public:
	size_t nReceivedEvents = 0;

public:
	/** Constructor.
	 * @param ipAddress IP address of the SpaceFibre ADC board
	 * @param tcpPortNumber TCP/IP Port number
	 */
	SpaceFibreADCBoard(std::string ipAddress) {
		using namespace std;

		//construct RMAPTargetNode instance
		adcRMAPTargetNode = new RMAPTargetNode;
		adcRMAPTargetNode->setID("ADCBox");
		adcRMAPTargetNode->setDefaultKey(0x00);
		adcRMAPTargetNode->setReplyAddress( { });
		adcRMAPTargetNode->setTargetSpaceWireAddress( { });
		adcRMAPTargetNode->setTargetLogicalAddress(0xFE);
		adcRMAPTargetNode->setInitiatorLogicalAddress(0xFE);

		this->rmapHandler = new RMAPHandler(ipAddress, TCPPortNumberForRMAPOverTCP, { adcRMAPTargetNode });

		//create an instance of ChannelManager
		this->channelManager = new ChannelManager(rmapHandler, adcRMAPTargetNode);

		//create an instance of ConsumerManager
		this->consumerManager = new ConsumerManagerSocketFIFO(ipAddress, rmapHandler, adcRMAPTargetNode);

		//create instances of ADCChannelRegister
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			this->channelModules[i] = new ChannelModule(rmapHandler, adcRMAPTargetNode, i);
		}

		//event decoder
		this->eventDecoder = new EventDecoder();

		//dump thread
		this->dumpThread = new SpaceFibreADCBoardDumpThread(this);
		this->dumpThread->start();
	}

public:
	~SpaceFibreADCBoard() {
		using namespace std;
		cout << "SpaceFibreADCBoard::~SpaceFibreADCBoard(): Deconstructing SpaceFibreADCBoard instance." << endl;
		cout << "SpaceFibreADCBoard::~SpaceFibreADCBoard(): Stopping dump thread." << endl;
		this->dumpThread->stop();
		delete this->dumpThread;

		cout << "SpaceFibreADCBoard::~SpaceFibreADCBoard(): Deleting RMAP Handler." << endl;
		delete rmapHandler;

		cout << "SpaceFibreADCBoard::~SpaceFibreADCBoard(): Deleting internal modules." << endl;
		delete this->channelManager;
		delete this->consumerManager;
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			delete this->channelModules[i];
		}
		delete eventDecoder;
		cout << "SpaceFibreADCBoard::~SpaceFibreADCBoard(): Completed." << endl;
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
		return channelManager;
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
		this->channelManager->reset();
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
	/** Opens the device by making a
	 * GigabitEthernet connection to SpaceFibre ADC board.
	 */
	void openDevice() throw (SpaceFibreADCException) {
		try {
			if(rmapHandler->connectoToSpaceWireToGigabitEther()==false){
				throw SpaceFibreADCException::OpenDeviceFailed;
			}
			this->stopAcquisition();
			consumerManager->openSocket();
		} catch (...) {
			throw SpaceFibreADCException::OpenDeviceFailed;
		}
	}

public:
	/** Closes the device.
	 */
	void closeDevice() {
		using namespace std;
		try {
			cout << "#stopping event data output" << endl;
			consumerManager->disableEventDataOutput();
			cout << "#stopping dump thread" << endl;
			consumerManager->stopDumpThread();
			this->dumpThread->stop();
			cout << "#closing sockets" << endl;
			consumerManager->closeSocket();
			cout << "#disconnecting SpaceWire-to-GigabitEther" << endl;
			rmapHandler->disconnectSpWGbE();
		} catch (...) {
			throw SpaceFibreADCException::CloseDeviceFailed;
		}
	}

	//=============================================
public:
	/** Reads, decodes, and returns event data recorded by the board.
	 * When no event packet is received within a timeout duration,
	 * this method will return empty vector meaning a time out.
	 * Each SpaceFibreADC::Event instances pointed by the entities
	 * of the returned vector should be freed after use in the user
	 * application. A freed SpaceFibreADC::Event instance will be
	 * reused to represent another event data.
	 * @return a vector containing pointers to decoded event data
	 */
	std::vector<SpaceFibreADC::Event*> getEvent() {
		std::vector<SpaceFibreADC::Event*> events;
		std::vector<uint8_t> data = consumerManager->getEventData();
		if (data.size() != 0) {
			eventDecoder->decodeEvent(&data);
			events = eventDecoder->getDecodedEvents();
		}
		nReceivedEvents += events.size();
		return events;
	}

public:
	/** Frees an event instance so that buffer area can be reused in the following commands.
	 * @param[in] event event instance to be freed
	 */
	void freeEvent(SpaceFibreADC::Event* event) {
		eventDecoder->freeEvent(event);
	}

public:
	/** Frees event instances so that buffer area can be reused in the following commands.
	 * @param[in] events a vector of event instance to be freed
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setTriggerMode(triggerMode);
		} else {
			using namespace std;
			cerr << "Error in setTriggerMode(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets the number of ADC samples recorded for a single trigger.
	 * This method sets the same number to all the channels.
	 * After the trigger, nSamples set via this method will be recorded
	 * in the waveform buffer,
	 * and then analyzed in the Pulse Processing Module to form an event packet.
	 * In this processing, the waveform can be truncated at a specified length,
	 * and see setNumberOfSamplesInEventPacket(uint16_t nSamples) for this
	 * truncating length setting.
	 * @note As of 20141103, the maximum waveform length is 1000.
	 * This value can be increased by modifying the depth of the event FIFO
	 * in the FPGA.
	 * @param nSamples number of adc samples per one waveform data
	 */
	void setNumberOfSamples(uint16_t nSamples) {
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			channelModules[i]->setNumberOfSamples(nSamples);
		}
		setNumberOfSamplesInEventPacket(nSamples);
	}

public:
	/** Sets the number of ADC samples which should be output in the
	 * event packet (this must be smaller than the number of samples
	 * recorded for a trigger, see setNumberOfSamples(uint16_t nSamples) ).
	 * @param[in] nSamples number of ADC samples in the event packet
	 */
	void setNumberOfSamplesInEventPacket(uint16_t nSamples) {
		consumerManager->setEventPacket_NumberOfWaveform(nSamples);
	}

public:
	/** Sets Leading Trigger Threshold.
	 * @param threshold an adc value for leading trigger threshold
	 */
	void setStartingThreshold(size_t chNumber, uint32_t threshold) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->turnADCPower(true);
		} else {
			using namespace std;
			cerr << "Error in turnOnADCPower(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Turn off ADC.
	 */
	void turnOffADCPower(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		consumerManager->enableEventDataOutput();
		channelManager->startAcquisition(channelsToBeStarted);
	}

public:
	/** Checks if all data acquisition is completed in all channels.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted() {
		return channelManager->isAcquisitionCompleted();
	}

public:
	/** Checks if data acquisition of single channel is completed.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted(size_t chNumber) {
		return channelManager->isAcquisitionCompleted(chNumber);
	}

public:
	/** Stops data acquisition regardless of the preset mode of
	 * the acquisition.
	 */
	void stopAcquisition() {
		channelManager->stopAcquisition();
	}

public:
	/** Sends CPU Trigger to force triggering in the specified channel.
	 * @param[in] chNumber channel to be CPU-triggered
	 */
	void sendCPUTrigger(size_t chNumber) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
		channelManager->setPresetMode(presetMode);
	}

public:
	/** Sets Livetime preset value.
	 * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
	 */
	void setPresetLivetime(uint32_t livetimeIn10msUnit) {
		channelManager->setPresetLivetime(livetimeIn10msUnit);
	}

public:
	/** Sets PresetnEventsRegister.
	 * @param nEvents number of event to be taken
	 */
	void setPresetnEvents(uint32_t nEvents) {
		channelManager->setPresetnEvents(nEvents);
	}

public:
	/** Get Realtime which is elapsed time since the board power was turned on.
	 * @return elapsed real time in 10ms unit
	 */
	double getRealtime() {
		return channelManager->getRealtime();
	}

//=============================================
public:
	/** Sets ADC Clock.
	 * - ADCClockFrequency::ADCClock200MHz <br>
	 * - ADCClockFrequency::ADCClock100MHz <br>
	 * - ADCClockFrequency::ADCClock50MHz <br>
	 * @param adcClockFrequency enum class ADCClockFrequency
	 */
	void setAdcClock(SpaceFibreADC::ADCClockFrequency adcClockFrequency) {
		channelManager->setAdcClock(adcClockFrequency);
	}

//=============================================
public:
	/** Gets HK data including the real time and the live time which are
	 * counted in the FPGA, and acquisition status (started or stopped).
	 * @retrun HK information contained in a SpaceFibreADC::HouseKeepingData instance
	 */
	SpaceFibreADC::HouseKeepingData getHouseKeepingData() {
		SpaceFibreADC::HouseKeepingData hkData;
		//realtime
		hkData.realtime = channelManager->getRealtime();

		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			//livetime
			hkData.livetime[i] = channelModules[i]->getLivetime();
			//acquisition status
			hkData.acquisitionStarted[i] = channelManager->isAcquisitionCompleted(i);
		}

		return hkData;
	}

public:
	/** Returns EventDecoder instance.
	 * @return EventDecoder instance
	 */
	EventDecoder* getEventDecoder() {
		return eventDecoder;
	}

};

#endif /* SPACEFIBREADCBoard_HH_ */
