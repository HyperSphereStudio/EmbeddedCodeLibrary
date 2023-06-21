#ifndef SIMPLE_ARDUINO_C_H
#define SIMPLE_ARDUINO_C_H

#include "../SimpleTimer.h"
#include "../SimpleConnection.h"

using namespace Simple;

namespace Simple{
    time_t NativeMillis(){
        return millis();
    }

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