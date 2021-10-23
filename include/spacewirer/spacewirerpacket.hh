#ifndef SPACEWIRERPACKET_HH_
#define SPACEWIRERPACKET_HH_

#include "spacewire/spacewirepacket.hh"
#include "spacewire/spacewireutilities.hh"
#include "spacewirer/spacewirerprotocol.hh"
#include "spacewirer/spacewirerutilities.hh"

#undef debugSpaceWireRPacket

class SpaceWireRPacketType {
 public:
  static constexpr u8 DataPacket = 0x00;
  static constexpr u8 DataAckPacket = 0x01;
  static constexpr u8 ControlPacketOpenCommand = 0x02;
  static constexpr u8 ControlPacketCloseCommand = 0x03;
  static constexpr u8 HeartBeatPacket = 0x04;
  static constexpr u8 HeartBeatAckPacket = 0x05;
  static constexpr u8 FlowControlPacket = 0x06;
  static constexpr u8 ControlAckPacket = 0x07;
};

class SpaceWireRSequenceFlagType {
 public:
  static constexpr u8 FirstSegment = 0x01;
  static constexpr u8 ContinuedSegment = 0x00;
  static constexpr u8 LastSegment = 0x02;
  static constexpr u8 CompleteSegment = 0x03;
};

class SpaceWireRSecondaryHeaderFlagType {
 public:
  static constexpr u8 SecondaryHeaderIsNotUsed = 0x00;
  static constexpr u8 SecondaryHeaderIsUsed = 0x01;
};

class SpaceWireRPacketException : public CxxUtilities::Exception {
 public:
  enum {
    InvalidPacket,         //
    InvalidCRC,            //
    InvalidPayloadLength,  //
    TooShortPacketSize,    //
    InvalidHeaderFormat,   //
    InvalidProtocolID,     //
    InvalidPrefixLength,   //
    PacketHasExtraBytesAfterTrailer
  };

 public:
  SpaceWireRPacketException(uint32_t status) : CxxUtilities::Exception(status) {}

 public:
  virtual ~SpaceWireRPacketException() {}

 public:
  std::string toString() {
    std::string str;
    switch (status) {
      case InvalidPacket:
        str = "InvalidPacket";
        break;
      case InvalidCRC:
        str = "InvalidCRC";
        break;
      case InvalidPayloadLength:
        str = "InvalidPayloadLength";
        break;
      case TooShortPacketSize:
        str = "TooShortPacketSize";
        break;
      case InvalidHeaderFormat:
        str = "InvalidHeaderFormat";
        break;
      case InvalidProtocolID:
        str = "InvalidProtocolID";
        break;
      case InvalidPrefixLength:
        str = "InvalidPrefixLength";
        break;
      case PacketHasExtraBytesAfterTrailer:
        str = "PacketHasExtraBytesAfterTrailer";
        break;
      default:
        str = "UndefinedException";
        break;
    }
    return str;
  }
};

/**
 *
 * SpaceWire-R Packet Format
 *
 * [Header] [Payload] [Trailer]
 *
 * where
 *
 * [Header] =  [Destination SLA] (1 byte)
 *             [Protocol ID]     (1 byte)
 *             [Packet Control]  (1 byte)
 *             [Payload Length]  (2 bytes)
 *             [Channel Number]  (2 bytes)
 *             [Sequence Number] (1 byte)
 *             [Address Control] (1 byte)
 *             [Source Address]  (N+1 bytes)
 *
 * [Packet Control] =
 *             [Protocol Version]       (2 bits)
 *             [Secondary Header Flag]  (1 bit)
 *             [Sequence Flag]          (2 bits)
 *             [Packet Type]            (3 bits)
 *
 * [Address Control] =
 *             [Reserved Spare] (4 bits)
 *             [Prefix Length]  (4 bits)
 *
 * [Source Address] =
 *             [Prefix]     (N bytes)
 *             [Source SLA] (1 byte)
 *
 * [Payload] = [Secondary Header (optional)]
 *             [Application Data]
 *
 * [Trailer] = [CRC] (2 bytes)
 *
 *--------------------------------------
 * ==Packet Type==
 * 0 Data Packet
 * 1 Data Ack Packet
 * 2 Control Packet (Open Command)
 * 3 Control Packet (Close Command)
 * 4 Keep Alive Packet
 * 5 Keep Alive Ack Packet
 * 6 Flow Control Packet
 * 7 Control Ack Packet
 *
 */
