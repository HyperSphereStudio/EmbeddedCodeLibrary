#ifndef SIMPLE_FEATHER_C_H
#define SIMPLE_FEATHER_C_H

#include "SimpleArduino.h"

#include <RH_RF95.h>
#define RH_HAVE_SERIAL

namespace Simple{
    enum Range{
        Short,
        Medium,
        Long,
        UltraLong
    };

    class RadioIO : public IO{
    protected:
        RH_RF95 rf95;
        const uint8_t resetPin;
    public:
        RadioIO(uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t resetPin) : rf95(slaveSelectPin, interruptPin), resetPin(resetPin){
            pinMode(resetPin, OUTPUT);
            digitalWrite(resetPin, HIGH);
        }

        virtual bool Initialize(float frequency, int8_t power, Range range, bool useRFO = false){
            digitalWrite(resetPin, LOW);
            delay(10);
            digitalWrite(resetPin, HIGH);

            if(!rf95.init()) {
                println("LoRa radio init failed\nUncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
                return false;
            }

            //Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
            if(!rf95.setFrequency(frequency)){
                println("setFrequency failed");
                return false;
            }

            // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
            // The default transmitter power is 13dBm, using PA_BOOST.
            // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
            // you can set transmitter powers from 5 to 23 dBm:
            rf95.setTxPower(power, useRFO);

            ModemConfigChoice modemConfig;
            switch(range){
                case Short: modemConfig = Bw500Cr45Sf128; break;      //Fast Short
                case Medium: modemConfig = Bw125Cr45Sf128; break;     //Medium
                case Long: modemConfig = Bw125Cr45Sf2048; break;      //Long Slow
                case UltraLong: modemConfig = Bw125Cr48Sf4096; break; //Long Really Slow
            }
            rf95.setModemConfig(modemConfig);
            return true;
        }

        int WriteBytes(uint8_t *ptr, int nbytes) final { return rf95.send(ptr, nbytes); }
        int BytesAvailable() final { return rf95.available() ? RH_RF95_MAX_MESSAGE_LEN : 0; }
        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) final { rf95.recv(ptr, (int8_t*) &buffer_size); return buffer_size; }
    };

    class RadioConnection : public AbstractMultiConnection, protected RadioIO{
    protected:
        void setTo(uint8_t t) override { rf95.setHeaderTo(t); }

    public:
        RadioConnection(uint8_t id, uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t resetPin) :
            RadioIO(slaveSelectPin, interruptPin, resetPin), AbstractMultiConnection(*(IO*) this, id){
        }

        bool Initialize(float frequency, int8_t power, Range range, bool useRFO = false) override {
            auto b = RadioIO::Initialize(frequency, power, range, useRFO);
            rf95.setPayloadCRC(false);  //Library comes built in with error checking, no need for extra
            rf95.setPreambleLength(4);  //Get rid of the preamble, internal system has one
            return b;
        }
    };

    struct RadioConnection : public SyncMultiConnection{
        TaskReturn Fire() final{
            if(rf95.available()){
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t size = sizeof(buf);
                while(size > 0){
                    if(!rf95.recv(buf, &size))
                        return TaskReturn::Nothing;
                    ReceiveBytes(buf, size);
                }
            }
            if(!rf95.isChannelActive())
                SyncMultiConnection::UpdateWrite();
            rf95.setModeRx();
            return TaskReturn::Nothing;
        }
    };
}
#endif