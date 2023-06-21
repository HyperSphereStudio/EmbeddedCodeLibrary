#ifndef Simple_IO_C_H
#define Simple_IO_C_H

#include "SimpleLambda.h"
#include "SimpleCore.h"
#include "stdarg.h"
#include "string.h"
#include <stdio.h>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <array>
#include <vector>
#include <string>

#include "SimpleMath.h"

#define print(fmt, ...) Out.Printf(fmt, ##__VA_ARGS__)
#define println(fmt, ...) print(fmt "\r\n", ##__VA_ARGS__)
#define printerr(fmt, ...) Error.Printf(fmt, ##__VA_ARGS__)
#define printerrln(fmt, ...) printerr(fmt "\r\n", ##__VA_ARGS__)

namespace Simple {
    using namespace std;

    enum Endianness { BIG, LITTLE, Unknown };
/*
#ifndef __BYTE_ORDER__
#error "Byte Order Not Defined! Please Define It. Set it to __ORDER_LITTLE_ENDIAN__ or __ORDER_BIG_ENDIAN__. Error will throw if its wrong"
#endif
*/
    static bool InitializeIO();

    struct IOBuffer;

    //Static Inherited Design for CompileTime Inheritence
    struct IO {
        virtual int BytesAvailable() = 0;
        virtual int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) = 0;
        virtual int WriteBytes(uint8_t *ptr, int nbytes) = 0;
        int WriteByte(uint8_t b){ return WriteBytes(&b, 1); }
        int ReadByte(){ uint8_t b; return ReadBytesUnlocked(&b, 1) == 1 ? b : -1; }

        int ReadFrom(IO& io){ return ReadFrom(io, io.BytesAvailable()); }
        int ReadFrom(IO& io, int bytes){
            bytes = min(bytes, io.BytesAvailable());
            int buf_size = min(BUFSIZ, bytes);
            uint8_t buffer[buf_size];
            int bytes_read = 0;
            while (io.BytesAvailable() > 0 && bytes > 0){
                int read = io.ReadBytesUnlocked(buffer, min(buf_size, bytes));
                bytes_read += read;
                bytes -= read;
                WriteBytes(buffer, read);
            }
            return bytes_read;
        }
        int WriteTo(IO& io) { return WriteTo(io, BytesAvailable()); }
        int WriteTo(IO& io, int bytes){ return io.ReadFrom(*this, bytes); }

        void WriteString(const char *str) { WriteString((char *) str); }

        void WriteString(char *str) {
            if (str != nullptr) {
                WriteArray(str, strlen(str));
            }else
                WriteString("null");
        }

        void WriteUnsafeString(char* str){
            if (str != nullptr) {
                WriteBytes((uint8_t*) str, strlen(str));
            }else
                WriteUnsafeString("null");
        }

        void WriteUnsafeString(const char* str){
            return WriteUnsafeString((char*) str);
        }

        template<typename T> void WriteArray(T* a, int length){
            WriteStd<uint32_t>(length);
            WriteBytes((uint8_t*) a, length * sizeof(T));
        }

        char Dig2Char(int i) { return (i >= 0 && i <= 9) ? (char) ('0' + i) : '?'; }

        void PrintUInt64(char *buffer, uint64_t l) {
            if (l == 0) {
                WriteByte('0');
                return;
            }
            int i = 0, k = 0, hl;
            while (l > 0) {
                buffer[i++] = Dig2Char((int) (l % 10));
                l /= 10;
            }
            //Reverse Order
            for (hl = i / 2; k < hl; k++) {
                int idx2 = i - k - 1;
                char tmp = buffer[k];
                buffer[k] = buffer[idx2];
                buffer[idx2] = tmp;
            }
            buffer[i] = '\0';
            WriteUnsafeString(buffer);
        }

        void PrintInt64(char *buffer, int64_t l) {
            if (l < 0) {
                WriteByte('-');
                l *= -1;
            }
            PrintUInt64(buffer, (uint64_t) l);
        }

        void PrintFloat64(char *buffer, double d) {
            if (d < 0) {
                WriteByte('-');
                d *= -1;
            }
            uint64_t units = floor(d);
            PrintUInt64(buffer, units);
            uint64_t decimals = (uint64_t) (10E3 * (d - units));
            if (decimals > 0) {
                WriteByte('.');
                PrintUInt64(buffer, decimals);
            }
        }