class SpaceWireRPacket : public SpaceWirePacket {
 public:
  enum {
    DataPacketType = 0x00,                 //
    DataAckPacketType = 0x01,              //
    ControlPacketOpenCommandType = 0x02,   //
    ControlPacketCloseCommandType = 0x03,  //
    HeartBeatPacketType = 0x04,            //
    HeartBeatAckPacketType = 0x05,         //
    FlowControlPacket = 0x06,              //
    ControlAckPacket = 0x07                //
  };

 private:
  std::vector<u8> header;

 private:
  u8 packetControl;
  u8 payloadLength[2];
  u8 channelNumber[2];
  u8 sequenceNumber;
  u8 prefixLength;

 private:
  u8 destinationLogicalAddress;
  std::vector<u8> destinationSpaceWireAddress;

 private:
  std::vector<u8> prefix;
  u8 sourceLogicalAddress;

 private:
  /// for Packet Control Field
  const static u8 protocolVersion = 0x01;  // 2bits
  u8 secondaryHeaderFlag;                  // 1bit
  u8 sequenceFlags;                        // 2bits
  u8 packetType;                           // 3bits

 private:
  std::vector<u8> payload;
  uint16_t crc16;

 private:
  double sentoutTimeStamp;

 public:
  const static size_t MaxPayloadLength = 65535;
  const static size_t MinimumHeaderLength = 10;

 public:
  SpaceWireRPacket() : SpaceWirePacket() {
    protocolID = SpaceWireRProtocol::ProtocolID;
    packetType = SpaceWireRPacketType::DataPacket;  // dummy
    unuseSecondaryHeader();
    prefixLength = 0;
    sourceLogicalAddress = SpaceWirePacket::DefaultLogicalAddress;
  }

 public:
  std::string getSequenceFlagsAsString() {
    std::string str;
    switch (this->getSequenceFlags()) {
      case SpaceWireRSequenceFlagType::FirstSegment:
        str = "FirstSegment";
        break;
      case SpaceWireRSequenceFlagType::ContinuedSegment:
        str = "ContinuedSegment";
        break;
      case SpaceWireRSequenceFlagType::LastSegment:
        str = "LastSegment";
        break;
      case SpaceWireRSequenceFlagType::CompleteSegment:
        str = "CompleteSegment";
        break;
      default:
        str = "InvalidSequenceFlag";
        break;
    }
    return str;
  }

 public:
  std::string getPacketTypeAsString() {
    std::string str;
    switch (this->getPacketType()) {
      case 0:
        str = "DataPacket";
        break;
      case 1:
        str = "DataAckPacket";
        break;
      case 2:
        str = "ControlPacket-Open";
        break;
      case 3:
        str = "ControlPacket-Close";
        break;
      case 4:
        str = "HeartBeatPacket";
        break;
      case 5:
        str = "HeartBeatAckPacket";
        break;
      case 6:
        str = "FlowControlPacket";
        break;
      case 7:
        str = "ControlAckPacket";
        break;
      default:
        str = "InvalidPacketType";
        break;
    }
    return str;
  }

 private:
  static const size_t MaximumDumpLength = 32;

