/*
 * Debug.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef DEBUG_HH_
#define DEBUG_HH_

/** An internal class which represents Debug configuration.
 *
 */
class Debug {
public:
	/** Returns if semaphore block is in debug mode.
	 * @return true if semaphore block is in debug mode.
	 */
	static bool semaphore() {
		return true;
	}
	/** Returns if channelmodule block is in debug mode.
	 * @return true if channelmodule block is in debug mode.
	 */
	static bool channelmodule() {
		return true;
	}
	/** Returns if consumermanager block is in debug mode.
	 * @return true if consumermanager block is in debug mode.
	 */
	static bool consumermanager() {
		return true;
	}
	/** Returns if ring buffer block is in debug mode.
	 * @return true if ring buffer block is in debug mode.
	 */
	static bool ringbuffer() {
		return false;
	}
	/** Returns if channelmanager block is in debug mode.
	 * @return true if channelmanager block is in debug mode.
	 */
	static bool channelmanager() {
		return true;
	}
	/** Returns if SpaceWire ADC Box is in debug mode.
	 * @return true if adc box is in debug mode.
	 */
	static bool adcbox() {
		return false;
	}
	/** Returns if DataRecorder is in debug mode.
	 * @return true if DataRecorder is in debug mode.
	 */
	static bool datarecorder() {
		return false;
	}
	/** Returns if EventDecoder is in debug mode.
	 * @return true if DataRecorder is in debug mode.
	 */
	static bool eventdecoder() {
		return true;
	}


};


#endif /* DEBUG_HH_ */