        void vPrintbf(char *buffer, char *fmt, va_list sprintf_args) {
            while (true) {
                switch (*fmt) {
                    case '%':
                        fmt++;
                        switch (*(fmt++)) {
                            case 'c':
                                WriteByte((uint8_t) va_arg(sprintf_args, int));
                                break;
                            case 'b':
                                WriteUnsafeString(va_arg(sprintf_args, int) ? "true" : "false");
                                break;
                            case 'i':
                                PrintInt64(buffer, va_arg(sprintf_args, int));
                                break;
                            case 'u':
                                PrintUInt64(buffer, va_arg(sprintf_args, unsigned int));
                                break;
                            case 'l':
                                PrintInt64(buffer, va_arg(sprintf_args, long));
                                break;
                            case 'U':
                                PrintUInt64(buffer, va_arg(sprintf_args, unsigned long));
                                break;
                            case 'f':
                                PrintFloat64(buffer, va_arg(sprintf_args, double));
                                break;
                            case 'd':
                                PrintFloat64(buffer, va_arg(sprintf_args, double));
                                break;
                            case 'p':
                                PrintUInt64(buffer, (uint64_t) va_arg(sprintf_args, void*));
                                break;
                            case 's':
                                WriteUnsafeString(va_arg(sprintf_args, char*));
                                break;
                            case '\0':
                                return;
                            default:
                                WriteByte('%');
                                fmt--;
                                break;
                        }
                        break;
                    case '\0':
                        return;
                    default:
                        WriteByte(*(fmt++));
                        continue;
                }
            }
        }

        void vPrintf(char *fmt, va_list list){
            char buffer[15];
            vPrintbf(buffer, fmt, list);
        }

        void PrintfEnd(char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf(fmt, sprintf_args);
            va_end(sprintf_args);
            WriteByte('\0');
        }

        void PrintfEnd(const char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf((char*) fmt, sprintf_args);
            va_end(sprintf_args);
            WriteByte('\0');
        }

        void Printf(char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf(fmt, sprintf_args);
            va_end(sprintf_args);
        }

        void Printf(const char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf((char*) fmt, sprintf_args);
            va_end(sprintf_args);
        }