 public:
  std::string toStringByteArray() {
    std::stringstream ss;
    using namespace std;
    std::vector<u8>* packet = this->getPacketBufferPointer();
    size_t dumpLength = min<size_t>(packet->size(), 32);
    for (size_t i = 0; i < dumpLength; i++) {
      ss << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)packet->at(i);
      if (i != dumpLength - 1) {
        ss << " ";
      }
    }
    if (dumpLength < packet->size()) {
      ss << " ... (total " << dec << packet->size() << " bytes)" << endl;
    }
    return ss.str();
  }

  std::string toString() {
    std::stringstream ss;
    using namespace std;
    ss << "Packet Seq Num = " << this->getSequenceNumberAs32bitInteger() << endl;
    ss << "Packet Type    = " << this->getPacketTypeAsString() << endl;
    ss << "Segment Type   = " << this->getSequenceFlagsAsString() << endl;
    ss << "Length         = " << this->getPacketBufferPointer()->size() << endl;
    ss << "Dump           = " << this->toStringByteArray() << endl;
    return ss.str();
    // todo implement toString()
  }

  std::vector<u8>* getPacketBufferPointer() {
    std::vector<u8>* buffer = new std::vector<u8>();
    constructHeader();

    // Target SpaceWire Address
    size_t destinationSpaceWireAddressSize = destinationSpaceWireAddress.size();
    for (size_t i = 0; i < destinationSpaceWireAddressSize; i++) {
      buffer->push_back(destinationSpaceWireAddress[i]);
    }

    // Header
    size_t headerSize = header.size();
    for (size_t i = 0; i < headerSize; i++) {
      buffer->push_back(header[i]);
    }

    // Payload
    size_t payloadSize = payload.size();
    for (size_t i = 0; i < payloadSize; i++) {
      buffer->push_back(payload[i]);
    }

    // Calculate CRC
    crc16 = SpaceWireRUtilities::calculateCRCForHeaderAndData(header, payload);

    // Trailer
    buffer->push_back(crc16 / 0x100);
    buffer->push_back(crc16 % 0x100);

    return buffer;
  }

  size_t getPacket(u8* buffer, size_t maxLength) {
    constructHeader();

    // Check buffer length
    size_t destinationSpaceWireAddressSize = destinationSpaceWireAddress.size();
    size_t headerSize = header.size();
    size_t payloadSize = payload.size();
    const size_t crcSize = 2;
    if (destinationSpaceWireAddressSize + headerSize + payloadSize + crcSize > maxLength) {
      return 0;
    }

    size_t index = 0;

    // SpaceWire Address
    for (size_t i = 0; i < destinationSpaceWireAddressSize; i++) {
      buffer[index] = destinationSpaceWireAddress[i];
      index++;
    }

    // Header
    for (size_t i = 0; i < headerSize; i++) {
      buffer[index] = header[i];
      index++;
    }

    // Payload
    for (size_t i = 0; i < payloadSize; i++) {
      buffer[index] = payload[i];
      index++;
    }

    // Calculate CRC
    crc16 = SpaceWireRUtilities::calculateCRCForArray(buffer, index - destinationSpaceWireAddressSize + 1);

    // Trailer
    buffer[index] = crc16 / 0x100;
    index++;
    buffer[index] = crc16 % 0x100;
    index++;

    return index;
  }

 public:
  bool isAckPacket() {
    if (this->packetType == SpaceWireRPacketType::DataAckPacket ||
        this->packetType == SpaceWireRPacketType::ControlAckPacket ||
        this->packetType == SpaceWireRPacketType::HeartBeatAckPacket) {
      return true;
    } else {
      return false;
    }
  }
  void constructAckForPacket(SpaceWireRPacket* packet, std::vector<u8> prefix) {
    this->constructAckForPacket(packet);
    this->prefix = prefix;
  }
  void constructAckForPacketWithFlowControl(SpaceWireRPacket* packet, u8 maximumAcceptableSequenceNumber) {
    // first construct ack packet
    this->constructAckForPacket(packet);

    // set MASN
    // note: HeartBeat Ack shall not carry MASN.
    if (this->isDataAckPacket() || this->isControlAckPacket()) {
      u8 masn[1] = {maximumAcceptableSequenceNumber};
      this->setPayload(masn, 1);
      this->setPayloadLength(1);
    }
  }
  void constructAckForPacket(SpaceWireRPacket* packet) {
    if (packet->isAckPacket()) {
      return;
    }

    // destination addresses
    this->destinationSpaceWireAddress = packet->getPrefix();
    this->destinationLogicalAddress = packet->getSourceLogicalAddress();

    // secondary header flag
    this->setSecondaryHeaderFlag(SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed);

    // set sequence flag bits
    this->setSequenceFlags(SpaceWireRSequenceFlagType::CompleteSegment);

    // set packet type
    if (packet->isControlPacketOpenCommand()) {
      this->setPacketType(SpaceWireRPacketType::ControlAckPacket);
    } else if (packet->isControlPacketCloseCommand()) {
      this->setPacketType(SpaceWireRPacketType::ControlAckPacket);
    } else if (packet->isDataPacket()) {
      this->setPacketType(SpaceWireRPacketType::DataAckPacket);
    } else if (packet->isHeartBeatPacketType()) {
      this->setPacketType(SpaceWireRPacketType::HeartBeatAckPacket);
    } else if (packet->isFlowControlPacket()) {
      // Note: Flow Control Packet and Flow Control Ack Packet use the
      // same packet type (0x06).
      this->setPacketType(SpaceWireRPacketType::FlowControlPacket);
    }

    // set payload length
    this->setPayloadLength(0);

    // set channel number
    this->setChannelNumber(packet->getChannelNumber());

    // sequence number
    this->setSequenceNumber(packet->getSequenceNumber());

    // source addresses
    this->sourceLogicalAddress = packet->destinationLogicalAddress;
    this->prefix.clear();

    // clear payload
    this->clearPayload();
  }

 public:
  void clearPayload() {
    payload.clear();
    this->setPayloadLength(0);
  }

 private:
  void constructHeader() {
    header.clear();
    header.push_back(this->destinationLogicalAddress);

    // Protocol ID
    header.push_back(SpaceWireRProtocol::ProtocolID);

    // Packet Control
    this->packetControl = protocolVersion * 0x40       /* 0100_0000 */
                          + secondaryHeaderFlag * 0x20 /* 0010_0000 */
                          + sequenceFlags * 0x08       /* 0000_1000 */
                          + packetType;
    header.push_back(this->packetControl);

    // Payload Length
    header.push_back(payloadLength[0]);
    header.push_back(payloadLength[1]);

    // Channel Number
    header.push_back(channelNumber[0]);
    header.push_back(channelNumber[1]);

    // Sequence Number
    header.push_back(sequenceNumber);

    // Address Control
    header.push_back(0x0000 + prefix.size());

    // Source Address
    for (size_t i = 0; i < prefix.size(); i++) {
      header.push_back(prefix[i]);
    }
    header.push_back(sourceLogicalAddress);
  }

 public:
  void interpretPacket(std::vector<u8>* buffer) throw(SpaceWireRPacketException) {
    using namespace std;

    if (buffer->size() < MinimumHeaderLength) {
      throw SpaceWireRPacketException(SpaceWireRPacketException::TooShortPacketSize);
    }

#ifdef debugSpaceWireRPacket
    cout << "SpaceWireRPacket::interpretPacket() #1" << endl;
#endif

    try {
      // SpaceWire Address
      size_t index = 0;
      std::vector<u8> temporarySpaceWireAddress;
      size_t destinationSpaceWireAddressLength = 0;
      try {
#ifdef debugSpaceWireRPacket
        cout << "SpaceWireRPacket::interpretPacket() #2" << endl;
#endif
        while (buffer->at(index) < 0x20) {
          temporarySpaceWireAddress.push_back(buffer->at(index));
          index++;
        }
#ifdef debugSpaceWireRPacket
        cout << "SpaceWireRPacket::interpretPacket() #3 Remaining path address " << index << " bytes [";
        for (size_t i = 0; i < temporarySpaceWireAddress.size(); i++) {
          cout << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)temporarySpaceWireAddress[i] << " ";
        }
        cout << "]" << endl;
#endif
      } catch (...) {
        throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidHeaderFormat);
      }
      this->destinationSpaceWireAddress = temporarySpaceWireAddress;
      destinationSpaceWireAddressLength = index;

      // Destination SLA
      this->destinationLogicalAddress = buffer->at(index);
      index++;

      // Protocol ID
