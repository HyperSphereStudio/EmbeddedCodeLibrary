#ifndef SIMPLE_PACKET_PROTOCOL_C_H
#define SIMPLE_PACKET_PROTOCOL_C_H

#include "stdint.h"
#include "malloc.h"
#include "math.h"

#include "SimpleDebug.h"
#include "SimpleTimer.h"
#include "SimpleIO.h"
#include "SimpleLock.h"
#include <numeric>
#include <vector>

#define MIN(a, b) (a) > (b) ? (a) : (b)


//[222, 173, 190, 239]
const uint32_t MAGIC_NUMBER = 0xDEADBEEF; //PACK in bytes
//[238]
const uint8_t TAIL_MAGIC_NUMBER = 0xEE;

const uint8_t ReceivedPacketType = 255;
const uint8_t SynchronizeTimePacketType = 254;

namespace Simple{
    /*Default Structure
     * net uint32_t MAGIC_NUMBER_CONSTANT
     * uint8_t PayLoad_Size
     * uint8_t Payload_Type
     * uint8_t Payload_Id
     * net PayLoad
     * uint8_t TAIL_MAGIC_NUMBER_CONSTANT
     * */

    /*Dynamic Structure (Internal Memory)
     * transient uint8_t retry_count
     * transient uint16_t time_since_last_sent
     * net uint32_t MAGIC_NUMBER_CONSTANT
     * uint8_t[PreambleLength] Preamble
     * net PayLoad
     * uint8_t TAIL_MAGIC_NUMBER_CONSTANT
     * **/

    struct PacketInfo{
        uint8_t Size, Type, ID, Retries;
        uint16_t LastRetryTime;
    };

    class AbstractConnection : public Task{
        uint8_t RetryCount;
        uint16_t Timeout;
        time_t ClockOffset, ClockDelta;

    public:
        AbstractConnection(uint8_t retries, uint16_t timeout) : RetryCount(retries), Timeout(timeout), ClockOffset(Timer::NativeMillis()), ClockDelta(0){}
        void SetClockOffset(time_t t){
            ClockDelta = ClockOffset - t;
            ClockOffset = t;
        }
        inline uint16_t GetClockNow(){ return Timer::NativeMillis() - ClockOffset; }
        TaskReturn Fire() final {

            if(Timer::NativeMillis() - ClockOffset > numeric_limits<uint16_t>::max() - max(5000, (int) Timeout))
                SetClockOffset(Timer::NativeMillis());  //Reset Clock Offset

            ReadFromSocket();
            WritePackets();

            return TaskReturn::Nothing;
        }

        inline size_t ReadBufferSize() { return read_buffer.Size(); }
        inline size_t WriteBufferSize() { return write_buffer.Size(); }

    protected:
        IOBuffer read_buffer, write_buffer;
        volatile uint8_t packet_count = 0;

