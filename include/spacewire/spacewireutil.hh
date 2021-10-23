#ifndef SPACEWIRE_SPACEWIREUTIL_HH_
#define SPACEWIRE_SPACEWIREUTIL_HH_

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "spacewire/types.hh"
namespace spacewire::util {

static constexpr size_t DEFAULT_BYTES_PER_LINE = 8;

/** Dumps packet content to an output stream instance.
 * @param[in] output stream to which dump is performed.
 */
void dumpPacket(std::ostream* ofs, const u8* data, size_t length, size_t wordwidth = 16,
                size_t bytesPerLine = DEFAULT_BYTES_PER_LINE);

/** Converts a packet content to std::string. */
std::string packetToString(const u8* data, size_t length, size_t nBytesDisplayed = 16);

/** Dumps a packet content with addresses to the screen.
 * @param address initial address.
 * @param data packet content to be dumped.
 */
void printVectorWithAddress(u32 address, const std::vector<u8>* data);

/** Dumps a packet content with addresses to the screen.
 * @param address initial address.
 * @param data packet content to be dumped.
 */
void printVectorWithAddress2bytes(u32 address, const std::vector<u8>* data);

}  // namespace spacewire::util

#endif
