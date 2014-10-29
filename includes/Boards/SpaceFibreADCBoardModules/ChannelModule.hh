/*
 * ChannelModule.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef CHANNELMODULE_HH_
#define CHANNELMODULE_HH_

#include "Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"
#include "Boards/SpaceFibreADCBoardModules/Types.hh"

/** A class which represents a ChannelModule on VHDL logic.
 */
class ChannelModule {
public:
	static const uint32_t InitialAddressOf_ChModule_0 = 0x01011000;
	static const uint32_t AddressOffsetBetweenChannels = 0x100;

public:
	uint32_t AddressOf_TriggerModeRegister;
	uint32_t AddressOf_TriggerBusMaskRegister;
	uint32_t AddressOf_NumberOfSamplesRegister;
	uint32_t AddressOf_ThresholdStartingRegister;
	uint32_t AddressOf_ThresholdClosingRegister;
	uint32_t AddressOf_AdcPowerDownModeRegister;
	uint32_t AddressOf_DepthOfDelayRegister;
	uint32_t AddressOf_LivetimeRegisterL;
	uint32_t AddressOf_LivetimeRegisterH;
	uint32_t AddressOf_CurrentAdcDataRegister;
	uint32_t AddressOf_CPUTriggerRegister;
	uint32_t AddressOf_DigitalFilterTrigger_ThresholdDeltaRegister;
	uint32_t AddressOf_DigitalFilterTrigger_WidthRegister;
	uint32_t AddressOf_DigitalFilterTrigger_HitPattern_FilterCoefficientSelectorRegister;

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;
	int chNumber;

public:
	/** Constructor.
	 * @param[in] rmaphandler a pointer to an instance of RMAPHandler
	 * @param[in] chNumber this instance's channel number
	 */
	ChannelModule(RMAPHandler* rmapHandler, RMAPTargetNode* adcRMAPTargetNode, int chNumber) {
		this->rmapHandler = rmapHandler;
		this->adcRMAPTargetNode = adcRMAPTargetNode;
		this->chNumber = chNumber;
		uint32_t BA = InitialAddressOf_ChModule_0 + chNumber * AddressOffsetBetweenChannels;
		AddressOf_TriggerModeRegister = BA + 0x0002;
		AddressOf_NumberOfSamplesRegister = BA + 0x0004;
		AddressOf_ThresholdStartingRegister = BA + 0x0006;
		AddressOf_ThresholdClosingRegister = BA + 0x0008;
		AddressOf_AdcPowerDownModeRegister = BA + 0x000a;
		AddressOf_DepthOfDelayRegister = BA + 0x000c;
		AddressOf_LivetimeRegisterL = BA + 0x000e;
		AddressOf_LivetimeRegisterH = BA + 0x0010;
		AddressOf_CurrentAdcDataRegister = BA + 0x0012;
		AddressOf_CPUTriggerRegister = BA + 0x0014;
		AddressOf_DigitalFilterTrigger_ThresholdDeltaRegister = BA + 0x0018;
		AddressOf_DigitalFilterTrigger_WidthRegister = BA + 0x001a;
		AddressOf_DigitalFilterTrigger_HitPattern_FilterCoefficientSelectorRegister = BA + 0x0020;
		AddressOf_TriggerBusMaskRegister = BA + 0x0024;
	}

public:
	void setRegister(uint32_t address, std::vector<uint8_t> data) {
		rmapHandler->write(adcRMAPTargetNode, address, &data[0], data.size());
	}

public:
	uint32_t getRegister(uint32_t address) {
		std::vector<uint8_t> readData(2);
		rmapHandler->read(adcRMAPTargetNode, address, 2, &readData[0]);
		return (uint32_t) (readData[1] * 0x100 + readData[0]);
	}

public:
	/** Sets trigger mode.
	 * SpaceFibreADC::TriggerMode;<br>
	 * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
	 * Common Gate In = CommonGateIn<br>
	 * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
	 * CPU Trigger = CPUTrigger<br>
	 * Trigger Bus (OR) = TriggerBusSelectedOR<br>
	 * Trigger Bus (AND) = TriggerBusSelectedAND<br>
	 * @param triggerMode trigger mode number
	 */
	void setTriggerMode(SpaceFibreADC::TriggerMode triggerMode) {
		uint32_t triggerModeInteger = static_cast<uint32_t>(triggerMode);
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting TriggerMode...";
		}
		vector<uint8_t> writeData = { static_cast<uint8_t>(triggerModeInteger), 0x00 };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_TriggerModeRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			std::vector<uint8_t> readData(2);
			rmapHandler->read(adcRMAPTargetNode, AddressOf_TriggerModeRegister, 2, &readData[0]);
			cout << "done(" << hex << setw(4) << (uint32_t) (readData[0]) << dec << ")" << endl;
		}
	}

public:
	/** Sets TriggerBusMask which is used in TriggerMode==TriggerBus.
	 * @param enabledChannels array of enabled trigger bus channels.
	 */
	void setTriggerBusMask(std::vector<size_t> enabledChannels) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting TriggerBusMask...";
		}
		std::bitset<32> maskBitPatter;
		for (size_t i = 0; i < enabledChannels.size(); i++) {
			maskBitPatter[enabledChannels[i]] = 1;
		}
		uint8_t mask = maskBitPatter.to_ulong();
		vector<uint8_t> writeData = { static_cast<uint8_t>(mask), 0x00 };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_TriggerBusMaskRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			std::vector<uint8_t> readData(2);
			rmapHandler->read(adcRMAPTargetNode, AddressOf_TriggerBusMaskRegister, 2, &readData[0]);
			cout << "done(" << hex << setw(4) << (uint32_t) (readData[0]) << dec << ")" << endl;
		}
	}