        void SendData(PacketInfo& info, Lambda<void (IOBuffer&)>& callback){
            write_buffer.SeekEnd();
            auto start = write_buffer.Position();
            info.ID = packet_count++;
            info.Retries = 0;
            info.LastRetryTime = 0;
            WritePacketInfo(info, true);
            auto start_payload = write_buffer.Position();
            callback(write_buffer);
            info.Size = write_buffer.Position() - start_payload;
            write_buffer.WriteStd(TAIL_MAGIC_NUMBER);
            write_buffer.Seek(start);
            bool dispose = false;
            bool write = CanWritePacket(info, dispose);
            InternalWritePacket(info, write, dispose);
        }
        virtual void onPacketReceived(PacketInfo& p, IOBuffer& io) = 0;
        virtual void onPacketCorrupted(PacketInfo& p) = 0;
        virtual size_t WritePacketInfo(PacketInfo& p, bool writeTransient){
            if(writeTransient)
                write_buffer.Write(p.Retries, p.LastRetryTime);
            auto pos = write_buffer.Position();
            write_buffer.WriteStd(MAGIC_NUMBER);
            write_buffer.Write(p.Size, p.Type, p.ID);
            return pos;
        }
        virtual bool ReadPacketInfo(PacketInfo& p, IOBuffer& io, bool readTransient){
            if(io.BytesAvailable() >= 3 + readTransient ? 7 : 0){
                if(readTransient){
                    p.Retries = io.ReadByte();
                    p.LastRetryTime = io.Read<uint16_t>();
                    io.SeekDelta(sizeof(MAGIC_NUMBER));
                }
                io.ReadBytesUnlocked((uint8_t*) &p, 3); //Write Size, Type, ID
                return true;
            }
            return false;
        }
        virtual void ReadFromSocket() = 0;
        virtual void WriteToSocket(IOBuffer& buffer, int length) = 0;
        virtual bool CanWritePacket(PacketInfo& pi, bool& dispose){
            if(pi.Type == ReceivedPacketType){  //Force Dispose And Write
                dispose = true;
                return true;
            }
            if(GetClockNow() - pi.LastRetryTime > Timeout){
                pi.LastRetryTime = GetClockNow();
                dispose = ++pi.Retries == RetryCount;
                return true;
            }
            dispose = false;
            return false;
        }
        virtual bool HandleRxPacket(PacketInfo& rxp, PacketInfo& lookp, uint8_t info_type) { return lookp.ID == info_type; }
        virtual void SendRxPacket(PacketInfo& p) = 0;
        virtual bool HandlePacket(PacketInfo& info, IOBuffer& io){
            switch(info.Type){
                case ReceivedPacketType: {
                    auto ty = io.Read<uint8_t>();
                    define_local_lambda(lam, [&], bool, (PacketInfo& pi, bool & dispose), dispose = HandleRxPacket(info, pi, ty); return false;);
                    WalkPackets(lam);
                    return true;
                }
                case SynchronizeTimePacketType:
                    SetClockOffset(io.Read<uint32_t>());
                    return true;
                default:
                    SendRxPacket(info);
                    return false;
            }
        }
        virtual void WritePackets(){
            define_local_lambda(lam, [&], bool, (PacketInfo& pi, bool& dispose), return CanWritePacket(pi, dispose));
            WalkPackets(lam);
        }
        virtual int PacketInfoSize(){ return sizeof(PacketInfo); }
        void InternalWritePacket(PacketInfo& info, bool write, bool dispose){
            auto data_start = write_buffer.Position();
            auto packet_start = WritePacketInfo(info, true);
            auto packet_head_length = write_buffer.Position() - packet_start;
            auto len = info.Size + sizeof(TAIL_MAGIC_NUMBER) + packet_head_length;
            write_buffer.Seek(packet_start);
            if(write)
                WriteToSocket(write_buffer, len);
            if(dispose){
                write_buffer.RemoveRange(data_start, packet_start + len);
                write_buffer.Seek(data_start);
            }else write_buffer.Seek(packet_start + len);
        }
        void WalkPackets(Lambda<bool (PacketInfo&, bool&)>& onPacket){
            write_buffer.Reset();
            uint8_t info_storage[PacketInfoSize()];
            auto& info = reinterpret_cast<PacketInfo&>(info_storage);
            auto startPtr = write_buffer.Position();
            while(ReadPacketInfo(info, write_buffer, true)){ //Has Packet
                info.LastRetryTime += ClockDelta;                                      //Adjust to new time
                write_buffer.Seek(startPtr);
                bool dispose = false;
                bool write = onPacket(info, dispose);
                InternalWritePacket(info, write, dispose);
                startPtr = write_buffer.Position();
            }
        }
        void ReceiveBytes(uint8_t* data, int nbytes){
            if (nbytes > 0) {
                read_buffer.SeekEnd();
                read_buffer.WriteBytes(data, nbytes);         //Copy to read buffer
                read_buffer.SeekStart();
                uint32_t head = 0;

                while(read_buffer.BytesAvailable() >= sizeof(MAGIC_NUMBER)){
                    while(read_buffer.TryReadStd(&head) && (head != MAGIC_NUMBER))
                        read_buffer.SeekDelta(-3); //Try the next Byte

                    if(head == MAGIC_NUMBER) {
                        uint8_t info_storage[PacketInfoSize()];
                        auto& info = reinterpret_cast<PacketInfo&>(info_storage);
                        if (ReadPacketInfo(info, read_buffer, false)) {
                            auto start_packet_pos = read_buffer.Position();
                            if(read_buffer.BytesAvailable() >= info.Size + sizeof(TAIL_MAGIC_NUMBER)) {
                                read_buffer.SeekDelta(info.Size);                                       //Peak Ahead to make sure packet is not corrupted!
                                if (read_buffer.Read<uint8_t>() != TAIL_MAGIC_NUMBER) {
                                    onPacketCorrupted(info);
                                    continue;
                                }
                                auto end_packet_pos = read_buffer.Position();
                                read_buffer.Seek(start_packet_pos);
                                if(!HandlePacket(info, read_buffer))
                                    onPacketReceived(info, read_buffer);
                                read_buffer.Seek(end_packet_pos);
                            }
                        }else return;
                    }else break;
                }
                read_buffer.ClearToPosition();
            }
        }
    };

