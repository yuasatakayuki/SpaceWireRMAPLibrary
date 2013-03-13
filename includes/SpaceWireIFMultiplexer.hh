/* 
============================================================================
SpaceWire/RMAP Library is provided under the MIT License.
============================================================================

Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
/*
 * SpaceWireIFMultiplexer.hh
 *
 *  Created on: Nov 1, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIFMULTIPLEXER_HH_
#define SPACEWIREIFMULTIPLEXER_HH_

#include "SpaceWireIFMultiplexerSuperClass.hh"
#include "SpaceWireIF.hh"
#include "SpaceWireIFMultiplexedIF.hh"
#include "CxxUtilities/CommonHeader.hh"

class SpaceWireIFMultiplexerException: public CxxUtilities::Exception {
public:
	enum {
		NoSuchVirtualSpaceWireIFRegistered, NotImplemented
	};
};

class SpaceWireIFMultiplexer: public SpaceWireIF,
		public CxxUtilities::StoppableThread,
		public SpaceWireIFMultiplexerSuperClass,
		public SpaceWireIFActionTimecodeScynchronizedAction {
private:
	SpaceWireIF** spwifs;
	SpaceWireIF** spwifs_default;
	std::list<SpaceWireIF*> spwif_list;
	std::map<std::string, SpaceWireIF*> spwif_map;
	std::map<SpaceWireIF*, std::string> spwif_name_map;
	std::map<SpaceWireIF*, std::vector<uint8_t> > acceptableProtocolIDMap;
	std::map<SpaceWireIF*, std::vector<uint8_t>* > receiving_spwif_map;
	CxxUtilities::Mutex receiving_spwif_map_mutex;
	SpaceWireIF* realSpaceWireIF;
	CxxUtilities::Mutex sendMutex;
	CxxUtilities::Mutex receiveMutex;
	CxxUtilities::Mutex emitTimecodeMutex;
	std::list<SpaceWireIF*> spwif_receivewait_list;
	bool isDefaultSpaceWireIFSet;
	SpaceWireIF* defaultSpaceWireIF;

public:
	static const double WaitDurationInMilliSecForReceiveIFSearch=10;//ms

public:
	int nReceivedPackets;
	int nEmptyPacket;

public:
	SpaceWireIFMultiplexer(SpaceWireIF* realSpaceWireIF) {
		this->realSpaceWireIF = realSpaceWireIF;
		this->spwifs=(SpaceWireIF**)malloc(256*sizeof(SpaceWireIF*));
		this->spwifs_default=(SpaceWireIF**)malloc(256*sizeof(SpaceWireIF*));
		this->isDefaultSpaceWireIFSet=false;
		this->defaultSpaceWireIF=NULL;
		realSpaceWireIF->addTimecodeAction(this);
		nReceivedPackets=0;
		nEmptyPacket=0;
	}

	~SpaceWireIFMultiplexer(){
		free(this->spwifs);
		free(this->spwifs_default);
	}

	SpaceWireIF* getRealSpaceWireIF() {
		return realSpaceWireIF;
	}

	SpaceWireIF* getDefaultSpaceWireIF(){
		return defaultSpaceWireIF;
	}

	void setDefaultVirtualSpaceWireIF(SpaceWireIF* spwif){
		for(size_t i=0;i<256;i++){
			spwifs[i]=spwif;
			spwifs_default[i]=spwif;
		}
		isDefaultSpaceWireIFSet=true;
		defaultSpaceWireIF=spwif;
	}

	void addVirtuaSpaceWirelIF(SpaceWireIF* spwif, std::vector<uint8_t> acceptableProtocolIDs,std::string name="") {
		if(isDefaultSpaceWireIFSet==false){
			setDefaultVirtualSpaceWireIF(spwif);
		}
		spwif_list.push_back(spwif);
		spwif_map[name] = spwif;
		acceptableProtocolIDMap[spwif] = acceptableProtocolIDMap;
		for(size_t i=0;i<acceptableProtocolIDMap.size();i++){
			spwifs[acceptableProtocolIDMap[i]]=spwif;
		}
	}

	SpaceWireIFMultiplexedIF* createVirtualSpaceWireIF(std::vector<uint8_t> acceptableProtocolIDs,std::string name="") {
		SpaceWireIFMultiplexedIF* spwif = new SpaceWireIFMultiplexedIF(this);
		addVirtualIF(acceptableProtocolIDs,name);
	}

	void removeVirtualIF(SpaceWireIF* spwif) throw (SpaceWireIFMultiplexerException) {
		using namespace std;
		std::list<SpaceWireIF*>::iterator it_spwif_list = spwif_list.find(spwif);
		std::map<SpaceWireIF*, std::string>::iterator it_spwif_name_map = spwif_name_map.find(spwif);
		if (it_spwif_list == spwif_list.end()) {
			throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NoSuchVirtualSpaceWireIFRegistered);
		}
		if (it_spwif_name_map == spwif_name_map.end()) {
			throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NoSuchVirtualSpaceWireIFRegistered);
		}

		std::map<std::string, SpaceWireIF*>::iterator it_spwif_map = spwif_map.find(it_spwif_name_map->second);
		if (it_spwif_map == spwif_map.end()) {
			throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NoSuchVirtualSpaceWireIFRegistered);
		}

		std::map<SpaceWireIF*, std::vector<uint8_t> >::iterator it_acceptableProtocolIDMap =
				acceptableProtocolIDMap.find(spwif);
		std::map<SpaceWireIF*, std::vector<uint8_t> >::iterator it_unacceptableProtocolIDMap =
				unacceptableProtocolIDMap.find(spwif);
		if (it_acceptableProtocolIDMap == acceptableProtocolIDMap.end()) {
			throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NoSuchVirtualSpaceWireIFRegistered);
		}
		if (it_unacceptableProtocolIDMap == unacceptableProtocolIDMap.end()) {
			throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NoSuchVirtualSpaceWireIFRegistered);
		}

		//revert SpaceWireIF array
		std::vector<uint8_t> acceptableProtocolIDs=it_acceptableProtocolIDMap->second;
		for(size_t i=0;i<acceptableProtocolIDs.size();i++){
			spwifs[i]=spwifs_default[i];
		}

		spwif_list.erase(it_spwif_list);
		spwif_name_map.erase(it_spwif_name_map);
		spwif_map.erase(it_spwif_map);
		acceptableProtocolIDMap.erase(it_acceptableProtocolIDMap);
	}

public:
	void send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) {
		sendMutex.lock();
		realSpaceWireIF->send(data, length, eopType);
		sendMutex.unlock();
	}

public:
	void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) {
		throw SpaceWireIFMultiplexerException(SpaceWireIFMultiplexerException::NotImplemented);
	}

	virtual void receive(std::vector<uint8_t>* buffer, SpaceWireIF* spwif) throw (SpaceWireIFException) {
		receiving_spwif_map_mutex.lock();
		receiving_spwif_map[spwif]=buffer;
		receiving_spwif_map_mutex.unlock();
	}

public:
	virtual void cancelReceive(SpaceWireIF* spwif) throw (SpaceWireIFException) {
		receiving_spwif_map_mutex.lock();
		//delete spwif from receive_spwif_map
		map<SpaceWireIF*,std::vector<uint8_t>*>::iterator it=receiving_spwif_map.find(receiving_spwif);
		if(it!=receiving_spwif_map.end()){
			receiving_spwif_map.erase(it);
		}
		receiving_spwif_map_mutex.unlock();
	}

public:
	void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) {
		realSpaceWireIF->emitTimecode(timeIn, controlFlagIn);
	}

	virtual void setTxLinkRate(uint32_t linkRateType) throw (SpaceWireIFException) {
		realSpaceWireIF->setTxLinkRate(linkRateType);
	}

	virtual uint32_t getTxLinkRateType() throw (SpaceWireIFException) {
		return realSpaceWireIF->getTxLinkRateType();
	}

public:
	void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) {
		realSpaceWireIF->setTimeoutDuration(microsecond);
	}

public:
	uint8_t getTimeCode() throw (SpaceWireIFException) {
		return realSpaceWireIF->getTimeCode();
	}

	void doAction(uint8_t timecode) {
		this->invokeTimecodeSynchronizedActions(timecode);
	}

public:
	void run() {
		stopped = false;
		while_loop: while (!stopped) {
			std::vector<uint8_t> buffer = new std::vector<uint8_t>;
			try {
				realSpaceWireIF->receive(buffer);
			} catch (SpaceWireIFException e) {
				if (e.getStatus() == SpaceWireIFException::Timeout) {
					goto while_loop;
				} else {
					this->stop();
				}
			}
			//search SpaceWire IF that should receive this packet
			if(buffer->size()!=0){
				//find protocolID
				uint8_t logicalAddress;
				static const int state_path_address=0;
				static const int state_logical_address=1;
				static const int state_protocol_id=2;
				static const int state_cargo=3;
				int state=state_path_address;
				uint8_t protocolID=0x00;
				for(size_t i=0;i<buffer->size();i++){
					if(state==state_path_address){
						if(buffer->at(i)>=0x20){
							state=state_logical_address;
						}
					}else if(state==state_logical_address){
						if(i!=buffer->size()-1){
							state=state_protocol_id;
						}
					}else if(state==state_protocol_id){
						protocolID=buffer->at(i);
						state=state_cargo;
					}else{
						break;
					}
				}
				if(state==state_cargo){//protocol id has been found
					waitUntilVirtualIFReceives(buffer,spwifs[protocolID]);
				}else{//no protocol id. the packet should be passed to the default SpaceWireIF
					waitUntilVirtualIFReceives(buffer,defaultSpaceWireIF);
				}
				nReceivedPackets++;
			}else{
				nEmptyPacket++;
			}
		}

	}

public:
	void waitUntilVirtualIFReceives(std::vector<uint8_t>* packet,SpaceWireIF* receiving_spwif){
		using namespace std;
		using namespace CxxUtilities;
		Condition c;
		while(true){//?? kokode packet wo haki suruka?
			//find buffer
			map<SpaceWireIF*,std::vector<uint8_t>* >::iterator it=receiving_spwif_map.find(receiving_spwif_map);
			if(it!=receiving_spwif_map.end()){//found
				*(it->second)=*packet;
				delete packet;
			}else{//not registered as receiving IF
				c.wait(WaitDurationInMilliSecForReceiveIFSearch);
			}
		}
		//set eop type
		receiving_spwif->setReceivedPacketEOPMarkerType(realSpaceWireIF->getReceivedPacketEOPMarkerType());
		//delete spwif from receive request map
		cancelReceive(receiving_spwif);
	}

public:
	void doAction(unsigned char timeOut){
		using namespace std;
		list<SpaceWireIF*>::iterator it=spwif_list.begin();
		while(it!=spwif_list.end()){
			it->second->doAction(timeOut);
			it++;
		}
	}
};

#endif /* SPACEWIREIFMULTIPLEXER_HH_ */
