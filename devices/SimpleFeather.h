#ifndef SIMPLE_FEATHER_C_H
#define SIMPLE_FEATHER_C_H

#include "SimpleArduino.h"

#include <SPI.h>
#include <RH_RF95.h>

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

        int WriteBytes(uint8_t *ptr, int nbytes) final { return rf95.send(ptr, nbytes); }
        int BytesAvailable() final { return rf95.available() ? RH_RF95_MAX_MESSAGE_LEN : 0; }
        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) final {
            uint8_t len = buffer_size;
            if(rf95.recv(ptr, &len))
                return len;
            return 0;
        }
    };

    class RadioConnection : public TDMAMultiConnection, protected RadioIO{
        uint8_t buffer[RH_RF95_MAX_MESSAGE_LEN];
    public:
        RadioConnection(uint8_t id, uint8_t retries, uint16_t retry_timeout, uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t resetPin) :
            RadioIO(slaveSelectPin, interruptPin, resetPin), TDMAMultiConnection(id, retries, retry_timeout){}

        bool Initialize(float frequency, int8_t power, Range range, bool useRFO = false) override {
            auto b = RadioIO::Initialize(frequency, power, range, useRFO);
            rf95.setThisAddress(ID);
            return b;
        }

        bool ReadPacketInfo(PacketInfo& p, IOBuffer& io, bool readTransient) override{
            if(io.BytesAvailable() >= 2 + readTransient ? sizeof(MAGIC_NUMBER) + 2 + sizeof(Time) : 0){
                if(readTransient){
                    p.Retries = io.ReadByte();
                    p.Retry = io.Read<Time>();
                    up(p).To = io.ReadByte();
                    up(p).From = io.ReadByte();
                    p.ID = io.ReadByte();
                    io.SeekDelta(sizeof(MAGIC_NUMBER));
                }else{
                    up(p).To = ID;
                    up(p).From = rf95.headerFrom();
                    p.ID = rf95.headerId();
                }
                p.Size = io.ReadByte();
                p.Type = io.ReadByte();
                return true;
            }
            return false;
        }

        size_t WritePacketInfo(PacketInfo& p, bool writeTransient) override{
            if(writeTransient){
                write_buffer.Write(p.Retries, p.Retry);
                write_buffer.Write(up(p).To, up(p).From, p.ID);
            }
            auto pos = write_buffer.Position();
            write_buffer.WriteStd(MAGIC_NUMBER);
            write_buffer.Write(p.Size, p.Type);
            return pos;
        }

        SocketReturn WriteToSocket(PacketInfo& pi, IOBuffer& io, int nbytes) override {
            if(CanWrite()){
                rf95.setHeaderTo(up(pi).To);
                rf95.setHeaderId(pi.ID);
                rf95.setHeaderFrom(ID);
                rf95.send(io.Interpret(), nbytes);
                return None;
            }else if(write_buffer.Size() > RH_RF95_MAX_MESSAGE_LEN)
                rf95.waitPacketSent(20);    //Prevent
            return DontDispose;
        }

        TaskReturn Fire() override {
            YIELD;
            return TDMAMultiConnection::Fire();
        }

        void ReadFromSocket() override {
            if(RadioIO::BytesAvailable()){
                int bytesRead = RadioIO::ReadBytesUnlocked(buffer, RH_RF95_MAX_MESSAGE_LEN);
                if(bytesRead > 0)
                    ReceiveBytes(buffer, bytesRead);
            }
        }
    };
}
#endif