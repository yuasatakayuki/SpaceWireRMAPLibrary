/*
 * RMAPProtocol.hh
 *
 *  Created on: Jul 27, 2011
 *      Author: yuasa
 */

#ifndef RMAPPROTOCOL_HH_
#define RMAPPROTOCOL_HH_

class RMAPProtocol {
public:
	static const uint8_t ProtocolIdentifier = 0x01;
	static const uint8_t DefaultPacketType = 0x01;
	static const uint8_t DefaultWriteOrRead = 0x01;
	static const uint8_t DefaultVerifyMode = 0x01;
	static const uint8_t DefaultAckMode = 0x01;
	static const uint8_t DefaultIncrementMode = 0x01;
	static const uint16_t DefaultTID = 0x00;
	static const uint8_t DefaultExtendedAddress = 0x00;
	static const uint32_t DefaultAddress = 0x00;
	static const uint32_t DefaultLength = 0x00;
	static const uint8_t DefaultHeaderCRC = 0x00;
	static const uint8_t DefaultDataCRC = 0x00;
	static const uint8_t DefaultStatus = 0x00;

	static const uint8_t PacketTypeCommand = 0x01;
	static const uint8_t PacketTypeReply = 0x00;

	static const uint8_t PacketWriteMode = 0x01;
	static const uint8_t PacketReadMode = 0x00;

	static const uint8_t PacketVerifyMode = 0x01;
	static const uint8_t PacketNoVerifyMode = 0x00;

	static const uint8_t PacketAckMode = 0x01;
	static const uint8_t PacketNoAckMode = 0x00;

	static const uint8_t PacketIncrementMode = 0x01;
	static const uint8_t PacketNoIncrementMode = 0x00;

	static const uint8_t PacketCRCDraftE = 0x00;
	static const uint8_t PacketCRCDraftF = 0x01;

	static const uint8_t DefaultKey = 0x20;

	static const bool DefaultCRCCheckMode = true;

	static const uint8_t DefaultLogicalAddress = 0xFE;

};

#endif /* RMAPPROTOCOL_HH_ */
