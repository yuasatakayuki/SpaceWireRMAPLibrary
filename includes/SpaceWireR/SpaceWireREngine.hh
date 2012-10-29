/*
 * SpaceWireREngine.hh
 *
 *  Created on: Oct 1, 2012
 *      Author: yuasa
 */

#ifndef SPACEWIRERENGINE_HH_
#define SPACEWIRERENGINE_HH_

#include "CxxUtilities/Thread.hh"

class SpaceWireREngine: public CxxUtilities::StoppableThread {
private:
	SpaceWireIF* spwif;
	CxxUtilities::Condition stopCondition;

private:
	static const double TimeoutDurationForStopCondition = 1000;

public:
	SpaceWireREngine(SpaceWireIF* spwif) {
		this->spwif = spwif;
	}

public:
	void run() {
		while (!stopped) {
			this->stopCondition.wait(TimeoutDurationForStopCondition);
		}

	}

public:
	void stop() {
		//stop child threads
		//todo

		//stop main thread
		stopCondition.signal();

	}

public:
	SpaceWireIF* getSpaceWireIF() {
		return spwif;
	}

public:
	class SpaceWireREngineSendThread: public CxxUtilities::StoppableThread {
	private:
		SpaceWireREngine* spwrEngine;

	public:
		SpaceWireREngineSendThread(SpaceWireREngine* spwrEngine) {
			this->spwrEngine = spwrEngine;
		}

	public:
		void run() {

		}
	};

public:
	class SpaceWireREngineReceiveThread: public CxxUtilities::StoppableThread {
	private:
		SpaceWireREngine* spwrEngine;

	public:
		SpaceWireREngineReceiveThread(SpaceWireREngine* spwrEngine) {
			this->spwrEngine = spwrEngine;
		}

	public:
		void run() {

		}
	};

};

#endif /* SPACEWIRERENGINE_HH_ */
