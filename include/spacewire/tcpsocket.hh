#ifndef SPACEWIRE_TCPSOCKET_HH_
#define SPACEWIRE_TCPSOCKET_HH_

#include "spacewire/types.hh"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

class TCPSocketException : public Exception {
 public:
  enum {
    Disconnected,
    Timeout,
    TCPSocketError,
    PortNumberError,
    BindError,
    ListenError,
    AcceptException,
    OpenException,
    CreateException,
    HostEntryError,
    ConnectFailed,
    ConnectExceptionWhenChangingSocketModeToNonBlocking,
    ConnectExceptionWhenWaitingForConnection,
    ConnectExceptionNonBlockingConnectionImmediateluSucceeded,
    ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult,
    ConnectExceptionWhenChangingSocketModeToBlocking,
    TimeoutDurationCannotBeSetToDisconnectedSocket,
    Undefined
  };

  TCPSocketException(u32 status) : Exception(status) {}

  std::string toString() const override {
    std::string result;
    switch (status_) {
      case Disconnected:
        result = "Disconnected";
        break;
      case Timeout:
        result = "Timeout";
        break;
      case TCPSocketError:
        result = "TCPSocketError";
        break;
      case PortNumberError:
        result = "PortNumberError";
        break;
      case BindError:
        result = "BindError";
        break;
      case ListenError:
        result = "ListenError";
        break;
      case AcceptException:
        result = "AcceptException";
        break;
      case OpenException:
        result = "OpenException";
        break;
      case CreateException:
        result = "CreateException";
        break;
      case HostEntryError:
        result = "HostEntryError";
        break;
      case ConnectFailed:
        result = "ConnectFailed";
        break;
      case ConnectExceptionWhenChangingSocketModeToNonBlocking:
        result = "ConnectExceptionWhenChangingSocketModeToNonBlocking";
        break;
      case ConnectExceptionWhenWaitingForConnection:
        result = "ConnectExceptionWhenWaitingForConnection";
        break;
      case ConnectExceptionNonBlockingConnectionImmediateluSucceeded:
        result = "ConnectExceptionNonBlockingConnectionImmediateluSucceeded";
        break;
      case ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult:
        result = "ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult";
        break;
      case ConnectExceptionWhenChangingSocketModeToBlocking:
        result = "ConnectExceptionWhenChangingSocketModeToBlocking";
        break;
      case TimeoutDurationCannotBeSetToDisconnectedSocket:
        result = "TimeoutDurationCannotBeSetToDisconnectedSocket";
        break;
      case Undefined:
        result = "Undefined";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

class TCPSocket {
 public:
  enum class State { TCPSocketInitialized, TCPSocketCreated, TCPSocketBound, TCPSocketListening, TCPSocketConnected };

  TCPSocket() = default;
  TCPSocket(u16 port);
  virtual ~TCPSocket() = default;
  virtual void close(){};

  /** Sends data stored in the data buffer for a specified length.
   * @param[in] data uint8_t buffer that contains sent data
   * @param[in] length data length in bytes
   * @return sent size
   */
  size_t send(const u8* data, size_t length);

  /** Receives data until specified length is completely received.
   * Received data are stored in the data buffer.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length length of data to be received
   * @return received size
   */
  size_t receiveLoopUntilSpecifiedLengthCompletes(u8* data, size_t length);

  /** Receives data. The maximum length can be specified as length.
   * Receive size may be shorter than the specified length.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length the maximum size of the data buffer
   * @return received size
   */
  size_t receive(u8* data, size_t length, bool waitUntilSpecifiedLengthCompletes = false);

  void setSocketDescriptor(u32 socketdescriptor) { this->socketDescriptor_ = socketdescriptor; }

  /** Sets time out duration in millisecond.
   * This method can be called only after a connection is opened.
   * @param[in] durationInMilliSec time out duration in ms
   */
  void setTimeout(f64 durationInMilliSec);
  void setNoDelay();

  f64 getTimeoutDuration() const { return timeoutDurationInMilliSec_; }
  u16 getPort() const { return port_; }
  void setPort(u16 port) { port_ = port; }
  TCPSocket::State getStatus() const { return status_; }
  bool isConnected() const { return status_ == State::TCPSocketConnected; }

 protected:
  State status_{State::TCPSocketInitialized};
  i32 socketDescriptor_{};
  u16 port_{};

 private:
  f64 timeoutDurationInMilliSec_{};
};

/** A class that represents a socket which is created by a server
 * when a new client is connected. This is used to perform multi-client
 * data transfer with TCPSocket server socket.
 */
class TCPServerAcceptedSocket : public TCPSocket {
 public:
  /** Constructs an instance.
   */
  TCPServerAcceptedSocket();
  ~TCPServerAcceptedSocket() override = default;

  /** Clones an instance.
   * This method is just for debugging purposes and not for ordinary user application.
   */
  void close() override;

  void setAddress(const sockaddr_in& address);

 private:
  ::sockaddr_in clientAddress_;
};

/** A class that represents a TCP server socket.
 */
class TCPServerSocket : public TCPSocket {
 public:
  TCPServerSocket(u16 port);
  ~TCPServerSocket() override = default;

  void open();
  void close() override;
  TCPServerAcceptedSocket* accept();
  TCPServerAcceptedSocket* accept(f64 timeoutDurationInMilliSec);

 private:
  void create();
  void bind();
  void listen();

  static constexpr i32 maxofconnections = 5;
};

class TCPClientSocket : public TCPSocket {
 public:
  TCPClientSocket(std::string ipHostname, u16 port);
  ~TCPClientSocket() override = default;

  void open(f64 timeoutDurationInMilliSec = 1000);
  void close() override;

 private:
  void create();
  void connect(f64 timeoutDurationInMilliSec);

  std::string ipHostname_;
};

#endif /* SPACEWIRE_TCPSOCKET_HH_ */
