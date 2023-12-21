/**********************************************************************
   NAME: SimpleFeather.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Arduino
        Feather implementation of the simple library
*********************************************************************/

#ifndef SIMPLE_FEATHER_C_H
#define SIMPLE_FEATHER_C_H

#include "SimpleArduino.hpp"

#include <SPI.h>
#include <RH_RF95.h>

namespace Simple{
    enum Range{
        Short,
        Medium,
        Long,
        UltraLong
    };

    struct RadioPacket : public Packet{
        uint8_t to = 0, from = 0, id = 0;

        RadioPacket(int capacity) : Packet(capacity){}

        void config(int To, int Type, bool reset = true){
            to = To;
            id = Type;
            Packet::config(reset);
        }
    };

    /**Wrapper of the radio to an IO **/
    class RadioConnection : public Connection{
    protected:
        RH_RF95 rf95;
        const uint8_t resetPin;

    public:
        RadioPacket buffer;

        RadioConnection(uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t resetPin, int buffer) :
            rf95(slaveSelectPin, interruptPin), resetPin(resetPin), buffer(RH_RF95_MAX_MESSAGE_LEN){
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

            RH_RF95::ModemConfigChoice modemConfig;
            switch(range){
                case Short: modemConfig = RH_RF95::Bw500Cr45Sf128; break;      //Fast Short
                case Medium: modemConfig = RH_RF95::Bw125Cr45Sf128; break;     //Medium
                case Long: modemConfig = RH_RF95::Bw125Cr45Sf2048; break;      //Long Slow
                case UltraLong: modemConfig = RH_RF95::Bw125Cr48Sf4096; break; //Long Really Slow
            }
            rf95.setModemConfig(modemConfig);
            return true;
        }

        void Send(Packet* p) override{
            rf95.setHeaderTo(((RadioPacket*) p)->to);
            rf95.setHeaderId(((RadioPacket*) p)->id);
            Write(p);
        }

        void Write(IO* in) final {
            buffer.SeekStart();
            rf95.send(buffer.Interpret(0), buffer.ReadFrom(*in));
        }

        TaskReturn Fire() override{
            if(rf95.available()){
                uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
                rf95.recv(buffer.Interpret(0), &len);
                buffer.SetSize(len);
                buffer.from = rf95.headerFrom();
                buffer.id = rf95.headerId();
                buffer.SeekStart();
                Receive(&buffer);
                buffer.SetSize(RH_RF95_MAX_MESSAGE_LEN);
            }
            return TaskReturn::Nothing;
        }
        void SetAddress(int id){ rf95.setThisAddress(id); }
        void Receive(Packet* p) final { Receive((RadioPacket*) p); }
        virtual void Receive(RadioPacket* rp) = 0;
    };
}
#endif