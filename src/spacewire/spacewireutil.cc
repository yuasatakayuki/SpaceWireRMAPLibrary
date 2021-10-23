#include "spacewire/spacewireutil.hh"

#include <cassert>
namespace spacewire::util {

/** Dumps packet content to an output stream instance.
 * @param[in] output stream to which dump is performed.
 */
void dumpPacket(std::ostream* ofs, const u8* data, size_t length, size_t wordwidth, size_t bytesPerLine) {
  const auto flags = ofs->flags();
  size_t v{};
  assert(wordwidth != 0);
  assert((length < wordwidth) || ((length % wordwidth) == 0));
  for (size_t i = 0; i < (length / wordwidth + bytesPerLine - 1) / bytesPerLine; i++) {
    for (size_t o = 0; o < bytesPerLine; o++) {
      if (i * bytesPerLine + o < length / wordwidth) {
        v = 0;
        for (size_t p = 0; p < wordwidth; p++) {
          v = v + (u32)data[(i * bytesPerLine + o) * wordwidth + p];
        }
        (*ofs) << "0x" << std::right << std::hex << std::setw(2 * wordwidth) << std::setfill('0') << v << " ";
      }
    }
    (*ofs) << '\n';
  }
  ofs->flags(flags);
}

/** Converts a packet content to std::string. */
std::string packetToString(const u8* data, size_t length, size_t nBytesDisplayed) {
  const size_t dumpLength = std::min(nBytesDisplayed, length);
  if (length == 0) {
    return "(empty packet)";
  }
  std::stringstream ss;
  for (size_t i = 0; i < dumpLength; i++) {
    ss << "0x" << std::right << std::hex << std::setw(2) << std::setfill('0') << static_cast<u32>(data[i]);
    if (i != dumpLength - 1) {
      ss << "  ";
    }
  }
  ss << " ... (" << std::dec << length << " bytes)";
  return ss.str();
}

/** Dumps a packet content with addresses to the screen.
 * @param address initial address.
 * @param data packet content to be dumped.
 */
void printVectorWithAddress(u32 address, const std::vector<u8>* data) {
  using namespace std;
  const auto flags = cout.flags();
  cout << "Address    Data" << endl;
  for (size_t i = 0; i < data->size(); i++) {
    cout << setfill('0') << setw(4) << hex << (address + i) / 0x00010000;
    cout << "-" << setfill('0') << setw(4) << hex << (address + i) % 0x00010000;
    cout << "  ";
    cout << "0x" << setfill('0') << setw(2) << hex << (u32)(data->at(i)) << endl;
  }
  cout << dec << left;
  cout.flags(flags);
}

/** Dumps a packet content with addresses to the screen.
 * @param address initial address.
 * @param data packet content to be dumped.
 */
void printVectorWithAddress2bytes(u32 address, const std::vector<u8>* data) {
  using namespace std;
  const auto flags = cout.flags();

  cout << "Address    Data" << endl;
  size_t size = data->size();
  if (size % 2 == 0 && size != 0) {
    for (size_t i = 0; i < (size + 1) / 2; i++) {
      cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
      cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
      cout << "  ";
      cout << "0x" << setfill('0') << setw(4) << hex << (u32)(data->at(i * 2 + 1) * 0x100 + data->at(i * 2)) << endl;
    }
  } else if (size != 0) {
    size_t i;
    for (i = 0; i < size / 2; i++) {
      cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
      cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
      cout << "  ";
      cout << "0x" << setfill('0') << setw(4) << hex << (u32)(data->at(i * 2 + 1) * 0x100 + data->at(i * 2)) << endl;
    }
    cout << setfill('0') << setw(4) << hex << (address + i * 2) / 0x00010000;
    cout << "-" << setfill('0') << setw(4) << hex << (address + i * 2) % 0x00010000;
    cout << "  ";
    cout << "0x??" << setfill('0') << setw(4) << hex << (u32)(data->at(i * 2)) << endl;
  }
  cout << dec << left;

  cout.flags(flags);
}

};  // namespace spacewire::util