    struct Connection : public AbstractConnection{
        template<typename ...Args> void Send(uint8_t type, Args... args){
            auto t = tuple<Args...>(args...);
            define_local_lambda(lam, [&], void, (IOBuffer& io), io.WriteStd(t));
            SendData(type, lam);
        }
        void SendData(uint8_t type, IO& io){ SendData(type, io, io.BytesAvailable()); }
        void SendData(uint8_t type, Lambda<void (IOBuffer&)>& callback){
            PacketInfo info;
            info.Type = type;
            AbstractConnection::SendData(info, callback);
        }
        void SendData(uint8_t type, IO& io, int count){
            define_local_lambda(lam, capture(=, &io), void, (IOBuffer& rb), rb.ReadFrom(io, count));
            SendData(type, lam);
        }
        void SendRxPacket(PacketInfo& p) override{ Send(ReceivedPacketType, p.Type); }
    };

    struct MultiPacketInfo : public PacketInfo{
        uint8_t To, From;
    };

    struct MultiConnection : public AbstractConnection{
    protected:
        bool HandleRxPacket(PacketInfo& rx_info, PacketInfo& info, uint8_t info_type) override {return up(rx_info).From == up(info).To && info.Type == info_type; }
        bool HandlePacket(PacketInfo& info, IOBuffer& io) override {
            if(up(info).To == ID)
                return AbstractConnection::HandlePacket(info, io);
            return true;
        }
        void SendRxPacket(PacketInfo& p) override{ Send(up(p).From, ReceivedPacketType, p.Type);}
        int PacketInfoSize() override { return sizeof(MultiPacketInfo); }
        size_t WritePacketInfo(PacketInfo& p, bool writeTransient) override{
            auto v = AbstractConnection::WritePacketInfo(p, writeTransient);
            write_buffer.Write(up(p).From, up(p).To);
            return v;
        }
        bool ReadPacketInfo(PacketInfo& p, IOBuffer& io, bool readTransient) override{
            if(io.BytesAvailable() >= 5 + readTransient ? 7 : 0){
                AbstractConnection::ReadPacketInfo(p, io, readTransient);
                up(p).From = io.ReadByte();
                up(p).To = io.ReadByte();
                return true;
            }
            return false;
        }
        inline MultiPacketInfo& up(PacketInfo& p){ return reinterpret_cast<MultiPacketInfo&>(p); }
    public:
        const uint8_t ID;

        MultiConnection(uint8_t id, uint8_t retries, uint16_t timeout): AbstractConnection(retries, timeout), ID(id){}

        template<typename ...Args> inline void Send(uint8_t to, uint8_t type, Args&&... args){
            define_local_lambda(lam, [=], void, (IOBuffer& io), io.WriteStd(args...));
            SendData(to, type, lam);
        }

        template<typename ...Args> inline void SendData(uint8_t to, uint8_t type, Lambda<void (IOBuffer&, Args...)>& callback){
            MultiPacketInfo info;
            info.Type = type;
            info.To = to;
            info.From = ID;
            AbstractConnection::SendData(info, callback);
        }

        inline void SendData(uint8_t to, uint8_t type, Simple::IO& io, int count){
            define_local_lambda(lam, capture(=, &io), void, (IOBuffer& rb), rb.ReadFrom(io, count));
            SendData(to, type, lam);
        }
        inline void SendData(uint8_t to, uint8_t type, Simple::IO& io){ SendData(to, type, io, io.BytesAvailable()); }
    };

    /*
    struct SyncMultiConnection : public MultiConnection{
    public:
        uint8_t LastRxID = 0;
        uint32_t LastRxTime = 0;
        uint16_t NodeReadTimeOut;

        SyncMultiConnection(uint8_t id, uint16_t node_timeout) : MultiConnection(id), NodeReadTimeOut(node_timeout){}

        //Round Robin Communication
        TaskReturn Fire() override{
            UpdateWrite();
            return Connection::Fire();
        }

        void SkipNodeWrite(){
            if(++LastRxID >= connections)
                LastRxID = 0;
        }

        //Round Robin Communication
        void FlushWriteBuffer() override{
            if((LastRxID + 1 == ID) || (LastRxID + 1 == connections && ID == 0)){
                if(!BytesAvailable()){
                    Connection::FlushWriteBuffer();
                    SkipNodeWrite();
                }
            }
        }

        void onPacketReceived(MultiPacketHeader& header, IOBuffer& io) override{ LastRxTime = Timer::Millis(); }

    protected:
        void UpdateWrite(){
            if((Timer::Millis() - LastRxTime > NodeReadTimeOut) && !BytesAvailable()){
                SkipNodeWrite();
                FlushWriteBuffer();
                LastRxTime = Timer::Millis();
            }
        }
    };
    
    */
}

#endif