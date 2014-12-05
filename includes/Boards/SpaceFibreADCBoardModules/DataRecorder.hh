/*
 * DataRecorder.hh
 *
 *  Created on: 2009/05/19
 *      Author: yuasa
 */

#ifndef DATARECORDER_HH_
#define DATARECORDER_HH_

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Types.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"

#include "TROOT.h"
#include "TApplication.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include <TGClient.h>

class DataRecorder: public CxxUtilities::StoppableThread {
private:
	TTree* tree;
	TH1D* histograms[SpaceWireADCBox::NumberOfChannels];
	std::vector<Int_t>* waveforms[SpaceWireADCBox::NumberOfChannels];
	TCanvas* histogramcanvas;
	TCanvas* waveformcanvas;
	TGraph* graph;

private:
	//event data
	uint32_t flag_fff0;
	uint32_t chid;
	uint32_t consumerid;
	uint32_t pha_max;
	uint32_t time_H;
	uint32_t time_M;
	uint32_t time_L;
	uint32_t baseline;
	uint32_t risetime_ld_time;
	uint32_t risetime_ud_time;
	uint32_t flag_fff1;
	//uint32_t pha_list[1024];//not used now
	uint32_t flag_fff2;
	uint32_t flag_ffff;

	uint32_t pha_list_size;
	uint32_t last_pha_list_size[SpaceWireADCBox::NumberOfChannels];

private:
	enum {
		state_flag_fff0,
		state_chid,
		state_consumerid,
		state_pha_max,
		state_time_H,
		state_time_M,
		state_time_L,
		state_baseline,
		state_risetime_ld_time,
		state_risetime_ud_time,
		state_flag_fff1,
		state_pha_list,
		state_flag_fff2,
		state_flag_ffff
	} event_packet_interpretation_state;

private:
	int state;
	SpaceWireADCBox* adcbox;
	CxxUtilities::Mutex mutex;
	CxxUtilities::Mutex treemutex;
	CxxUtilities::Mutex waveformmutex;
	CxxUtilities::Mutex drawmutex;
	//RMAPInitiator* rmapInitiator;
	ConsumerManager* consumermanager;

public:
	/** Constructor. Automatically calls
	 * createContainerInstances() inside.
	 */
	DataRecorder() :
			CxxUtilities::StoppableThread() {
		histogramcanvas = new TCanvas("histogramcanvas", "Histogram Canvas", 1200, 700);
		histogramcanvas->Draw();
		waveformcanvas = new TCanvas("waveformcanvas", "Waveform Canvas", 500, 500);
		waveformcanvas->Draw();
		mutex.lock();
		tree = NULL;
		initialize();
		mutex.unlock();
	}
	virtual ~DataRecorder() {
	}

	/** Initializes internal variables.
	 */
	void initialize() {
		createContainerInstances();
		histogramcanvas->Divide(4, 2);
		histogramcanvas->SetHighLightColor(10);
		drawHistograms();
		state = state_flag_fff0;
	}

	/** Sets SpaceWireADCBox instance to be accessed from
	 * this instance. Based on the RMAPSocket instance
	 * which is used in the SpaceWireADCBox instance,
	 * this method creates new RMAPSocket connected to
	 * the ADCBox. Data readout is done via the created
	 * RMAPSocket.
	 * param adcbox a pointer to an instance of SpaceWireADCBox
	 */
	void setADCBox(SpaceWireADCBox* adcbox) {
		mutex.lock();
		this->adcbox = adcbox;

		if (consumermanager != NULL) {
			delete consumermanager;
		}
		consumermanager = adcbox->getConsumerManager();
		mutex.unlock();
	}

	/** Returns ConsumerManager.
	 * @return a pointer to ConsumerManager instance used in this instance
	 */
	ConsumerManager* getConsumerManager() {
		return consumermanager;
	}

