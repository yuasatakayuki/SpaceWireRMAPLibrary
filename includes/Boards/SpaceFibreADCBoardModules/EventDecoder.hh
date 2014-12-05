/*
 * EventDecoder.hh
 *
 *  Created on: Oct 27, 2014
 *      Author: yuasa
 */

#ifndef EVENTDECODER_HH_
#define EVENTDECODER_HH_

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Types.hh"
#include <queue>
#include <stack>

/** Decodes event data received from the SpaceFibre ADC Board.
 * Decoded event instances will be stored in a queue.
 */
class EventDecoder {
private:
	struct RawEvent {
		uint16_t flag_FFF0;
		uint16_t ch;
		uint16_t consumerID;
		uint16_t phaMax;
		uint16_t timeH;
		uint16_t timeM;
		uint16_t timeL;
		uint16_t triggerCountH;
		uint16_t triggerCountL;
		uint16_t baseline;
		uint16_t flag_FFF1;
		uint16_t* waveform;
		uint16_t flag_FFF2;
		uint16_t flag_FFFF;
	} rawEvent;

private:
	enum class EventDecoderState {
		state_flag_FFF0,
		state_ch,
		state_consumerID,
		state_phaMax,
		state_timeH,
		state_timeM,
		state_timeL,
		state_triggerCountH,
		state_triggerCountL,
		state_baseline,
		state_flag_FFF1,
		state_pha_list,
		//state_flag_FFF2, not used in the switch statement
		state_flag_FFFF
	};

private:
	EventDecoderState state;
	std::vector<SpaceFibreADC::Event*> eventQueue;
	std::vector<uint16_t> readDataUint16Array;
	std::stack<SpaceFibreADC::Event*> eventInstanceResavoir;
	size_t waveformLength;

public:
	/** Constructor.
	 */
	EventDecoder() {
		state = EventDecoderState::state_flag_FFF0;
		rawEvent.waveform = new uint16_t[SpaceFibreADC::MaxWaveformLength];
		prepareEventInstances();
	}

public:
	virtual ~EventDecoder() {
		while (eventInstanceResavoir.size() != 0) {
			SpaceFibreADC::Event* event = eventInstanceResavoir.top();
			eventInstanceResavoir.pop();
			delete event->waveform;
			delete event;
		}
		delete rawEvent.waveform;
	}

public:
	void decodeEvent(std::vector<uint8_t>* readDataUint8Array) {
		using namespace std;

		size_t size = readDataUint8Array->size();
		if(size%2==1){
			cerr << "EventDecoder::decodeEvent(): odd data length " << size << " bytes" << endl;
			exit(-1);
		}

		size_t size_half = size / 2;

		if (Debug::eventdecoder()) {
			cout << "EventDecoder::decodeEvent() read " << size << " bytes (state = " << stateToString() << ")" << endl;
		}

		//resize if necessary
		if (size_half > readDataUint16Array.size()) {
			readDataUint16Array.resize(size_half);
		}

		//fill data
		for (size_t i = 0; i < size_half; i++) {
			readDataUint16Array[i] = (readDataUint8Array->at((i << 1) + 1) << 8) + readDataUint8Array->at((i << 1));
		}

		//decode the data
		for (size_t i = 0; i < size_half; i++) {
			switch (state) {
			case EventDecoderState::state_flag_FFF0:
				waveformLength = 0;
				if (readDataUint16Array[i] == 0xfff0) {
					state = EventDecoderState::state_ch;
				}else{
					cerr << "EventDecoder::decodeEvent(): invalid start flag (" << "0x" << hex << right << setw(4) << setfill('0')  << (uint32_t)readDataUint16Array[i] << ")" << endl;
				}
				break;
			case EventDecoderState::state_ch:
				rawEvent.ch = readDataUint16Array[i];
				state = EventDecoderState::state_consumerID;
				break;
			case EventDecoderState::state_consumerID:
				rawEvent.consumerID = readDataUint16Array[i];
				state = EventDecoderState::state_phaMax;
				break;
			case EventDecoderState::state_phaMax:
				rawEvent.phaMax = readDataUint16Array[i];
				state = EventDecoderState::state_timeH;
				break;
			case EventDecoderState::state_timeH:
				rawEvent.timeH = readDataUint16Array[i];
				state = EventDecoderState::state_timeM;
				break;
			case EventDecoderState::state_timeM:
				rawEvent.timeM = readDataUint16Array[i];
				state = EventDecoderState::state_timeL;
				break;
			case EventDecoderState::state_timeL:
				rawEvent.timeL = readDataUint16Array[i];
				state = EventDecoderState::state_triggerCountH;
				break;
			case EventDecoderState::state_triggerCountH:
				rawEvent.triggerCountH = readDataUint16Array[i];
				state = EventDecoderState::state_triggerCountL;
				break;
			case EventDecoderState::state_triggerCountL:
				rawEvent.triggerCountL = readDataUint16Array[i];
				state = EventDecoderState::state_baseline;
				break;
			case EventDecoderState::state_baseline:
				rawEvent.baseline = readDataUint16Array[i];
				state = EventDecoderState::state_flag_FFF1;
				break;
			case EventDecoderState::state_flag_FFF1:
				if (readDataUint16Array[i] == 0xfff1) {
					state = EventDecoderState::state_pha_list;
				}
				break;
			case EventDecoderState::state_pha_list:
				if (readDataUint16Array[i] == 0xfff2) {
					state = EventDecoderState::state_flag_FFFF;
				} else {
					rawEvent.waveform[waveformLength] = readDataUint16Array[i];
					waveformLength++;
				}
				break;
			case EventDecoderState::state_flag_FFFF:
				//if (0 <= ch && ch < SpaceWireADCBox::NumberOfChannels) {
				//tree->SetBranchAddress("pha_list", &(waveforms[ch]->at(0)));
				//}

				//push SpaceFibreADC::Event to a queue
				pushEventToQueue();

				//move to the idle state
				state = EventDecoderState::state_flag_FFF0;
				break;
			}
		}
	}

public:
	static const size_t InitialEventInstanceNumber = 10000;

private:
	void prepareEventInstances() {
		for (size_t i = 0; i < InitialEventInstanceNumber; i++) {
			SpaceFibreADC::Event* event = new SpaceFibreADC::Event;
			event->waveform = new uint16_t[SpaceFibreADC::MaxWaveformLength];
			eventInstanceResavoir.push(event);
		}
	}

public:
	void pushEventToQueue() {
		SpaceFibreADC::Event* event;
		if (eventInstanceResavoir.size() == 0) {
			event = new SpaceFibreADC::Event;
			//debug dump
			if (Debug::eventdecoder()) {
				using namespace std;
				cerr << "EventDecoder::pushEventToQueue() new Event instance was created." << endl;
			}
			event->waveform = new uint16_t[SpaceFibreADC::MaxWaveformLength];
		} else {
			//reuse already created instance
			event = eventInstanceResavoir.top();
			eventInstanceResavoir.pop();
		}
		event->ch = rawEvent.ch;
		event->timeTag = (static_cast<uint64_t>(rawEvent.timeH) << 32) + (static_cast<uint64_t>(rawEvent.timeM) << 16)
				+ (rawEvent.timeL);
		event->phaMax = rawEvent.phaMax;
		event->nSamples = waveformLength;
		event->livetime = 0; //todo: implement event livetime
		event->triggerCount = ((static_cast<uint32_t>(rawEvent.triggerCountH)) << 16)
				+ (static_cast<uint32_t>(rawEvent.triggerCountL));

		//copy waveform
		for (size_t i = 0; i < waveformLength; i++) {
			event->waveform[i] = rawEvent.waveform[i];
		}

		eventQueue.push_back(event);
	}

public:
	/** Returns decoded event queue (as std::vecotr).
	 * After used in user application, decoded events should be freed
	 * via EventDecoder::freeEvent(SpaceFibreADC::Event* event).
	 * @return std::queue containing pointers to decoded events
	 */
	std::vector<SpaceFibreADC::Event*> getDecodedEvents() {
		std::vector<SpaceFibreADC::Event*> eventQueueCopied = eventQueue;
		eventQueue.clear();
		return eventQueueCopied;
	}

public:
	/** Frees event instance so that buffer area can be reused in the following commands.
	 * @param event event instance to be freed
	 */
	void freeEvent(SpaceFibreADC::Event* event) {
		eventInstanceResavoir.push(event);
	}

public:
	/** Returns the number of available (allocated) Event instances.
	 * @return the number of Event instances
	 */
	size_t getNAllocatedEventInstances() {
		return eventInstanceResavoir.size();
	}

public:
public:
	std::string stateToString() {
		std::string result;
		switch (state) {

		case EventDecoderState::state_flag_FFF0:
			result = "state_flag_FFF0";
			break;
		case EventDecoderState::state_ch:
			result = "state_ch";
			break;
		case EventDecoderState::state_consumerID:
			result = "state_consumerID";
			break;
		case EventDecoderState::state_phaMax:
			result = "state_phaMax";
			break;
		case EventDecoderState::state_timeH:
			result = "state_timeH";
			break;
		case EventDecoderState::state_timeM:
			result = "state_timeM";
			break;
		case EventDecoderState::state_timeL:
			result = "state_timeL";
			break;
		case EventDecoderState::state_triggerCountH:
			result = "state_triggerCountH";
			break;
		case EventDecoderState::state_triggerCountL:
			result = "state_triggerCountL";
			break;
		case EventDecoderState::state_baseline:
			result = "state_baseline";
			break;
		case EventDecoderState::state_flag_FFF1:
			result = "state_flag_FFF1";
			break;
		case EventDecoderState::state_pha_list:
			result = "state_pha_list";
			break;
		case EventDecoderState::state_flag_FFFF:
			result = "state_flag_FFFF";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

#endif /* EVENTDECODER_HH_ */
