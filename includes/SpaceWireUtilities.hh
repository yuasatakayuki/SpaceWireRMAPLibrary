/*
 * SpaceWireUtilities.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREUTILITIES_HH_
#define SPACEWIREUTILITIES_HH_

#include "CxxUtilities/CxxUtilities.hh"

class SpaceWireUtilitiesException: public CxxUtilities::Exception {
public:
	enum {
		SizeIncorrect
	};
public:
	SpaceWireUtilitiesException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}
};

class SpaceWireUtilities {
public:
	static std::vector<uint8_t> concatenate(std::vector<uint8_t>& array1, std::vector<uint8_t>& array2) {
		std::vector<uint8_t> result;
		for (size_t i = 0; i < array1.size(); i++) {
			result.push_back(array1[i]);
		}
		for (size_t i = 0; i < array2.size(); i++) {
			result.push_back(array2[i]);
		}
		return result;
	}

	static void concatenateTo(std::vector<uint8_t>& array1, std::vector<uint8_t>& array2) {
		using namespace std;
		for (size_t i = 0; i < array2.size(); i++) {
			array1.push_back(array2[i]);
		}
	}

public:
	static void dumpPacket(std::vector<uint8_t>& data) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, &data, 1);
	}

	static void dumpPacket(std::vector<uint8_t>* data) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, data, 1);
	}

	static void dumpPacket(std::ostream* ofs, std::vector<uint8_t>* data) throw (SpaceWireUtilitiesException) {
		dumpPacket(ofs, data, 1);
	}

	static void dumpPacket(std::vector<uint8_t>* data, uint32_t wordwidth) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, data, wordwidth);
	}

	static void dumpPacket(std::ostream* ofs, std::vector<uint8_t>* data, uint32_t wordwidth,uint32_t bytesPerLine=SpaceWireUtilities::DumpsPerLine)
			throw (SpaceWireUtilitiesException) {
		using namespace std;
		int v;
		if (wordwidth == 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		if (data->size() % wordwidth != 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		for (uint32_t i = 0; i < (data->size() / wordwidth + bytesPerLine - 1)
				/ bytesPerLine; i++) {
			for (uint32_t o = 0; o < bytesPerLine; o++) {
				if (i * bytesPerLine + o < data->size() / wordwidth) {
					v = 0;
					for (uint32_t p = 0; p < wordwidth; p++) {
						v = v + (uint32_t) data->at((i * bytesPerLine + o) * wordwidth + p);
					}
					(*ofs) << "0x" << right << hex << setw(2 * wordwidth) << setfill('0') << v << " ";
				}
			}
			(*ofs) << endl;
		}
	}

	static std::string packetToString(std::vector<uint8_t>* data) throw (SpaceWireUtilitiesException) {
		using namespace std;
		if(data->size()==0){
			return "";
		}
		stringstream ss;
		for (size_t i = 0; i < data->size(); i++) {
			ss << "0x" << right << hex << setw(2) << setfill('0') << (uint32_t)data->at(i);
			if(i!=data->size()-1){
				ss  << "  ";
			}
		}
		return ss.str();
	}

	static uint8_t convertStringToUnsignedChar(std::string str) {
		using namespace std;
		int data = 0;
		stringstream ss;
		if ((str.at(0) == '0' && str.at(1) == 'x') || (str.at(0) == '0' && str.at(1) == 'X')) {
			str = str.substr(2);
		}
		ss << str;
		ss >> hex >> data;
		return (uint8_t) data;
	}

	static void printVectorWithAddress(uint32_t address, std::vector<uint8_t>* data) {
		using namespace std;
		cout << "Address    Data" << endl;
		for (uint32_t i = 0; i < data->size(); i++) {
			cout << setfill('0') << setw(4) << hex << (address + i) / 0x00010000;
			cout << "-" << setfill('0') << setw(4) << hex << (address + i) % 0x00010000;
			cout << "  ";
			cout << "0x" << setfill('0') << setw(2) << hex << (uint32_t) (data->at(i)) << endl;
		}
		cout << dec << left;
	}

	static void printVectorWithAddress2bytes(uint32_t address, std::vector<uint8_t>* data) {
		using namespace std;
		cout << "Address    Data" << endl;
		uint32_t size = data->size();
		if (size % 2 == 0 && size != 0) {
			for (uint32_t i = 0; i < (size + 1) / 2; i++) {
				cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
				cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
				cout << "  ";
				cout << "0x" << setfill('0') << setw(4) << hex << (uint32_t) (data->at(i * 2 + 1) * 0x100 + data->at(i
						* 2)) << endl;
			}
		} else if (size != 0) {
			uint32_t i;
			for (i = 0; i < size / 2; i++) {
				cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
				cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
				cout << "  ";
				cout << "0x" << setfill('0') << setw(4) << hex << (uint32_t) (data->at(i * 2 + 1) * 0x100 + data->at(i
						* 2)) << endl;
			}
			cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
			cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
			cout << "  ";
			cout << "0x??" << setfill('0') << setw(4) << hex << (uint32_t) (data->at(i * 2)) << endl;
		}
		cout << dec << left;
	}

public:
	static const uint32_t DumpsPerLine = 8;
};
#endif /* SPACEWIREUTILITIES_HH_ */