	/** Creates data container instances.
	 */
	void createContainerInstances() {
		using namespace std;
		treemutex.lock();
		tree = new TTree("eventtree", "Event Tree");
		tree->Branch("chid", &chid, "chid/I");
		tree->Branch("consumerid", &consumerid, "consumerid/I");
		tree->Branch("pha_max", &pha_max, "pha_max/I");
		tree->Branch("time_H", &time_H, "time_H/I");
		tree->Branch("time_M", &time_M, "time_M/I");
		tree->Branch("time_L", &time_L, "time_L/I");
		tree->Branch("baseline", &baseline, "baseline/I");
		tree->Branch("pha_list_size", &pha_list_size, "pha_list_size/I");

		for (size_t i = 0; i < SpaceWireADCBox::NumberOfChannels; i++) {
			stringstream ss, tt;
			ss << "histogram" << i;
			tt << "Histogram of Ch." << i;
			histograms[i] = new TH1D(ss.str().c_str(), tt.str().c_str(), 4096, -0.5, 4095.5);
			histograms[i]->SetXTitle("ADC ch");
			histograms[i]->SetYTitle("Counts");

			last_pha_list_size[i] = 0;
			waveforms[i] = new vector<Int_t>(1024, 0x800);
		}
		tree->Branch("pha_list", &(waveforms[0]->at(0)), "pha_list[1024]/I");

		graph = new TGraph();
		graph->SetEditable(false);
		graph->GetXaxis()->SetTitle("Samples");
		graph->GetYaxis()->SetTitle("ADC ch");

		treemutex.unlock();
	}

	/** Deletes instances of TTree and TH1D.
	 */
	void clear() {
		treemutex.lock();
		delete tree;
		tree = NULL;
		for (size_t i = 0; i < SpaceWireADCBox::NumberOfChannels; i++) {
			delete histograms[i];
			delete waveforms[i];
		}
		delete graph;
		treemutex.unlock();
	}

	/** Saves Tree.
	 */
	void saveTree(std::string filename) {
		using namespace std;
		treemutex.lock();
		TFile file(filename.c_str(), "recreate");
		tree->Write();
		treemutex.unlock();
	}

	/** Saves Histograms.
	 */
	void saveHistograms(std::string filename) {
		using namespace std;
		treemutex.lock();
		TFile file(filename.c_str(), "recreate");
		for (size_t i = 0; i < SpaceWireADCBox::NumberOfChannels; i++) {
			histograms[i]->Write();
		}
		treemutex.unlock();
	}

	/** Returns the number of entries in tree.
	 * @return the number oevents stored in event tree
	 */
	Long64_t getNumberOfEvents() {
		if (tree != NULL) {
			treemutex.lock();
			uint32_t n = tree->GetEntries();
			treemutex.unlock();
			return tree->GetEntries();
		} else {
			return 0;
		}
	}

private:
	bool resumed;


public:
	/** Starts readout.
	 */
	void resume() {
		resumed  = false;
	}

	/** Stops read out.
	 */
	void stop() {
		resumed = true;
	}

	/** Returns WritePointer.
	 */
	uint32_t getWritePointer() {
		mutex.lock();
		uint32_t writepointer = consumermanager->getWritePointer();
		mutex.unlock();
		return writepointer;
	}

	/** Returns ReadPointer.
	 */
	uint32_t getReadPointer() {
		uint32_t readpointer = consumermanager->getReadPointer();
		return readpointer;
	}

	/** Draws histogram.
	 * param channelnumber the number of channel to be plotted
	 */
	void drawHistogram(uint32_t channelnumber) {
		drawmutex.lock();
		histogramcanvas->cd(0);
		histograms[channelnumber]->Draw();
		histogramcanvas->Update();
		drawmutex.unlock();
	}

	/** Draws histograms.
	 */
	void drawHistograms() {
		using namespace std;
		drawmutex.lock();
		for (size_t i = 0; i < SpaceWireADCBox::NumberOfChannels; i++) {
			histogramcanvas->cd(i + 1);
			histograms[i]->Draw();
			histogramcanvas->Update();
		}
		if (Debug::datarecorder()) {
			cout << "DataRecorder::drawHistograms()" << endl;
		}
		drawmutex.unlock();
	}