#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() #4 ProtocolID="
           << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)buffer->at(index) << endl;
#endif
      if (buffer->at(index) != SpaceWireRProtocol::ProtocolID) {
        cerr << "Invalid Protocol ID: "
             << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)buffer->at(index) << endl;
        throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidProtocolID);
      }
      index++;

#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() #5" << endl;
#endif
      // Packet Control
      this->packetControl = buffer->at(index);
      interpretPacketControl();
      index++;

      // Payload Length
      this->payloadLength[0] = buffer->at(index);
      this->payloadLength[1] = buffer->at(index + 1);
      index += 2;

      // Channel Number
      this->channelNumber[0] = buffer->at(index);
      this->channelNumber[1] = buffer->at(index + 1);
      index += 2;

      // Sequence Number
      this->sequenceNumber = buffer->at(index);
      index++;

      // Address Control
      u8 prefixLength = buffer->at(index);
      index++;

#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() packetControl="
           << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)packetControl << endl;
      cout << "SpaceWireRPacket::interpretPacket() channel number="
           << "0x" << hex << right << setw(2) << setfill('0') << channelNumber << endl;
      cout << "SpaceWireRPacket::interpretPacket() sequenceNumber="
           << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)sequenceNumber << endl;
#endif

#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() #6" << endl;
#endif
      // Prefix
      try {
        prefix.clear();
#ifdef debugSpaceWireRPacket
        cout << "SpaceWireRPacket::interpretPacket() #7 prefixLength=" << (size_t)prefixLength << endl;
#endif
        for (size_t i = 0; i < prefixLength; i++) {
#ifdef debugSpaceWireRPacket
          cout << "SpaceWireRPacket::interpretPacket() #8" << endl;
#endif
          prefix.push_back(buffer->at(index));
          index++;
        }
#ifdef debugSpaceWireRPacket
        cout << "SpaceWireRPacket::interpretPacket() #8-1 prefix = ";
        for (size_t i = 0; i < prefix.size(); i++) {
          cout << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)prefix[i] << " ";
        }
        cout << endl;
