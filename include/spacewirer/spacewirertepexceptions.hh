#ifndef SPACEWIRERTEPEXCEPTIONS_HH_
#define SPACEWIRERTEPEXCEPTIONS_HH_

class SpaceWireRTEPException {
 public:
  SpaceWireRTEPException(int status) : CxxUtilities::Exception(status) {}
  ~SpaceWireRTEPException() = default;

  enum {
    OpenFailed,                                //
    IlleagalOpenDirectionWhileClosingASocket,  //
    NotInTheOpenState,                         //
    SpaceWireIFIsNotWorking,                   //
    Timeout,                                   //
    TooManyRetryFailures,                      //
    NoRoomInSlidingWindow
  };

  std::string toString() {
    std::string result;
    switch (status) {
      case OpenFailed:
        result = "OpenFailed";
        break;
      case IlleagalOpenDirectionWhileClosingASocket:
        result = "IlleagalOpenDirectionWhileClosingASocket";
        break;
      case NotInTheOpenState:
        result = "NotInTheOpenState";
        break;
      case SpaceWireIFIsNotWorking:
        result = "SpaceWireIFIsNotWorking";
        break;
      case Timeout:
        result = "Timeout";
        break;
      case TooManyRetryFailures:
        result = "TooManyRetryFailures";
        break;
      case NoRoomInSlidingWindow:
        result = "NoRoomInSlidingWindow";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

#endif /* SPACEWIRERTEPEXCEPTIONS_HH_ */
