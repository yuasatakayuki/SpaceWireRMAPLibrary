//============================================================================
// Name        : RMAPHandler.hpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#ifndef RMAPHANDLER_HH_
#define RMAPHANDLER_HH_

#include "SpaceWireRMAPLibrary/SpaceWire.hh"
#include "SpaceWireRMAPLibrary/RMAP.hh"
#include "CxxUtilities/CxxUtilities.hh"

#include <iostream>
#include <fstream>
#include <vector>

class RMAPHandler {
private:
	RMAPTargetNodeDB rmapTargetDB;
	std::string ipAddress;
	uint32_t tcpPortNumber;
	//std::string xmlFileName;

	SpaceWireIFOverTCP* spwif;
	RMAPEngine* rmapEngine;
	RMAPInitiator* rmapInitiator;
	RMAPTargetNode* adcRMAPTargetNode;

	double timeOutDuration;
	int maxNTrials;
	bool useDraftECRC = false;
	bool _isConnectedToSpWGbE;

public:
	static constexpr double DefaultTimeOut = 1000; //ms

public:
	RMAPHandler(std::string ipAddress, uint32_t tcpPortNumber = 10030,
			std::vector<RMAPTargetNode*> rmapTargetNodes = { }) {
		using namespace std;
		this->ipAddress = ipAddress;
		this->tcpPortNumber = tcpPortNumber;
		//this->xmlFileName = "";
		this->timeOutDuration = 1000.0;
		this->maxNTrials = 10;
		this->useDraftECRC = false;
		this->spwif = NULL;
		this->rmapEngine = NULL;
		this->rmapInitiator = NULL;
		this->setTimeOutDuration(DefaultTimeOut);

		//add RMAPTargetNodes to DB
		for (auto& rmapTargetNode : rmapTargetNodes) {
			rmapTargetDB.addRMAPTargetNode(rmapTargetNode);
		}

		adcRMAPTargetNode = rmapTargetDB.getRMAPTargetNode("ADCBox");
		if (adcRMAPTargetNode == NULL) {
			cerr << "RMAPHandler::RMAPHandler(): ADCBox RMAP Target Node not found" << endl;
			exit(-1);
		}
	}

public:
	~RMAPHandler() {
	}

public:
	bool isConnectedToSpWGbE() {
		return _isConnectedToSpWGbE;
	}

public:
	bool connectoToSpaceWireToGigabitEther() {
		using namespace std;
		if (spwif != NULL) {
			delete spwif;
		}

		//connect to SpaceWire-to-GigabitEther
		spwif = new SpaceWireIFOverTCP(ipAddress, tcpPortNumber);
		try {
			spwif->open();
			_isConnectedToSpWGbE = true;
		} catch (...) {
			cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() connection failed" << endl;
			_isConnectedToSpWGbE = false;
			return false;
		}

		//start RMAPEngine
		rmapEngine = new RMAPEngine(spwif);
		if (useDraftECRC) {
			cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPEngine" << endl;
			rmapEngine->setUseDraftECRC(true);
		}
		rmapEngine->start();
		rmapInitiator = new RMAPInitiator(rmapEngine);
		rmapInitiator->setInitiatorLogicalAddress(0xFE);
		rmapInitiator->setVerifyMode(true);
		rmapInitiator->setReplyMode(true);
		if (useDraftECRC) {
			rmapInitiator->setUseDraftECRC(true);
			cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPInitiator" << endl;
		}

		return true;
	}

public:
	void disconnectSpWGbE() {
		_isConnectedToSpWGbE = false;

		spwif->close();
		rmapEngine->stop();
		delete rmapEngine;
		delete rmapInitiator;
		delete spwif;

		spwif = NULL;
		rmapEngine = NULL;
		rmapInitiator = NULL;
	}

public:
	std::string getIPAddress() {
		return ipAddress;
	}

public:
	uint32_t getIPPortNumber() {
		return tcpPortNumber;
	}