#endif

      } catch (...) {
        throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPrefixLength);
      }

      // Source SLA
      this->sourceLogicalAddress = buffer->at(index);
      index++;

      // Payload
      try {
        size_t payloadLengthValue = payloadLength[0] * 0x100 + payloadLength[1];
        payload.clear();
#ifdef debugSpaceWireRPacket
        cout << "SpaceWireRPacket::interpretPacket() #9 payloadLength=" << dec << payloadLengthValue << endl;
#endif
        for (size_t i = 0; i < payloadLengthValue; i++) {
          payload.push_back(buffer->at(index));
          index++;
        }
      } catch (...) {
        throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPayloadLength);
      }

      // Trailer
#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() #10" << endl;
#endif
      this->crc16 = buffer->at(index) * 0x100 + buffer->at(index + 1);
      index += 2;

      // Size check
#ifdef debugSpaceWireRPacket
      cout << "SpaceWireRPacket::interpretPacket() #11 index=" << dec << index << " buffer.size()=" << buffer->size()
           << endl;
#endif
      if (index != buffer->size()) {
        throw SpaceWireRPacketException(SpaceWireRPacketException::PacketHasExtraBytesAfterTrailer);
      }

      // CRC Check
      uint16_t calculatedCRC =
          SpaceWireRUtilities::calculateCRCForArray((u8*)(&(buffer->at(destinationSpaceWireAddressLength))),
                                                    buffer->size() - 2 - destinationSpaceWireAddressLength);
      if (this->crc16 != calculatedCRC) {
        cerr << "SpaceWireRPacket::interpretPacket() #12 CRC received="
             << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)this->crc16 << " calculated="
             << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)calculatedCRC << endl;
        throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidCRC);
      }

    } catch (SpaceWireRPacketException& e) {
      throw e;
    } catch (...) {
      throw SpaceWireRPacketException(SpaceWireRPacketException::InvalidPacket);
    }
  }

 private:
  void interpretPacketControl() {
    this->secondaryHeaderFlag = (packetControl & 0x20) >> 5 /* 0010_0000 */;
    this->sequenceFlags = (packetControl & 0x18) >> 3 /* 0001_1000 */;
    this->packetType = (packetControl & 0x07) /* 0000_0111 */;
  }

 public:
  uint16_t getChannelNumber() const { return channelNumber[0] * 0x100 + channelNumber[1]; }
  uint16_t getCRC() const { return crc16; }
  std::vector<u8> getHeader() const { return header; }
  u8 getPacketControl() const { return packetControl; }
  u8 getPacketType() const { return this->packetType; }
  std::vector<u8>* getPayload() { return &payload; }
  uint16_t getPayloadLength() const { return payloadLength[0] * 0x100 + payloadLength[1]; }
  u8 getSequenceNumber() const { return sequenceNumber; }
  uint32_t getSequenceNumberAs32bitInteger() const { return (uint32_t)sequenceNumber; }
  std::vector<u8> getSourceAddressPrefix() const { return prefix; }
  u8 getSourceLogicalAddress() const { return sourceLogicalAddress; }
  void setChannelNumber(uint16_t channelNumber) {
    this->channelNumber[0] = channelNumber / 0x100;
    this->channelNumber[1] = channelNumber % 0x100;
  }
  void setCRC(uint16_t crc) { this->crc16 = crc; }
  void setHeader(std::vector<u8>& header) { this->header = header; }
  void setPacketControl(u8 packetControl) { this->packetControl = packetControl; }
  void setPacketType(u8 packetType) { this->packetType = packetType; }
  void setPayload(std::vector<u8>& payload) throw(SpaceWireRPacketException) {
    if (payload.size() > SpaceWireRPacket::MaxPayloadLength) {
      throw SpaceWireRPacketException::InvalidPayloadLength;
    }
    this->payload = payload;
    setPayloadLength(payload.size());
  }
  void setPayload(u8* payload, size_t length) throw(SpaceWireRPacketException) {
    if (length > SpaceWireRPacket::MaxPayloadLength) {
      throw SpaceWireRPacketException::InvalidPayloadLength;
    }
    this->payload.clear();
    for (size_t i = 0; i < length; i++) {
      this->payload.push_back(payload[i]);
    }
    setPayloadLength(length);
  }
  void setPayload(std::vector<u8>* payload, size_t index, size_t length) throw(SpaceWireRPacketException) {
    if (length > SpaceWireRPacket::MaxPayloadLength) {
      throw SpaceWireRPacketException::InvalidPayloadLength;
    }
    this->payload.clear();
    size_t loopEnd = length + index;
    for (size_t i = index; i < loopEnd; i++) {
      this->payload.push_back(payload->at(i));
    }
    setPayloadLength(length);
  }
  void setPayloadLength(uint16_t payloadLength) {
    this->payloadLength[0] = payloadLength / 0x100;
    this->payloadLength[1] = payloadLength % 0x100;
  }
  void setSequenceNumber(u8 sequenceNumber) { this->sequenceNumber = sequenceNumber; }
  void setSourceAddressPrefix(std::vector<u8>& sourceAddressPrefix) { this->prefix = sourceAddressPrefix; }
  void setSourceLogicalAddress(u8 sourceSpaceWireLogicalAddress) {
    this->sourceLogicalAddress = sourceSpaceWireLogicalAddress;
  }
  void setTrailer(uint16_t trailer) { setCRC(trailer); }
  void setDestinationLogicalAddress(u8 destinationLogicalAddress) {
    this->destinationLogicalAddress = destinationLogicalAddress;
  }
  void setDestinationSpaceWireAddress(std::vector<u8>& destinationSpaceWireAddress) {
    this->destinationSpaceWireAddress = destinationSpaceWireAddress;
  }
  /// Packet Control
  u8 getSecondaryHeaderFlag() const { return secondaryHeaderFlag; }
  void setSecondaryHeaderFlag(u8 secondaryHeaderFlag) { this->secondaryHeaderFlag = secondaryHeaderFlag; }
  u8 getSequenceFlags() const { return sequenceFlags; }
  void setSequenceFlags(u8 sequenceFlags) { this->sequenceFlags = sequenceFlags; }
  // set packet types
  void setDataPacketFlag() { setPacketType(SpaceWireRPacketType::DataPacket); }
  void setDataAckPacketFlag() { setPacketType(SpaceWireRPacketType::DataAckPacket); }
  void setControlPacketOpenCommandFlag() { setPacketType(SpaceWireRPacketType::ControlPacketOpenCommand); }
  void setControlPacketCloseCommandFlag() { setPacketType(SpaceWireRPacketType::ControlPacketCloseCommand); }
  void setHeartBeatPacketFlag() { setPacketType(SpaceWireRPacketType::HeartBeatPacket); }
  void setHeartBeatAckPacketFlag() { setPacketType(SpaceWireRPacketType::HeartBeatAckPacket); }
  void setFlowControlPacketFlag() { setPacketType(SpaceWireRPacketType::FlowControlPacket); }
  void setControlAckPacketFlag() { setPacketType(SpaceWireRPacketType::ControlAckPacket); }
  void unuseSecondaryHeader() {
    this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed;
  }
  void useSecondaryHeader() { this->secondaryHeaderFlag = SpaceWireRSecondaryHeaderFlagType::SecondaryHeaderIsNotUsed; }
  void setFirstSegmentFlag() { this->sequenceFlags = SpaceWireRSequenceFlagType::FirstSegment; }
  void setContinuedSegmentFlag() { this->sequenceFlags = SpaceWireRSequenceFlagType::ContinuedSegment; }
  void setLastSegmentFlag() { this->sequenceFlags = SpaceWireRSequenceFlagType::LastSegment; }
  void setCompleteSegmentFlag() { this->sequenceFlags = SpaceWireRSequenceFlagType::CompleteSegment; }
  bool isFirstSegment() { return (this->sequenceFlags == SpaceWireRSequenceFlagType::FirstSegment) ? true : false; }
  bool isLastSegment() { return (this->sequenceFlags == SpaceWireRSequenceFlagType::LastSegment) ? true : false; }
  bool isContinuedSegment() {
    return (this->sequenceFlags == SpaceWireRSequenceFlagType::ContinuedSegment) ? true : false;
  }
  bool isCompleteSegment() {
    return (this->sequenceFlags == SpaceWireRSequenceFlagType::CompleteSegment) ? true : false;
  }

  // prefix-related
  size_t getPrefixLength() const { return prefix.size(); }
  std::vector<u8> getPrefix() const { return prefix; }
  void setPrefix(std::vector<u8>& prefix) {
    this->prefix = prefix;
    this->prefixLength = prefix.size();
  }
  bool isDataPacket() { return (packetType == DataPacketType) ? true : false; }
  bool isDataAckPacket() { return (packetType == DataAckPacketType) ? true : false; }
  bool isControlPacketOpenCommand() { return (packetType == ControlPacketOpenCommandType) ? true : false; }
  bool isControlPacketCloseCommand() { return (packetType == ControlPacketCloseCommandType) ? true : false; }
  bool isHeartBeatPacketType() { return (packetType == HeartBeatPacketType) ? true : false; }
  bool isHeartBeatAckPacketType() { return (packetType == HeartBeatAckPacketType) ? true : false; }
  bool isFlowControlPacket() { return (packetType == FlowControlPacket) ? true : false; }
  bool isControlAckPacket() { return (packetType == ControlAckPacket) ? true : false; }
  bool hasSecondaryHeader() { return (secondaryHeaderFlag == 0x01) ? true : false; }
  double getSentoutTimeStamp() const { return sentoutTimeStamp; }
  void setSentoutTimeStamp(double sentoutTimeStamp) { this->sentoutTimeStamp = sentoutTimeStamp; }
  void setCurrentTimeToSentoutTimeStamp() { this->sentoutTimeStamp = CxxUtilities::Time::getClockValueInMilliSec(); }
};

