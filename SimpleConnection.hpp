/**********************************************************************
   NAME: SimpleConnection.h
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Connection
		Medium Level Network library that provides access to several different protocols with error minimization
*********************************************************************/

#ifndef SIMPLE_PACKET_PROTOCOL_C_H
#define SIMPLE_PACKET_PROTOCOL_C_H

#include "stdint.h"
#include "malloc.h"
#include "math.h"

#include "SimpleDebug.hpp"
#include "SimpleTimer.hpp"
#include "SimpleIO.hpp"
#include "SimpleLock.hpp"
#include <numeric>
#include <vector>

//[222, 173, 190, 239]
const uint32_t MAGIC_NUMBER = 0xDEADBEEF; //PACK in bytes
//[238]
const uint8_t TAIL_MAGIC_NUMBER = 0xEE;

namespace Simple {
    struct Packet : public IOArray{
        explicit Packet(int capacity = 256) : IOArray(capacity){}

        void config(bool reset = true){
            if(reset)
                Clear();
        }
    };

    class Connection : public Task{
    public:
        virtual void Write(IO* p) = 0;
        virtual void Send(Packet* p){ Write(p); }
        virtual void Receive(Packet* p) = 0;
        TaskReturn Fire() override { return TaskReturn::Nothing; }
    };

    class SimpleConnection : public Connection{
        IOArray write_buffer;
        Packet read_buffer;
    public:

        explicit SimpleConnection(int capacity = 256) : write_buffer(capacity), read_buffer(capacity){}

        void Send(Packet* p) override {
            write_buffer.Clear();
            write_buffer.WriteStd(MAGIC_NUMBER);
            write_buffer.Write<uint8_t>(0);

            auto start = write_buffer.Position();
            write_buffer.ReadFrom(*p);
            auto length = write_buffer.Position() - start;
            *write_buffer.Interpret(4) = length;                //Position of the size

            write_buffer.WriteStd<uint8_t>(TAIL_MAGIC_NUMBER);

            write_buffer.SeekStart();

            Write(&write_buffer);
        }

        void Receive(Packet* io) override {
            read_buffer.SeekEnd();
            read_buffer.ReadFrom(*io);
            read_buffer.SeekStart();

            uint32_t maybe_number = 0;

            while(read_buffer.TryReadStd(&maybe_number) && maybe_number != MAGIC_NUMBER)
                read_buffer.SeekDelta(-3);   //Read next byte

            uint8_t read_size = 0;
            if(maybe_number == MAGIC_NUMBER && read_buffer.TryReadStd(&read_size)){
                auto pos = read_buffer.Position();
                uint8_t tail = 0;

                read_buffer.SeekDelta(read_size);
                if(!read_buffer.TryReadStd(&tail)) return;
                read_buffer.Seek(pos);

                if(tail == TAIL_MAGIC_NUMBER){
                    auto rbs = read_buffer.Size();
                    read_buffer.SetBytesAvailable(read_size);
                    ReceivedMessage(&read_buffer);
                    read_buffer.SetSize(rbs);

                    read_buffer.Seek(pos + read_size + sizeof(TAIL_MAGIC_NUMBER));
                }

                read_buffer.ClearToPosition();
            }else
                read_buffer.ClearToPosition();      //Junk Data
        }

        virtual void ReceivedMessage(Packet* io) = 0;
    };

    class ConnectionIO : public Connection{
        IO* io;

    public:
        explicit ConnectionIO(IO* io) : io(io){}

    protected:
        void Write(IO* in) override { io->ReadFrom(*in); }
    };
}

#endif