public:
	void sendCPUTrigger() {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") sending CPU trigger...";
		}
		vector<uint8_t> writeData = { 0xff, 0xff };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_CPUTriggerRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets number of samples register.
	 * Waveforms are sampled according to this number.
	 * @param nSamples number of adc samples per one waveform
	 */
	void setNumberOfSamples(uint32_t nSamples) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting NumberOfSamples...";
		}
		vector<uint8_t> writeData = { static_cast<uint8_t>(nSamples % 0x100), static_cast<uint8_t>(nSamples / 0x100) };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_NumberOfSamplesRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			std::vector<uint8_t> readData(2);
			rmapHandler->read(adcRMAPTargetNode, AddressOf_NumberOfSamplesRegister, 2, &readData[0]);
			cout << "done(" << hex << setw(4) << (uint32_t) (readData[0]) << dec << ")" << endl;
		}
	}

public:
	/** Sets Leading Trigger Threshold.
	 * @param threshold an adc value for leading trigger threshold
	 */
	void setStartingThreshold(uint32_t threshold) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting LeadingTriggerThreshold...";
		}
		vector<uint8_t> writeData = { static_cast<uint8_t>(threshold % 0x100), static_cast<uint8_t>(threshold / 0x100) };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_ThresholdStartingRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			std::vector<uint8_t> readData(2);
			rmapHandler->read(adcRMAPTargetNode, AddressOf_ThresholdStartingRegister, 2, &readData[0]);
			cout << "done(" << hex << setw(4) << (uint32_t) (readData[1] * 0x100 + readData[0]) << dec << ")" << endl;
		}
	}

public:
	/** Sets Trailing Trigger Threshold.
	 * @param threshold an adc value for trailing trigger threshold
	 */
	void setClosingThreshold(uint32_t threshold) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting TrailingTriggerThreshold...";
		}
		vector<uint8_t> writeData = { static_cast<uint8_t>(threshold % 0x100), static_cast<uint8_t>(threshold / 0x100) };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_ThresholdClosingRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			std::vector<uint8_t> readData(2);
			rmapHandler->read(adcRMAPTargetNode, AddressOf_ThresholdClosingRegister, 2, &readData[0]);
			cout << "done(" << hex << setw(4) << (uint32_t) (readData[1] * 0x100 + readData[0]) << dec << ")" << endl;
		}
	}

public:
	/** Turn on/off power of this channle's ADC chip.
	 * @param trueifon true if turing on, false if turing off
	 */
	void turnADCPower(bool trueifon) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") turning ADC ";
			if (trueifon) {
				cout << "on...";
			} else {
				cout << "off...";
			}
		}
		vector<uint8_t> writeData;
		if (trueifon) {
			writeData.push_back(0x00);
			writeData.push_back(0x00);
		} else {
			writeData.push_back(0xff);
			writeData.push_back(0xff);
		}
		rmapHandler->write(adcRMAPTargetNode, AddressOf_AdcPowerDownModeRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			cout << "done" << endl;
		}
	}

public:
	/** Sets depth of delay per trigger. When triggered,
	 * a waveform will be recorded starting from N steps
	 * before of the trigger timing.
	 * @param depthOfDelay number of samples retarded
	 */
	void setDepthOfDelay(uint32_t depthOfDelay) {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") setting Depth Of Delay...";
		}
		vector<uint8_t> writeData =
				{ static_cast<uint8_t>(depthOfDelay % 0x100), static_cast<uint8_t>(depthOfDelay / 0x100) };
		rmapHandler->write(adcRMAPTargetNode, AddressOf_DepthOfDelayRegister, &writeData[0], 2);
		if (Debug::channelmodule()) {
			cout << "done" << endl;
		}
	}

public:
	/** Gets Livetime.
	 * @return elapsed livetime in 10ms unit
	 */
	uint32_t getLivetime() {
		std::vector<uint8_t> readData(4);
		rmapHandler->read(adcRMAPTargetNode, AddressOf_LivetimeRegisterL, 4, &readData[0]);
		uint32_t livetime = readData[3] * 0x1000000 + readData[2] * 0x10000 + readData[1] * 0x100 + readData[0];
		return livetime;
	}

public:
	/** Get current ADC value.
	 * @return temporal ADC value
	 */
	uint32_t getCurrentADCValue() {
		using namespace std;
		if (Debug::channelmodule()) {
			cout << "channelmodule(" << chNumber << ") current ADC value:0x";
		}
		std::vector<uint8_t> readData(2);
		rmapHandler->read(adcRMAPTargetNode, AddressOf_CurrentAdcDataRegister, 2, &readData[0]);
		uint32_t adcvalue = (uint32_t) (readData[1] * 0x100 + readData[0]);
		if (Debug::channelmodule()) {
			cout << hex << setw(4) << setfill('0') << adcvalue << dec << endl;
		}
		return adcvalue;
	}
};

#endif /* CHANNELMODULE_HH_ */