        template<typename T> inline void Write(T& t){
            WriteBytes((uint8_t*) &t, sizeof(T));
        }

        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type Write(std::tuple<Tp...>& t){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        Write(std::tuple<Tp...>& t){
            Write(std::get<I>(t));
            Write<I+1, Tp...>(t);
        }
        template<typename... TArgs> inline void Write(TArgs... args){
            auto t = tuple<TArgs...>(args...);
            Write(t);
        }

        template<typename T, bool RequiresByteSwap> void WriteStd(T* v){
            if(RequiresByteSwap){
                auto ptr = (uint8_t *) v;
                std::reverse(ptr, ptr + sizeof(T));
            }
            WriteBytes((uint8_t*) v, sizeof(T));
        }

        template<typename T>
        void WriteStd(T v) {
            if (std::is_integral<T>() || std::is_floating_point<T>()) {
                WriteStd<T,
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        true
#else
                        false
#endif
                >(&v);
            }else WriteStd<T, false>(&v);
        }

        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type WriteStd(std::tuple<Tp...> t){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        WriteStd(std::tuple<Tp...> t){
            WriteStd(std::get<I>(t));
            WriteStd<I+1, Tp...>(t);
        }

        template<typename T, int S> void WriteStd(std::array<T, S>& a){
            for(int i = 0; i < S; i++)
                WriteStd(a[i]);
        }

        template<typename T> void WriteStd(std::vector<T>& a){
            WriteStd((uint32_t) a.size());
            for(int i = 0; i < a.size(); i++)
                WriteStd(a[i]);
        }

        template<typename ...TArgs> void WriteStd(TArgs... args){
            WriteStd(tuple<TArgs...>(args...));
        }

        void ReadBuffer(IOBuffer& buffer);
        int ReadUnsafeString(IOBuffer& buffer) { return ReadStringUntilChars(buffer, false, '\0'); }
        int ReadLine(IOBuffer& buffer) { return ReadStringUntilChars(buffer, true, '\n', '\r'); }
        template<typename... Chars> int ReadStringUntilChars(IOBuffer& buffer, bool greedy, Chars... stop_chars);

        template<typename T>
        void Read(T*t, int count = 1) {
            auto data = (uint8_t *) t;
            int remaining = sizeof(T) * count;
            while (remaining > 0) {
                int read = ReadBytesUnlocked(data, remaining);
                remaining -= read;
                data += read;
            }
        }

        template<typename T>
        T Read() {
            T t;
            Read(&t, 1);
            return t;
        }

        /*Returns position of the array
         * Length Returns the actual size
         * */
        template<typename T> int ReadArray(int* length, IOBuffer& b);
        int ReadString(int* length, IOBuffer& b);

        template<typename T> void ReadStd(T *v) {
            Read(v, 1);
            if(std::is_integral<T>() || std::is_floating_point<T>())
                ReadStd<T,
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        true
#else
                        false
#endif
                >(v);
        }

        template<typename T> void ReadStd(T* v, int count){
            for(int i = 0; i < count; i++)
                ReadStd<T>(v + i);
        }

        template<typename T, bool RequiresByteSwap> void ReadStd(T* v){
            if(RequiresByteSwap){
                auto ptr = (uint8_t *) v;
                std::reverse(ptr, ptr + sizeof(T));
            }
        }

        template<typename T>
        T ReadStd() {
            T t;
            ReadStd(&t);
            return t;
        }

        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type ReadStd(std::tuple<Tp...>* t){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        ReadStd(std::tuple<Tp...>* t){
            ReadStd(&std::get<I>(*t));
            ReadStd<I+1, Tp...>(t);
        }

        template<typename T, int S> void ReadStd(std::array<T, S>* a){
            for(int i = 0; i < S; i++)
                ReadStd<T>(&(*a)[i]);
        }

        template<typename T> void ReadStd(std::vector<T>* a){
            auto s = ReadStd<uint32_t>();
            a->resize(s);
            for(int i = 0; i < s; i++)
                ReadStd<T>(&(*a)[i]);
        }

        template<typename ...TArgs> void ReadStd(Lambda<void (TArgs...)>& lam){
            apply(lam, ReadStd<tuple<TArgs...>>());
        }

        template<typename T> bool TryRead(T* t, int count = 1){
            if(BytesAvailable() >= sizeof(T) * count){
                Read<T>(t, count);
                return true;
            }else return false;
        }

        template<typename T> bool TryReadStd(T* t, int count = 1){
            if(BytesAvailable() >= sizeof(T) * count){
                ReadStd(t, count);
                return true;
            }else return false;
        }

    private:
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type ReadStdArgs(std::tuple<Tp...>& t, std::tuple<Tp&...>& at){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        ReadStdArgs(std::tuple<Tp...>& t, std::tuple<Tp&...>& at){
            std::get<I>(at) = ReadStd(&std::get<I>(t));
            ReadStdArgs<I+1, Tp...>(t, at);
        }
    };

    class IOBuffer : public IO {
        size_t position = 0, max_size;
        std::vector<uint8_t> memory;
    public:
        IOBuffer() : max_size(numeric_limits<long>::max()){}
        IOBuffer(int capacity, long max_size = numeric_limits<long>::max()) : memory(capacity), max_size(max_size) {}
        IOBuffer(void *data, int length, long max_size = numeric_limits<long>::max()) : IOBuffer(length, max_size) {
            WriteBytes((uint8_t*) data, length);
            SeekStart();
        }

        int WriteByte(uint8_t c) {
            if (memory.size() + 1 > max_size)
                return 0;
            if (position >= memory.size()){
                memory.push_back(c);            //Add to end
            }else
                memory[position] = c;           //Overwrite
            position++;

            return 1;
        }

        int WriteBytes(uint8_t *ptr, int nbytes) override{
            if(nbytes == 1)
                return ptr != nullptr ? WriteByte(*(uint8_t*) ptr) : WriteByte(0);
            else if(nbytes > 0){
                if(memory.size() + nbytes > max_size)
                    nbytes = min(max_size, memory.size() + nbytes) - memory.size();
                if(nbytes > 0){
                    if(position + nbytes > memory.size())
                        memory.resize(position + nbytes);
                    if(ptr != nullptr)
                        memcpy(memory.data() + position, ptr, nbytes); //Overwrite
                    position += nbytes;
                }
                return nbytes;
            }
            return 0;
        }

        int ReadByte() { return memory[position++]; }

        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) override{
            if(buffer_size == 1 && BytesAvailable() > 0){
                *ptr = ReadByte();
                return 1;
            }else{
                auto read_bytes = min(buffer_size, BytesAvailable());
                memcpy(ptr, memory.data() + position, read_bytes);
                position += read_bytes;
                return read_bytes;
            }
        }

        inline int BytesAvailable() final { return memory.size() - position; }
        inline size_t Position(){ return position; }
        inline void Seek(size_t pos){ position = pos; }
        inline void SeekDelta(size_t delta){ position += delta; }
        inline void SeekStart(){ position = 0; }
        inline void SeekEnd(){ position = Capacity(); }
        inline size_t Capacity() { return memory.size(); }
        inline size_t Size(){ return memory.size(); }
        inline void SetSize(size_t s){ memory.resize(s); }
        void Print(IO& io){
            io.WriteByte('[');
            for(int i = position; i < memory.size(); i++){
                if(i != position)
                    io.Printf(", %i", memory[i]);
                else io.Printf("%i", memory[i]);
            }
            io.Printf(": Size=%i, Position=%i]\n", Size(), Position());
        }

        void Clear(){
            memory.clear();
            position = 0;
        }

        void ClearToPosition(){
            if(position != 0){
                RemoveRange(0, position);
                position = 0;
            }
        }
        inline void SetMax(size_t max) { max_size = max; }
        template<typename T = uint8_t> T* Interpret(size_t pos){ return (T*) &memory[pos]; }
        template<typename T = uint8_t> T* Interpret(){ return (T*) &memory[position]; }

        int ReadBytes(IOBuffer& b){return ReadBytes(b, b.BytesAvailable()); }
        int ReadBytes(IOBuffer& b, int count){ return WriteBytes(b.Interpret<uint8_t>(), min(count, b.BytesAvailable())); }
        int WriteTo(IO& io){ return WriteTo(io, BytesAvailable()); }
        int WriteTo(IO& io, int count){ return io.WriteBytes(Interpret<uint8_t>(), min(count, BytesAvailable())); }
        inline void Reset(){ position = 0; }
        void RemoveRange(size_t start, size_t end){ memory.erase(memory.begin() + start, memory.begin() + end); }
    };

    template<typename... Chars>
    int IO::ReadStringUntilChars(Simple::IOBuffer& buffer, bool greedy, Chars ...stop_chars) {
        uint8_t c;
        auto pos = buffer.Position();
        bool foundStopChar = false;
        while (true) {
            c = ReadByte();
            for (auto stop_char: {stop_chars...}) {
                if (c == stop_char){
                    if(!greedy){
                        buffer.WriteByte('\0');
                        return pos;
                    }
                    foundStopChar = true;
                }else if(foundStopChar){
                    buffer.WriteByte('\0');
                    return pos;
                }
            }
            if(!foundStopChar && buffer.WriteByte(c) < 1) //Didnt Write
                return pos;
        }
    }

    template<typename T>
    int IO::ReadArray(int *length, Simple::IOBuffer &b){
        auto pos = b.Position();
        *length = ReadStd<uint32_t>();
        WriteTo(b, *length * sizeof(T));
        return pos;
    }

    int IO::ReadString(int *length, Simple::IOBuffer &b){
        auto pos = ReadArray<char>(length, b);
        *length = *length + 1;
        b.WriteByte('\0');
        return pos;
    }

    //Super Classes
    struct FileIO : public IO{
        FILE* out, *in;

        FileIO(FILE* out, FILE* in) : out(out), in(in){}
        int WriteByte(uint8_t c){ return putc(c, out) == EOF ? 0 : 1; }
        int WriteBytes(uint8_t *ptr, int nbytes) final { return fwrite(ptr, nbytes, 1, out); }
        int ReadByte() { return getc(in); }
        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) final { return fread(ptr, 1, buffer_size, in); }
        int BytesAvailable() final { return feof(out) ? 1 : 0; }
    };

    static bool InitializeIO(){
        uint16_t v = 0xdeef;
        auto lead = *(uint8_t*) &v;
        auto machine_endian_type =  lead == 0xef ? Endianness::LITTLE :
                                    lead == 0xde ? Endianness::BIG    :
                                    Endianness::Unknown;
        if(Endianness::Unknown == machine_endian_type){
            return false;
        }
        else if(Endianness::BIG == machine_endian_type){
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            return false;
#endif
        }else{
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
            return false;
#endif
        }
        return true;
    }
}

#endif