	/** Draws the latest Waveform.
	 */
	void drawWaveform(uint32_t channelnumber) {
		using namespace std;
		if (last_pha_list_size[channelnumber] != 0) {
			drawmutex.lock();
			waveformmutex.lock();
			//set graph data points
			for (size_t i = 0; i < last_pha_list_size[channelnumber]; i++) {
				graph->SetPoint(i, i, (*waveforms[channelnumber])[i]);
			}
			waveformcanvas->cd(0);
			stringstream ss;
			ss << "Waveform Ch." << channelnumber;
			graph->SetTitle(ss.str().c_str());
			graph->Draw("a*c");
			waveformcanvas->Update();
			if (Debug::datarecorder()) {
				cout << "DataRecorder::drawWaveform() drew " << last_pha_list_size << " samples" << endl;
			}
			waveformmutex.unlock();
			drawmutex.unlock();
		}
	}

	void run() {
		using namespace std;
		vector<uint8_t> tmp;
		vector<uint32_t> readdata;
		cout << "DataRecorder::run() starting thread" << endl;
		resumed=false;
		while (!stopped) {

			//wait sequence
			while (resumed==true) {
				sleep(500); //wait for started
			}

			//wait until SpaceWire/RMAP communication with SpaceWireADCBox is established
			if (!this->adcbox->getRMAPHandler()->isConnectedToSpWGbE()){
				continue;
			}

			//read-out sequence
			mutex.lock();
			tmp = consumermanager->read();
			mutex.unlock();
			size_t size = tmp.size();
			size_t size_half = size / 2;
			if (Debug::datarecorder()) {
				cout << "DataRecorder::run() read " << size << " bytes" << endl;
			}
			if (size == 0) {
				sleep(500);
			}
			readdata.resize(size_half);
			for (size_t i = 0; i < size_half; i++) {
				readdata[i] = tmp[2 * i + 1] * 0x100 + tmp[2 * i];
			}
			treemutex.lock();
			for (size_t i = 0; i < size_half; i++) {
				switch (state) {
				case state_flag_fff0:
					pha_list_size = 0;
					if (readdata[i] == 0xfff0) {
						state = state_chid;
					}
					break;
				case state_chid:
					chid = readdata[i];
					state = state_consumerid;
					break;
				case state_consumerid:
					consumerid = readdata[i];
					state = state_pha_max;
					break;
				case state_pha_max:
					pha_max = readdata[i];
					state = state_time_H;
					break;
				case state_time_H:
					time_H = readdata[i];
					state = state_time_M;
					break;
				case state_time_M:
					time_M = readdata[i];
					state = state_time_L;
					break;
				case state_time_L:
					time_L = readdata[i];
					state = state_baseline;
					break;
				case state_baseline:
					baseline = readdata[i];
					state = state_risetime_ld_time;
					break;
				case state_risetime_ld_time:
					state = state_risetime_ud_time;
					break;
				case state_risetime_ud_time:
					state = state_flag_fff1;
					break;
				case state_flag_fff1:
					if (readdata[i] == 0xfff1) {
						state = state_pha_list;
						waveformmutex.lock();
					}
					break;
				case state_pha_list:
					if (readdata[i] == 0xfff2) {
						state = state_flag_ffff;
						last_pha_list_size[chid] = pha_list_size;
						waveformmutex.unlock();
					} else {
						(*waveforms[chid])[pha_list_size] = readdata[i];
						pha_list_size++;
					}
					break;
				case state_flag_ffff:
					state = state_flag_fff0;
					//Fill histogram
					if (0 <= chid && chid < SpaceWireADCBox::NumberOfChannels) {
						tree->SetBranchAddress("pha_list", &(waveforms[chid]->at(0)));
						histograms[chid]->Fill(pha_max);
					}
					//Fill tree
					tree->Fill();
					break;
				}
			}
			treemutex.unlock();
		}
	}
};

#endif /* DATARECORDER_HH_ */
