#ifndef SPACEWIRE_SPACEWIRESSDTPMODULE_HH_
#define SPACEWIRE_SPACEWIRESSDTPMODULE_HH_

#include "spacewire/tcpsocket.hh"

#include <array>
#include <memory>
#include <mutex>
#include <vector>

/** An exception class used by SpaceWireSSDTPModule.
 */
class SpaceWireSSDTPException : public Exception {
 public:
  enum {
    DataSizeTooLarge,
    Disconnected,
    ReceiveFailed,
    SendFailed,
    Timeout,
    TCPSocketError,
    EEP,
    SequenceError,
    NotImplemented,
    TimecodeReceiveError,
    Undefined
  };

  SpaceWireSSDTPException(u32 status) : Exception(status) {}
  ~SpaceWireSSDTPException() override = default;
  std::string toString() const override;
};

/** A class that performs synchronous data transfer via
 * TCP/IP network using "Simple- Synchronous- Data Transfer Protocol"
 * which is defined for this class.
 */
class SpaceWireSSDTPModule {
 public:
  explicit SpaceWireSSDTPModule(std::unique_ptr<TCPSocket> socket);
  ~SpaceWireSSDTPModule() = default;

  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] eopType End-of-Packet marker. EOPType::EOP or EOPType::EEP.
   */
  void send(std::vector<u8>* data, EOPType eopType = EOPType::EOP);

  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] the length length of the packet.
   * @param[in] eopType End-of-Packet marker. EOPType::EOP or EOPType::EEP.
   */
  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP);

  /** Tries to receive a pcket from the SpaceWire interface.
   * This method will block the thread for a certain length of time.
   * Timeout can happen via the TCPSocket provided to the instance.
   * The code below shows how the timeout duration can be changed.
   * @code
   * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
   * socket->open();
   * socket->setTimeoutDuration(1000); //ms
   * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
   * try{
   * 	ssdtpModule->receive();
   * }catch(SpaceWireSSDTPException& e){
   * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
   * 	 std::cout << "Timeout" << std::endl;
   * 	}else{
   * 	 throw e;
   * 	}
   * }
   * @endcode
   * @returns packet content.
   */
  std::vector<u8> receive();

  /** Tries to receive a pcket from the SpaceWire interface.
   * This method will block the thread for a certain length of time.
   * Timeout can happen via the TCPSocket provided to the instance.
   * The code below shows how the timeout duration can be changed.
   * @code
   * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
   * socket->open();
   * socket->setTimeoutDuration(1000); //ms
   * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
   * try{
   * 	ssdtpModule->receive();
   * }catch(SpaceWireSSDTPException& e){
   * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
   * 	 std::cout << "Timeout" << std::endl;
   * 	}else{
   * 	 throw e;
   * 	}
   * }
   * @endcode
   * @param[out] data a vector instance which is used to store received data.
   * @param[out] eopType contains an EOP marker type (EOPType::EOP or EOPType::EEP).
   */
  size_t receive(std::vector<u8>* data, EOPType& eopType);

  void close() { closed_ = true; }
  void cancelReceive() { receiveCanceled_ = true; }

 public:
  /* for SSDTP2 */
  static constexpr u8 DATA_FLAG_COMPLETE_EOP = 0x00;
  static constexpr u8 DATA_FLAG_COMPLETE_EEP = 0x01;
  static constexpr u8 DATA_FLAG_FLAGMENTED = 0x02;
  static constexpr u8 CONTROL_FLAG_SEND_TIME_CODE = 0x30;
  static constexpr u8 CONTROL_FLAG_GOT_TIME_CODE = 0x31;
  static constexpr u8 CONTROL_FLAG_CHANGE_TX_SPEED = 0x38;
  static constexpr u8 CONTROL_FLAG_REGISTER_ACCESS_READ_COMMAND = 0x40;
  static constexpr u8 CONTROL_FLAG_REGISTER_ACCESS_READ_REPLY = 0x41;
  static constexpr u8 CONTROL_FLAG_REGISTER_ACCESS_WRITE_COMMAND = 0x50;
  static constexpr u8 CONTROL_FLAG_REGISTER_ACCESS_WRITE_REPLY = 0x51;
  static constexpr u32 LENGTH_OF_SIZE_PART = 10;

  static constexpr u32 BUFFER_SIZE_BYTES = 10 * 1024 * 1024;

 private:
  bool closed_ = false;
  bool receiveCanceled_ = false;

  std::unique_ptr<TCPSocket> socket_{};
  std::vector<u8> sendBuffer_{};
  std::vector<u8> receiveBuffer_{};
  std::array<u8, 12> sendHeader_{};
  std::array<u8, 12> receiveHeader_{};

  std::mutex sendMutex_;
  std::mutex receiveMutex_;
};

#endif