class SpaceWireRDataPacket : public SpaceWireRPacket {
 public:
  SpaceWireRDataPacket() { this->setDataPacketFlag(); }
  ~SpaceWireRDataPacket() override = default;
};

class SpaceWireRDataAckPacket : public SpaceWireRPacket {
 public:
  SpaceWireRDataAckPacket() { this->setDataAckPacketFlag(); }
  ~SpaceWireRDataAckPacket() override = default;
};

class SpaceWireROpenCommandPacket : public SpaceWireRPacket {
 public:
  SpaceWireROpenCommandPacket() { this->setControlPacketOpenCommandFlag(); }
  ~SpaceWireROpenCommandPacket() override = default;
};

class SpaceWireRCloseCommandPacket : public SpaceWireRPacket {
 public:
  SpaceWireRCloseCommandPacket() { this->setControlPacketCloseCommandFlag(); }
  ~SpaceWireRCloseCommandPacket() override = default;
};

class SpaceWireRHeartBeatPacket : public SpaceWireRPacket {
 public:
  SpaceWireRHeartBeatPacket() { this->setHeartBeatPacketFlag(); }
  ~SpaceWireRHeartBeatPacket() override = default;
};

class SpaceWireRHeartBeatAckPacket : public SpaceWireRPacket {
 public:
  SpaceWireRHeartBeatAckPacket() { this->setHeartBeatAckPacketFlag(); }
  ~SpaceWireRHeartBeatAckPacket() override = default;
};

class SpaceWireRFlowControlPacket : public SpaceWireRPacket {
 public:
  SpaceWireRFlowControlPacket() { this->setFlowControlPacketFlag(); }
  ~SpaceWireRFlowControlPacket() override = default;
};

class SpaceWireRControlAckPacket : public SpaceWireRPacket {
 public:
  SpaceWireRControlAckPacket() { this->setControlAckPacketFlag(); }
  ~SpaceWireRControlAckPacket() override = default;
};

#endif /* SPACEWIRERPACKET_HH_ */
