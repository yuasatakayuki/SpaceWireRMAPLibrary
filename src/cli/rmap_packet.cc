#include <fstream>
#include <iostream>

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"
#include "rmap/rmappacket.hh"

u8 toU8(const std::string& str) { return static_cast<u8>(std::stoul(str, nullptr, 16)); }

u32 toU32(const std::string& str) { return static_cast<u32>(std::stoul(str, nullptr, 16)); }

std::vector<u8> toU8Array(const std::vector<std::string>& strArray) {
  std::vector<u8> result{};
  for (const auto& str : strArray) {
    result.push_back(toU8(str));
  }
  return result;
}
std::vector<u8> loadFromFile(std::istream& ist) {
  std::vector<u8> result{};
  std::string str{};
  while (ist) {
    str.clear();
    ist >> str;
    if (str.empty()) {
      continue;
    }
    result.push_back(toU8(str));
  }
  return result;
}

void commandParse(const std::vector<u8>& data) {
  RMAPPacket packet;
  try {
    packet.interpretAsAnRMAPPacket(&data, true);
    std::cout << packet.toString() << '\n';
  } catch (const RMAPPacketException& e) {
    std::cout << e.what() << '\n';
    std::cout << "Input array: ";
    const auto coutFlags = std::cout.flags();
    for (const u8 byte : data) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<u32>(byte) << " ";
    }
    std::cout.setf(coutFlags);
  }
}

struct RMAPPacketOption {
  std::string type{"command"};

  std::string instruction{"read"};
  std::string key{"0x00"};
  std::vector<std::string> replyAddress{};

  std::string memoryAddress{"0x00"};
  std::string dataLength{"0x00"};
  std::string transactionID{"0x00"};

  std::string status{"0x00"};
  std::string extendedAddress{"0x00"};

  std::string initiatorLogicalAddress{"0xFE"};
  std::string targetLogicalAddress{"0xFE"};
  std::vector<std::string> targetSpaceWireAddress{};

  std::vector<std::string> data;
};

void commandConstruct(const RMAPPacketOption& params, std::ostream& ost) {
  RMAPPacket packet;

  if (params.type == "command" || params.type == "c") {
    packet.setCommand();
  } else {
    packet.setReply();
  }

  if (params.instruction == "read" || params.instruction == "r") {
    packet.setRead();
  } else {
    packet.setWrite();
  }

  packet.setKey(toU8(params.key));
  packet.setReplyAddress(toU8Array(params.replyAddress));
  packet.setAddress(toU32(params.memoryAddress));
  packet.setDataLength(std::stoul(params.dataLength));
  const auto data = toU8Array(params.data);
  packet.setData(data.data(), data.size());
  packet.setTransactionID(toU32(params.transactionID));
  packet.setStatus(toU8(params.status));
  packet.setExtendedAddress(toU8(params.extendedAddress));
  packet.setInitiatorLogicalAddress(toU8(params.initiatorLogicalAddress));
  packet.setTargetLogicalAddress(toU8(params.targetLogicalAddress));
  packet.setTargetSpaceWireAddress(toU8Array(params.targetSpaceWireAddress));

  for (const u8 byte : *packet.getPacketBufferPointer()) {
    ost << std::hex << std::setw(2) << std::setfill('0') << static_cast<u32>(byte) << " ";
  }
  ost << '\n';
}

int main(int argc, char* argv[]) {
  CLI::App app{"CLI for interacting with RMAP packets"};

  auto subcomParse = app.add_subcommand("parse", "Parse hex array as an RMAP packet");
  auto subcomConstruct = app.add_subcommand("construct", "Construct an RMAP packet");

  app.require_subcommand(1, 1);

  std::string filename{};
  RMAPPacketOption packetParams;
  if (subcomParse) {
    subcomParse->add_option("-f,--file", filename, "Read space-separated uint8 array from a file");
    subcomParse->allow_extras();
  }
  if (subcomConstruct) {
    subcomConstruct->add_option("-f,--file", filename, "Write space-separated uint8 array to a file");

    subcomConstruct->add_option("-t,--type", packetParams.type, "command (c) or reply (r)")
        ->required()
        ->check(CLI::IsMember({"command", "c", "reply", "r"}));
    subcomConstruct->add_option("-i,--instruction", packetParams.instruction, "read (r) or write (w)")
        ->required()
        ->check(CLI::IsMember({"read", "r", "write", "w"}));
    subcomConstruct->add_option("-k,--key", packetParams.key, "Key (hex). Default = 0x00");
    subcomConstruct->add_option("-R,--reply-address", packetParams.replyAddress,
                                "Reply address (hex array). Default = empty");
    subcomConstruct->add_option("-a,--memory-address,--address", packetParams.memoryAddress, "Memory address (hex)")
        ->required();
    subcomConstruct->add_option("-d,--data", packetParams.data, "Data (hex array). Default = empty");
    subcomConstruct->add_option("-l,--length", packetParams.dataLength, "Length in bytes (decimal)");
    subcomConstruct->add_option("--tid,--transaction-id", packetParams.transactionID,
                                "Transaction ID (hex). Default = 0x0000");

    subcomConstruct->add_option("-s,--status", packetParams.status, "Reply status (hex). Default = 0x00");
    subcomConstruct->add_option("-e,--extended-address", packetParams.extendedAddress,
                                "Extended address (hex). Default = 0x00");
    subcomConstruct->add_option("-I,--initiator-logical-address", packetParams.initiatorLogicalAddress,
                                "Initiator logical address (hex). Default = 0xFE");
    subcomConstruct->add_option("-T,--target-logical-address", packetParams.targetLogicalAddress,
                                "Target logical address (hex). Default = 0xFE");
    subcomConstruct->add_option("-S,--target-spacewire-address", packetParams.targetSpaceWireAddress,
                                "Target SpaceWire address (hex array). Default = empty");
  }

  CLI11_PARSE(app, argc, argv);

  if (*subcomParse) {
    const std::vector<u8> data = [&]() {
      if (!filename.empty()) {
        std::ifstream ifs(filename);
        if (!ifs) {
          std::cerr << "Failed to open '" << filename << "'" << '\n';
          exit(1);
        }
        return loadFromFile(ifs);
      } else {
        return toU8Array(subcomParse->remaining());
      }
    }();
    commandParse(data);
  }
  if (*subcomConstruct) {
    // Additional check on parameters
    const auto dataLength = toU32(packetParams.dataLength);
    if (!packetParams.data.empty() && dataLength != 0 && packetParams.data.size() != dataLength) {
      std::cerr << "The length of data and the specified data length differ" << '\n';
      return 1;
    }

    const bool useCout = filename.empty();

    if (!filename.empty()) {
      std::ofstream ofs(filename);
      commandConstruct(packetParams, ofs);
    } else {
      const auto coutFlags = std::cout.flags();
      commandConstruct(packetParams, std::cout);
      std::cout.setf(coutFlags);
    }
  }
}
