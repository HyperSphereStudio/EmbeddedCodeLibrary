/**********************************************************************
   NAME: SimpleArduino.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Arduino
        Arduino implementation of the simple library
*********************************************************************/

#ifndef SIMPLE_ARDUINO_C_H
#define SIMPLE_ARDUINO_C_H

#include "../SimpleTimer.hpp"
#include "../SimpleConnection.hpp"

using namespace Simple;

namespace Simple{
    time_t NativeMillis(){
        return millis();
    }

    /**Wrapper of a Arduino Stream to an IO**/
    struct StreamIO : public IO{
        Stream& uart;
        StreamIO(Stream& uart = Serial) : uart(uart){}
        int WriteByte(uint8_t b) { return uart.write(b); }
        int WriteBytes(uint8_t *ptr, int nbytes) final { return uart.write((uint8_t*) ptr, nbytes); }
        int ReadByte() { return uart.read(); }
        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) final { return uart.readBytes((char*) ptr, buffer_size); }
        int BytesAvailable() final { return uart.available(); }
    };

    StreamIO Out, Error;

    /**Wrapper of a Arduino Serial Port to an IO**/
    struct SerialConnection : public StableConnection{
        void ReadFromSocket() override{
            uint8_t buffer[BUFSIZ];
            while(Serial.available() > 0)
                ReceiveBytes(buffer, Serial.readBytes((char*) buffer, BUFSIZ));
        }
        SocketReturn WriteToSocket(PacketInfo& pi, IOBuffer& io, int nbytes) final {
            uint8_t buffer[BUFSIZ];
            while(nbytes > 0){
                int read = io.ReadBytesUnlocked(buffer, nbytes);
                Serial.write(buffer, read);
                nbytes -= read;
            }
            return None;
        }
    };


}

#endif