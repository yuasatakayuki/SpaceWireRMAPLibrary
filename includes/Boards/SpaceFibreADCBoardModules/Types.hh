/*
 * Types.hh
 *
 *  Created on: Oct 27, 2014
 *      Author: yuasa
 */

#ifndef SPACEFIBREADC_TYPES_HH_
#define SPACEFIBREADC_TYPES_HH_

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Constants.hh"

namespace SpaceFibreADC {
enum class TriggerMode
	: uint32_t {
		StartThreshold_NSamples_AutoClose = 1, //
	CommonGateIn = 2, //
	StartThreshold_NSamples_CloseThreshold = 3, //
	CPUTrigger = 5, //
	TriggerBusSelectedOR = 8, //
	TriggerBusSelectedAND = 9
};

enum class PresetMode
	: uint32_t {
		NonStop = 0, //
	Livetime = 1, //
	NumberOfEvents = 2, //
};

struct HouseKeepingData {
	uint32_t realtime;
	uint32_t livetime[SpaceFibreADC::NumberOfChannels];
	bool acquisitionStarted[SpaceFibreADC::NumberOfChannels];
};

struct Event {
	uint8_t ch;
	uint64_t timeTag;
	uint32_t triggerCount;
	uint32_t livetime;
	uint16_t phaMax;
	uint16_t nSamples;
	uint16_t* waveform;
};

enum class ADCClockFrequency
	: uint16_t {
		ADCClock200MHz = 20000, ADCClock100MHz = 10000, ADCClock50MHz = 5000
};
}

#endif /* SPACEFIBREADC_TYPES_HH_ */
