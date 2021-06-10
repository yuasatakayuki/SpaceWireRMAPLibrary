/* 
   ============================================================================
   SpaceWire/RMAP Library is provided under the MIT License.
   ============================================================================

   Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project
   Copyright (c) 2020- 

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
 * SpaceWireIFOverSPI.hh
 *
 *  Created on: May 19, 2020
 *      Author: takada
 */

#ifndef SPACEWIREIFOVERSPI_HH_
#define SPACEWIREIFOVERSPI_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireIF.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireSpacePiModule.hh"

/** SpaceWire IF class which is connected to a real SpaceWire IF
 * via SPI interface on SpacePi.
 */
class SpaceWireIFOverSPI : public SpaceWireIF, public SpaceWireIFActionTimecodeScynchronizedAction {
    private:
        bool opened;
        SpaceWireSpacePiModule* spacepi;

    public:
        enum {
            InitiatorMode, TargetMode
        };

    private:
        uint32_t operationMode;

    public:
        SpaceWireIFOverSPI() :
            SpaceWireIF() {
            }

        virtual ~SpaceWireIFOverSPI() {
        }

    public:
        void open() throw (SpaceWireIFException) {

            if (state == Opened) {
                return;
            }

            spacepi = NULL;
            spacepi = new SpaceWireSpacePiModule();
            if(spacepi->open() != 0)
                state = Closed;
            else
                state = Opened;
        }

        void close() throw (SpaceWireIFException) {
            using namespace CxxUtilities;
            using namespace std;
            if (state == Closed) {
                return;
            }
            state = Closed;
            //invoke SpaceWireIFCloseActions to tell other instances
            //closing of this SpaceWire interface
            invokeSpaceWireIFCloseActions();
            if(spacepi != NULL){
                spacepi->close();
                delete spacepi;
            }

        }

    public:
        void send(uint8_t* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
            throw (SpaceWireIFException) {
                using namespace std;
                try {
                    spacepi->send(data, length); // TODO: 
                } catch (SpaceWireIFException& e) {
                    throw SpaceWireIFException(SpaceWireIFException::Disconnected);
                }
                // dummy wait to avoid buffer bug
                //usleep(3000);
            }

    public:
        void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) {
            try {
                uint32_t length, i;
                uint8_t buf[2056];
                length = 2056;

                spacepi->receive(&buf[0], &length);

                buffer->resize(length);
                if(length != 0){  // SPI DATA copy
                    memcpy(&(buffer->at(0)), buf+2, length);
                }

               // for(i=2;i<length+2;i++) { 
               //     printf("%02X ", buf[i]);
               // }
               // printf("\n");
            } catch (SpaceWireSpacePiException& e){
                if(e.getStatus() == SpaceWireSpacePiException::Timeout){
                    throw SpaceWireIFException(SpaceWireIFException::Timeout);
                }
            } catch (SpaceWireIFException& e) {
                using namespace std;
                throw SpaceWireIFException(SpaceWireIFException::Disconnected);
            }
        }

    public:
        void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::emitTimecode() is not implemented." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
        }

    public:
        virtual void setTxLinkRate(uint32_t linkRateType) throw (SpaceWireIFException) {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::setTxLinkRate() is not implemented." << endl;
            cerr << "Please use SpaceWireIFOverIPClient::setTxDivCount() instead." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
        }

        virtual uint32_t getTxLinkRateType() throw (SpaceWireIFException) {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::getTxLinkRate() is not implemented." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
        }

    public:
        void setTxDivCount(uint8_t txdivcount) {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::setTxDivCount() is not implemented." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
        }

    public:
        void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) {
            spacepi->setTimeout(microsecond/1000.);
            timeoutDurationInMicroSec = microsecond;
        }

    public:
        uint8_t getTimeCode() throw (SpaceWireIFException) {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::getTimeCode() is not implemented." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);

            return 0;  //TODO
        }

        void doAction(uint8_t timecode) {
            this->invokeTimecodeSynchronizedActions(timecode);
        }

    public:
        uint32_t getOperationMode() const {
            using namespace std;
            cerr << "SpaceWireIFOverSPI::getOperationMode() is not implemented." << endl;
            throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);

            return InitiatorMode;
        }


    public:
        /** Cancels ongoing receive() method if any exist.
        */
        void cancelReceive(){}
};

#endif /* SPACEWIREIFOVERSPI_HH_ */
