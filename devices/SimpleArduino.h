#ifndef SIMPLE_ARDUINO_C_H
#define SIMPLE_ARDUINO_C_H

#include "../SimpleTimer.h"
#include "../SimpleConnection.h"

using namespace Simple;

namespace Simple{
    time_t Timer::NativeMillis(){
        return millis();
    }

    struct SerialIO : public IO{
        int WriteByte(uint8_t b) final { return Serial.write(b); }
        int WriteBytes(void *ptr, int nbytes) final { return Serial.write((uint8_t*) ptr, nbytes); }
        int ReadByte() final { return Serial.read(); }
        int ReadBytesUnlocked(void *ptr, int buffer_size) final { return Serial.readBytes((char*) ptr, buffer_size); }
        int BytesAvailable() final { return Serial.available(); }
    };

    SerialIO Out, Error;

    struct SerialConnection : public Connection{
        int WriteByte(uint8_t b) final { return Serial.write(b); }
        int WriteBytes(void *ptr, int nbytes) final { return Serial.write((uint8_t*) ptr, nbytes); }
        int ReadByte() final { return Serial.read(); }
        int ReadBytesUnlocked(void *ptr, int buffer_size) final { return Serial.readBytes((char*) ptr, buffer_size); }
        int BytesAvailable() final { return Serial.available(); }
    };
}

#endif