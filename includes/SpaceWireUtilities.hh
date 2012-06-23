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
	static void dumpPacketUnsignedChar(std::vector<unsigned char>& data) throw (SpaceWireUtilitiesException) {
		std::vector < uint8_t > data2(data.size());
		for (size_t i = 0; i < data.size(); i++) {
			data2[i] = data[i];
		}
		dumpPacket(&std::cout, &data2, 1);
	}

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

	static void dumpPacket(std::ostream* ofs, std::vector<uint8_t>* data, uint32_t wordwidth, uint32_t bytesPerLine =
			SpaceWireUtilities::DumpsPerLine) throw (SpaceWireUtilitiesException) {
		using namespace std;
		int v;
		if (wordwidth == 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		if (data->size() > wordwidth && data->size() % wordwidth != 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		for (uint32_t i = 0; i < (data->size() / wordwidth + bytesPerLine - 1) / bytesPerLine; i++) {
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

	static void dumpPacket(std::ostream* ofs, uint8_t* data, size_t length, uint32_t wordwidth = 16,
			uint32_t bytesPerLine = SpaceWireUtilities::DumpsPerLine) throw (SpaceWireUtilitiesException) {
		using namespace std;
		int v;
		if (wordwidth == 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		if (length > wordwidth && length % wordwidth != 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		for (uint32_t i = 0; i < (length / wordwidth + bytesPerLine - 1) / bytesPerLine; i++) {
			for (uint32_t o = 0; o < bytesPerLine; o++) {
				if (i * bytesPerLine + o < length / wordwidth) {
					v = 0;
					for (uint32_t p = 0; p < wordwidth; p++) {
						v = v + (uint32_t) data[(i * bytesPerLine + o) * wordwidth + p];
					}
					(*ofs) << "0x" << right << hex << setw(2 * wordwidth) << setfill('0') << v << " ";
				}
			}
			(*ofs) << endl;
		}
	}

	static std::string packetToString(std::vector<uint8_t>* data, size_t nBytesDisplayed = 16)
			throw (SpaceWireUtilitiesException) {
		if (data->size() == 0) {
			return "";
		}
		return packetToString(&(data->at(0)), data->size(), nBytesDisplayed);
	}

	static std::string packetToString(uint8_t* data, int length, size_t nBytesDisplayed = 16)
			throw (SpaceWireUtilitiesException) {
		using namespace std;
		size_t max;
		if (nBytesDisplayed < (unsigned int) length) {
			max = nBytesDisplayed;
		} else {
			max = length;
		}
		if (length == 0) {
			return "(empty packet)";
		}
		stringstream ss;
		for (size_t i = 0; i < max; i++) {
			ss << "0x" << right << hex << setw(2) << setfill('0') << (uint32_t) data[i];
			if (i != max - 1) {
				ss << "  ";
			}
		}
		if (max != (unsigned int) length) {
			ss << " ... (total " << dec << length << " bytes)";
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
		size_t size = data->size();
		if (size % 2 == 0 && size != 0) {
			for (size_t i = 0; i < (size + 1) / 2; i++) {
				cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
				cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
				cout << "  ";
				cout << "0x" << setfill('0') << setw(4) << hex
						<< (uint32_t) (data->at(i * 2 + 1) * 0x100 + data->at(i * 2)) << endl;
			}
		} else if (size != 0) {
			size_t i;
			for (i = 0; i < size / 2; i++) {
				cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
				cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
				cout << "  ";
				cout << "0x" << setfill('0') << setw(4) << hex
						<< (uint32_t) (data->at(i * 2 + 1) * 0x100 + data->at(i * 2)) << endl;
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
