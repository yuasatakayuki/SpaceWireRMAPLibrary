/*
 * main_RMAP_interpretAsAnRMAPPacket.cc
 *
 *  Created on: Nov 25, 2011
 *      Author: yuasa
 */

#include "RMAPPacket.hh"
#include "SpaceWireUtilities.hh"
#include "CxxUtilities/String.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace CxxUtilities;
	if (argc == 1) {
		cerr << "usage : main_RMAP_interpretAsAnRMAPPacket (FILE 1) ... (FILE n)" << endl;
		cerr << "Files should contain byte sequence of an RMAP packet (in plane text format)." << endl;
		exit(-1);
	}

	for (int i = 1; i < argc; i++) {
		string filename(argv[i]);
		cout << "============================" << endl;
		cout << "File " << i << " : " << filename << endl;
		cout << "============================" << endl;
		try {
			string str = File::getAllLinesAsString(filename);
			vector<uint8_t> bytes = String::toUInt8Array(str);
			try {
				RMAPPacket packet;
				packet.setHeaderCRCIsChecked(false);
				packet.setDataCRCIsChecked(false);
				packet.interpretAsAnRMAPPacket(bytes);
				cout << packet.toString();
				//check if data part exists
				bool hasData=packet.hasData();
				//check header crc
				try {
					packet.setHeaderCRCIsChecked(true);
					packet.setDataCRCIsChecked(false);
					packet.interpretAsAnRMAPPacket(bytes);
					cout << "Header CRC is correct." << endl;
				} catch (RMAPPacketException e) {
					cout << "Header CRC is not correct." << endl;
					packet.setHeaderCRCIsChecked(false);
					packet.setDataCRCIsChecked(false);
					packet.interpretAsAnRMAPPacket(bytes);
					packet.constructPacket();
					cout << "Correct Header CRC is 0x" << hex << setw(2) << setfill('0') << right << (uint32_t)packet.getHeaderCRC() << endl;
				}
				//check data crc
				if (hasData) {
					try {
						packet.setHeaderCRCIsChecked(false);
						packet.setDataCRCIsChecked(true);
						packet.interpretAsAnRMAPPacket(bytes);
						cout << "Data CRC is correct." << endl;
					} catch (RMAPPacketException e) {
						cout << "Data CRC is not correct." << endl;
						packet.setHeaderCRCIsChecked(false);
						packet.setDataCRCIsChecked(false);
						packet.interpretAsAnRMAPPacket(bytes);
						packet.constructPacket();
						packet.calculateDataCRC();
						cout << "Correct Data CRC is 0x" << hex << setw(2) << setfill('0') << right << (uint32_t)packet.getDataCRC() << endl;
					}
				}
			} catch (RMAPPacketException e) {
				cerr << "Invalid RMAP packet." << endl;
				cerr << e.toString() << endl;
			}
		} catch (...) {
			cerr << "File " << filename << " was not found." << endl;
		}
		cout << endl;

	}
}
