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
 * SpaceWireUtilities.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREUTILITIES_HH_
#define SPACEWIREUTILITIES_HH_

#include "CxxUtilities/CxxUtilities.hh"

/** An exception class that is used by the SpaceWireUtilities class. */
class SpaceWireUtilitiesException: public CxxUtilities::Exception {
public:
	enum {
		SizeIncorrect
	};

public:
	SpaceWireUtilitiesException(uint32_t status) :
			CxxUtilities::Exception(status) {
	}

public:
	virtual ~SpaceWireUtilitiesException() {
	}
};

/** A class that provides utility methods used in SpaceWire RMAP Library. */
class SpaceWireUtilities {
public:
	/** Concatenates two std::vector<uint8_t> instances creating new instance.
	 */
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

public:
	/** Concatenates two std::vector<uint8_t> instances creating new instance.
	 */
	static void concatenateTo(std::vector<uint8_t>& array1, std::vector<uint8_t>& array2) {
		using namespace std;
		for (size_t i = 0; i < array2.size(); i++) {
			array1.push_back(array2[i]);
		}
	}

public:
	/** Dumps packet content to the screen. */
	static void dumpPacketUnsignedChar(std::vector<unsigned char>& data) throw (SpaceWireUtilitiesException) {
		std::vector<uint8_t> data2(data.size());
		for (size_t i = 0; i < data.size(); i++) {
			data2[i] = data[i];
		}
		dumpPacket(&std::cout, &data2, 1);
	}

public:
	/** Dumps packet content to the screen. */
	static void dumpPacket(std::vector<uint8_t>& data) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, &data, 1);
	}

public:
	/** Dumps packet content to the screen. */
	static void dumpPacket(std::vector<uint8_t>* data) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, data, 1);
	}

public:
	/** Dumps packet content to the screen. */
	static void dumpPacket(std::ostream* ofs, std::vector<uint8_t>* data) throw (SpaceWireUtilitiesException) {
		dumpPacket(ofs, data, 1);
	}

public:
	/** Dumps packet content to the screen. */
	static void dumpPacket(std::vector<uint8_t>* data, size_t wordwidth) throw (SpaceWireUtilitiesException) {
		dumpPacket(&std::cout, data, wordwidth);
	}

public:
	/** Dumps packet content to an output stream instance.
	 * @param[in] output stream to which dump is performed.
	 */
	static void dumpPacket(std::ostream* ofs, std::vector<uint8_t>* data, uint32_t wordwidth, size_t bytesPerLine =
			SpaceWireUtilities::DumpsPerLine) throw (SpaceWireUtilitiesException) {
		using namespace std;
		int v;
		if (wordwidth == 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		if (data->size() > wordwidth && data->size() % wordwidth != 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		for (size_t i = 0; i < (data->size() / wordwidth + bytesPerLine - 1) / bytesPerLine; i++) {
			for (size_t o = 0; o < bytesPerLine; o++) {
				if (i * bytesPerLine + o < data->size() / wordwidth) {
					v = 0;
					for (size_t p = 0; p < wordwidth; p++) {
						v = v + (uint32_t) data->at((i * bytesPerLine + o) * wordwidth + p);
					}
					(*ofs) << "0x" << right << hex << setw(2 * wordwidth) << setfill('0') << v << " ";
				}
			}
			(*ofs) << endl;
		}
	}

public:
	/** Dumps packet content to an output stream instance.
	 * @param[in] output stream to which dump is performed.
	 */
	static void dumpPacket(std::ostream* ofs, uint8_t* data, size_t length, size_t wordwidth = 16,
			size_t bytesPerLine = SpaceWireUtilities::DumpsPerLine) throw (SpaceWireUtilitiesException) {
		using namespace std;
		int v;
		if (wordwidth == 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		if (length > wordwidth && length % wordwidth != 0) {
			throw(SpaceWireUtilitiesException(SpaceWireUtilitiesException::SizeIncorrect));
		}
		for (size_t i = 0; i < (length / wordwidth + bytesPerLine - 1) / bytesPerLine; i++) {
			for (size_t o = 0; o < bytesPerLine; o++) {
				if (i * bytesPerLine + o < length / wordwidth) {
					v = 0;
					for (size_t p = 0; p < wordwidth; p++) {
						v = v + (uint32_t) data[(i * bytesPerLine + o) * wordwidth + p];
					}
					(*ofs) << "0x" << right << hex << setw(2 * wordwidth) << setfill('0') << v << " ";
				}
			}
			(*ofs) << endl;
		}
	}

public:
	/** Converts a packet content to std::string. */
	static std::string packetToString(std::vector<uint8_t>* data, size_t nBytesDisplayed = 16)
			throw (SpaceWireUtilitiesException) {
		if (data->size() == 0) {
			return "";
		}
		return packetToString(&(data->at(0)), data->size(), nBytesDisplayed);
	}

public:
	/** Converts a packet content to std::string. */
	static std::string packetToString(uint8_t* data, int length, size_t nBytesDisplayed = 16)
			throw (SpaceWireUtilitiesException) {
		using namespace std;
		size_t max;
		if (nBytesDisplayed < length) {
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

public:
	/** Converts string to uint8_t. */
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

public:
	/** Dumps a packet content with addresses to the screen.
	 * @param address initial address.
	 * @param data packet content to be dumped.
	 */
	static void printVectorWithAddress(uint32_t address, std::vector<uint8_t>* data) {
		using namespace std;
		cout << "Address    Data" << endl;
		for (size_t i = 0; i < data->size(); i++) {
			cout << setfill('0') << setw(4) << hex << (address + i) / 0x00010000;
			cout << "-" << setfill('0') << setw(4) << hex << (address + i) % 0x00010000;
			cout << "  ";
			cout << "0x" << setfill('0') << setw(2) << hex << (uint32_t) (data->at(i)) << endl;
		}
		cout << dec << left;
	}

public:
	/** Dumps a packet content with addresses to the screen.
	 * @param address initial address.
	 * @param data packet content to be dumped.
	 */
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
	static const size_t DumpsPerLine = 8;
};
#endif /* SPACEWIREUTILITIES_HH_ */