	/*
	 public:
	 std::string getXMLFilename() {
	 return xmlFileName;
	 }*/

public:
	RMAPInitiator* getRMAPInitiator() {
		return rmapInitiator;
	}

public:
	void setDraftECRC(bool useECRC = true) {
		useDraftECRC = useECRC;
		if (rmapInitiator != NULL && rmapEngine != NULL) {
			rmapEngine->setUseDraftECRC(useDraftECRC);
			rmapInitiator->setUseDraftECRC(useDraftECRC);
		}
	}

	/*
	 public:
	 void loadRMAPTargetNodesFromXMLFile(std::string XMLFileName) {
	 xmlFileName = XMLFileName;
	 if (xmlFileName == "") {
	 throw RMAPHandlerException(RMAPHandlerException::NoSuchFile);
	 }

	 try {
	 rmapTargetDB.loadRMAPTargetNodesFromXMLFile(xmlFileName);
	 } catch (...) {
	 throw RMAPHandlerException(RMAPHandlerException::TargetLoadFailed);
	 }
	 }
	 */
public:
	void setTimeOutDuration(double msec) {
		timeOutDuration = msec;
	}

public:
	void read(std::string rmapTargetNodeID, uint32_t memoryAddress, uint32_t length, uint8_t *buffer) {
		RMAPTargetNode* targetNode;
		try {
			targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
		} catch (...) {
			throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
		}
		read(targetNode, memoryAddress, length, buffer);
	}

public:
	void read(std::string rmapTargetNodeID, std::string memoryObjectID, uint8_t *buffer) {
		RMAPTargetNode* targetNode;
		try {
			targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
		} catch (...) {
			std::cout << "NoSuchTarget" << std::endl;
			throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
		}
		read(targetNode, memoryObjectID, buffer);
	}

public:
	void read(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint32_t length, uint8_t *buffer) {
		using namespace std;
		if (rmapInitiator == NULL) {
			return;
		}
		for (size_t i = 0; i < maxNTrials; i++) {
			try {
				rmapInitiator->read(rmapTargetNode, memoryAddress, length, buffer, timeOutDuration);
				spwif->emitTimecode(0x00, 0x00);
				break;
			} catch (RMAPInitiatorException& e) {
				std::cerr << "Read timed out (address=" << "0x" << hex << right << setw(8) << setfill('0')
						<< (uint32_t) memoryAddress << " length=" << dec << length << "); trying again..." << std::endl;
				if (i == maxNTrials - 1) {
					if (e.getStatus() == RMAPInitiatorException::Timeout) {
						throw RMAPHandlerException(RMAPHandlerException::TimeOut);
					} else {
						throw RMAPHandlerException(RMAPHandlerException::LowerException);
					}
				}
				usleep(100);
			}
		}
		//	cout << "Read successfully done." << endl;
	}

public:
	void read(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, uint8_t *buffer) {
		if (rmapInitiator == NULL) {
			return;
		}
		for (size_t i = 0; i < maxNTrials; i++) {
			try {
				rmapInitiator->read(rmapTargetNode, memoryObjectID, buffer, timeOutDuration);
				spwif->emitTimecode(0x00, 0x00);
				break;
			} catch (RMAPInitiatorException& e) {
				std::cerr << "Time out; trying again..." << std::endl;
				if (i == maxNTrials - 1) {
					if (e.getStatus() == RMAPInitiatorException::Timeout) {
						throw RMAPHandlerException(RMAPHandlerException::TimeOut);
					} else {
						throw RMAPHandlerException(RMAPHandlerException::LowerException);
					}
				}
				usleep(100);
			} catch (RMAPReplyException& e) {
				std::cerr << "RMAPReplyException" << std::endl;
				throw RMAPHandlerException(RMAPHandlerException::LowerException);
			}
		}
	}

public:
	void write(std::string rmapTargetNodeID, uint32_t memoryAddress, uint8_t *data, uint32_t length) {
		RMAPTargetNode* targetNode;
		try {
			targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
		} catch (...) {
			throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
		}
		write(targetNode, memoryAddress, data, length);
	}

public:
	void write(std::string rmapTargetNodeID, std::string memoryObjectID, uint8_t* data) {
		RMAPTargetNode* targetNode;
		try {
			targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
		} catch (...) {
			throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
		}
		write(targetNode, memoryObjectID, data);
	}

public:
	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, uint8_t *data, uint32_t length) {
		if (rmapInitiator == NULL) {
			return;
		}
		for (size_t i = 0; i < maxNTrials; i++) {
			try {
				if (length != 0) {
					rmapInitiator->write(rmapTargetNode, memoryAddress, data, length, timeOutDuration);
					spwif->emitTimecode(0x00, 0x00);
				} else {
					rmapInitiator->write(rmapTargetNode, memoryAddress, (uint8_t*) NULL, (uint32_t) 0, timeOutDuration);
					spwif->emitTimecode(0x00, 0x00);
				}
				break;
			} catch (RMAPInitiatorException& e) {
				std::cerr << "Time out; trying again..." << std::endl;
				if (i == maxNTrials - 1) {
					if (e.getStatus() == RMAPInitiatorException::Timeout) {
						throw RMAPHandlerException(RMAPHandlerException::TimeOut);
					} else {
						throw RMAPHandlerException(RMAPHandlerException::LowerException);
					}
				}
				usleep(100);
			}
		}
	}

