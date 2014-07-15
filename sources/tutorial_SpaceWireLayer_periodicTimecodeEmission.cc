/*
 * tutorial_SpaceWireLayer_periodicTimecodeEmission.cc
 *
 *  Created on: Jan 10, 2012
 *      Author: yuasa
 */

#include "SpaceWire.hh"

class TimecodeThread: public CxxUtilities::StoppableThread {
private:
	SpaceWireIF* spwif;

public:
	static constexpr double TimecodeFrequency = 64; //Hz

public:
	TimecodeThread(SpaceWireIF* spwif) {
		this->spwif = spwif;
	}

public:
	void run() {
		uint8_t timecode = 0x00;
		while (!isStopped()) {
			try {
				spwif->emitTimecode(timecode);
			} catch (...) {
				using namespace std;
				cerr << "Timecode emission failed" << endl;
			}
			if (timecode == 63) {
				timecode = 0;
			} else {
				timecode++;
			}
			sleep(1 / TimecodeFrequency);
		}
	}
};

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;

	/* Open the SpaceWire interface */
	cout << "Opening SpaceWireIF...";
	SpaceWireIF* spwif = new SpaceWireIFOverIPClient("192.168.1.100", 10030);
	try {
		spwif->open();
	} catch (...) {
		cerr << "Connection timed out." << endl;
		exit(-1);
	}
	cout << "done" << endl;

	/* Start timecode emission */
	TimecodeThread* timecodeThread=new TimecodeThread(spwif);
	timecodeThread->start();

	/*************************************/
	/* Insert user application code here */
	/*************************************/


	/* Sample : 10s wait */
	Condition c;
	c.wait(10*1000);//10seconds

	/* Stop timecode thread */
	timecodeThread->stop();

	/* Wait until the thread is totally stopped */
	c.wait(2*1/TimecodeThread::TimecodeFrequency);

	/* Delete timecodeThread */
	delete timecodeThread;

	/* Close */
	spwif->close();

	delete spwif;
}