public:
	void write(RMAPTargetNode *rmapTargetNode, std::string memoryObjectID, uint8_t* data) {
		if (rmapInitiator == NULL) {
			return;
		}
		for (size_t i = 0; i < maxNTrials; i++) {
			try {
				if (1) {
					rmapInitiator->write(rmapTargetNode, memoryObjectID, data, timeOutDuration);
					spwif->emitTimecode(0x00, 0x00);
				} else {
					rmapInitiator->write(rmapTargetNode, memoryObjectID, (uint8_t*) NULL, timeOutDuration);
					spwif->emitTimecode(0x00, 0x00);
				}
			} catch (RMAPInitiatorException& e) {
				std::cerr << "Time out; trying again..." << std::endl;
				if (i == maxNTrials - 1) {
					if (e.getStatus() == RMAPInitiatorException::Timeout) {
						throw RMAPHandlerException(RMAPHandlerException::TimeOut);
					} else {
						throw RMAPHandlerException(RMAPHandlerException::LowerException);
					}
				}
				usleep(100);
			}
		}
	}

public:
	RMAPTargetNode* getRMAPTargetNode(std::string rmapTargetNodeID) {
		RMAPTargetNode* targetNode;
		try {
			targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
		} catch (...) {
			throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
		}
		return targetNode;
	}

public:
	void setRegister(uint32_t address, uint16_t data) {
		uint8_t writeData[2] = { static_cast<uint8_t>(data / 0x100), static_cast<uint8_t>(data % 0x100) };
		this->write(adcRMAPTargetNode, address, writeData, 2);
	}

public:
	uint16_t getRegister(uint32_t address) {
		uint8_t readData[2];
		this->read(adcRMAPTargetNode, address, 2, readData);
		return (uint16_t) (readData[0] * 0x100 + readData[1]);
	}

	/*
	 public:
	 RMAPMemoryObject* getMemoryObject(std::string rmapTargetNodeID, std::string memoryObjectID) {
	 RMAPTargetNode* targetNode = getRMAPTargetNode(rmapTargetNodeID);
	 return targetNode->getMemoryObject(memoryObjectID);
	 }*/

public:
	RMAPEngine* getRMAPEngine() {
		return rmapEngine;
	}

public:
	class RMAPHandlerException {
	public:
		RMAPHandlerException(int type) {
			std::string message;
			switch (type) {
			case NoSuchFile:
				message = "NoSuchFile";
				break;
			case TargetLoadFailed:
				message = "TargetLoadFailed";
				break;
			case NoSuchTarget:
				message = "NoSuchTarget";
				break;
			case TimeOut:
				message = "TimeOut";
				break;
			case CouldNotConnect:
				message = "CouldNotConnect";
				break;
			case LowerException:
				message = "LowerException";
				break;
			default:
				break;
			}
			std::cout << message << std::endl;
		}

		enum {
			NoSuchFile, TargetLoadFailed, NoSuchTarget, TimeOut, CouldNotConnect, LowerException
		};
	};
};

#endif
