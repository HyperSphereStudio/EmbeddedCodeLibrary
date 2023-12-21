/**********************************************************************
   NAME: SimpleIO.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple IO
		Medium Level Stream Library built for moving around memory into device portable (network) format
*********************************************************************/

#ifndef Simple_IO_C_H
#define Simple_IO_C_H

#include "SimpleLambda.hpp"
#include "SimpleCore.hpp"
#include "stdarg.h"
#include "string.h"
#include <stdio.h>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <array>
#include <vector>
#include <string>

#include "SimpleMath.hpp"

#ifndef print
    #define print(fmt, ...) Out.Printf(fmt, ##__VA_ARGS__)
#endif

#ifndef printerr
    #define printerr(fmt, ...) Error.Printf(fmt, ##__VA_ARGS__)
#endif

#define println(fmt, ...) print(fmt "\r\n", ##__VA_ARGS__)
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

    struct IOVector;
    struct IOArray;
    struct SeekableIO;

    /**Base Implementation of a stream. Is a wrapper over ports
     **/
    struct IO {
        /** Return the number of bytes that are in the stream that can be read **/
        virtual int BytesAvailable() = 0;

        /** Read bytes from the stream to a buffer without blocking. Return the bytes written **/
        virtual int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) = 0;

        /** Write the bytes from this IO to another IO **/
        virtual int WriteBytes(uint8_t *ptr, int nbytes) = 0;

        /** Write a byte to the stream **/
        int WriteByte(uint8_t b){ return WriteBytes(&b, 1); }

        /** Read a byte from the stream **/
        int ReadByte(){ uint8_t b; return ReadBytesUnlocked(&b, 1) == 1 ? b : -1; }

        /** Read from another IO to this IO **/
        int ReadFrom(IO& io){ return ReadFrom(io, io.BytesAvailable()); }
        int ReadFrom(IO& io, int bytes){ return io.WriteTo(*this, bytes); }

        /** Read $bytes bytes from another IO to this IO  **/
        int WriteTo(IO& io, int bytes){
            int buf_size = min(BUFSIZ, min(bytes, BytesAvailable()));
            int bytes_remaining = bytes;
            uint8_t buffer[buf_size];

            while (bytes_remaining > 0 && BytesAvailable() > 0){
                int read = ReadBytesUnlocked(buffer, min(buf_size, bytes_remaining));
                bytes_remaining -= read;
                io.WriteBytes(buffer, read);
            }

            return bytes;
        }

        /** Write the bytes from this IO to another IO **/
        int WriteTo(IO& io) { return WriteTo(io, BytesAvailable()); }

        /** Write a string safely (with a length) **/
        void WriteString(const char *str) { WriteString((char *) str); }

        /** Write a string safely (with a length) **/
        void WriteString(char *str) {
            if (str != nullptr) {
                WriteArray(str, strlen(str));
            }else
                WriteString("null");
        }

        /** Write a cstring (with a null terminated char) **/
        void WriteUnsafeString(char* str){
            if (str != nullptr) {
                WriteBytes((uint8_t*) str, strlen(str));
            }else
                WriteUnsafeString("null");
        }

        /** Write a cstring (with a null terminated char) **/
        void WriteUnsafeString(const char* str){
            return WriteUnsafeString((char*) str);
        }

        /** Write a c array to the stream with $length size **/
        template<typename T> void WriteArray(T* a, int length){
            WriteStd<uint32_t>(length);
            WriteBytes((uint8_t*) a, length * sizeof(T));
        }

        /** Convert a Digit (value) to a Digit (char) **/
        char Dig2Char(int i) { return (i >= 0 && i <= 9) ? (char) ('0' + i) : '?'; }

        /** Print a UInt64 number to the stream. The buffer should be big enough to handle all digits **/
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

        /** Print a Int64 number to the stream. The buffer should be big enough to handle all digits **/
        void PrintInt64(char *buffer, int64_t l) {
            if (l < 0) {
                WriteByte('-');
                l *= -1;
            }
            PrintUInt64(buffer, (uint64_t) l);
        }

        /** Print a Float64 number to the stream. The buffer should be big enough to handle all digits **/
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

        /**Print to the io using the simple printf impl. The buffer should be able to handle the digits of the biggest number used
         * %c -> char
         * %b -> bool
         * %u -> uint
         * %i -> int
         * %l -> long
         * %f -> float
         * %p -> ptr
         * %U -> ulong
         * %s -> string (null terminated)
         * %d -> double**/
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
            char buffer[15];    //Probaly good enough
            vPrintbf(buffer, fmt, list);
        }

        /**Print a null terminated string to the io using the simple printf impl.
         * %c -> char
         * %b -> bool
         * %u -> uint
         * %i -> int
         * %l -> long
         * %f -> float
         * %p -> ptr
         * %U -> ulong
         * %s -> string (null terminated)
         * %d -> double**/
        void PrintfEnd(char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf(fmt, sprintf_args);
            va_end(sprintf_args);
            WriteByte('\0');
        }


        /**Print a null terminated string to the io using the simple printf impl.
         * %c -> char
         * %b -> bool
         * %u -> uint
         * %i -> int
         * %l -> long
         * %f -> float
         * %p -> ptr
         * %U -> ulong
         * %s -> string (null terminated)
         * %d -> double**/
        void PrintfEnd(const char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf((char*) fmt, sprintf_args);
            va_end(sprintf_args);
            WriteByte('\0');
        }

        /**Print a string to the io using the simple printf impl.
        * %c -> char
        * %b -> bool
        * %u -> uint
        * %i -> int
        * %l -> long
        * %f -> float
        * %p -> ptr
        * %U -> ulong
        * %s -> string (null terminated)
        * %d -> double**/
        void Printf(char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf(fmt, sprintf_args);
            va_end(sprintf_args);
        }

        /**Print a string to the io using the simple printf impl.
                * %c -> char
                * %b -> bool
                * %u -> uint
                * %i -> int
                * %l -> long
                * %f -> float
                * %p -> ptr
                * %U -> ulong
                * %s -> string (null terminated)
                * %d -> double**/
        void Printf(const char *fmt, ...) {
            va_list sprintf_args;
            va_start(sprintf_args, fmt);
            vPrintf((char*) fmt, sprintf_args);
            va_end(sprintf_args);
        }

        /**Write a raw value to the stream
         * WARNING: DO NOT USE THIS FOR TYPES THAT ARE NOT PORTABLE AND SEND THEM ACROSS THE NETWORK
         * USE WriteStd at the minimum**/
        template<typename T> inline void Write(T& t){
            WriteBytes((uint8_t*) &t, sizeof(T));
        }

        /**Write a raw value to the stream
         * WARNING: DO NOT USE THIS FOR TYPES THAT ARE NOT PORTABLE AND SEND THEM ACROSS THE NETWORK
         * USE WriteStd at the minimum**/
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type Write(std::tuple<Tp...>& t){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        Write(std::tuple<Tp...>& t){
            Write(std::get<I>(t));
            Write<I+1, Tp...>(t);
        }

        /**Write a raw value to the stream
         * WARNING: DO NOT USE THIS FOR TYPES THAT ARE NOT PORTABLE AND SEND THEM ACROSS THE NETWORK
         * USE WriteStd at the minimum**/
        template<typename... TArgs> inline void Write(TArgs... args){
            auto t = tuple<TArgs...>(args...);
            Write(t);
        }

        /**Write a standardized value to the stream. Use this for integral types etc to send information to different devices**/
        template<typename T, bool RequiresByteSwap> void WriteStd(T* v){
            if(RequiresByteSwap){
                auto ptr = (uint8_t *) v;
                std::reverse(ptr, ptr + sizeof(T));
            }
            WriteBytes((uint8_t*) v, sizeof(T));
        }

        /**Write a standardized value to the stream. Use this for integral types etc to send information to different devices**/
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

        /**Write a standardized value to the stream. Use this for integral types etc to send information to different devices**/
        template<typename T, int S> void WriteStd(std::array<T, S>& a){
            for(int i = 0; i < S; i++)
                WriteStd(a[i]);
        }

        /**Write a standardized value to the stream. Use this for integral types etc to send information to different devices**/
        template<typename T> void WriteStd(std::vector<T>& a){
            WriteStd((uint32_t) a.size());
            for(int i = 0; i < a.size(); i++)
                WriteStd(a[i]);
        }

        /**Write a standardized value to the stream. Use this for integral types etc to send information to different devices**/
        template<typename ...TArgs> void WriteStd(TArgs... args){
            WriteStd(tuple<TArgs...>(args...));
        }

        /**Read a c string from the buffer (null terminated) and return the chars read **/
        int ReadUnsafeString(SeekableIO& buffer) { return ReadStringUntilChars(buffer, false, '\0'); }

        /**Read a line from a buffer and return the chars read **/
        int ReadLine(SeekableIO& buffer) { return ReadStringUntilChars(buffer, true, '\n', '\r'); }

        /**Read a buffer until certain stop chars. Greedy means keep reading until a non stop char is found after stop char **/
        template<typename... Chars> int ReadStringUntilChars(SeekableIO& buffer, bool greedy, Chars... stop_chars);

        /**Read a raw value from the stream (count)
        * WARNING: DO NOT USE THIS FOR TYPES THAT ARE NOT PORTABLE AND SEND THEM ACROSS THE NETWORK
        * USE WriteStd at the minimum**/
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

        /**Write a raw value to the stream
        * WARNING: DO NOT USE THIS FOR TYPES THAT ARE NOT PORTABLE AND SEND THEM ACROSS THE NETWORK
        * USE WriteStd at the minimum**/
        template<typename T>
        T Read() {
            T t;
            Read(&t, 1);
            return t;
        }

        /**Read an array from this stream to $b**/
        template<typename T> int ReadArray(int* length, IOVector& b);

        /**Read an string from this stream to $b**/
        int ReadString(int* length, IOVector& b);

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
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

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
        template<typename T> void ReadStd(T* v, int count){
            for(int i = 0; i < count; i++)
                ReadStd<T>(v + i);
        }

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
        template<typename T, bool RequiresByteSwap> void ReadStd(T* v){
            if(RequiresByteSwap){
                auto ptr = (uint8_t *) v;
                std::reverse(ptr, ptr + sizeof(T));
            }
        }

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
        template<typename T>
        T ReadStd() {
            T t;
            ReadStd(&t);
            return t;
        }

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I == sizeof...(Tp), void>::type ReadStd(std::tuple<Tp...>* t){ }
        template<std::size_t I = 0, typename... Tp> inline typename std::enable_if<I < sizeof...(Tp), void>::type
        ReadStd(std::tuple<Tp...>* t){
            ReadStd(&std::get<I>(*t));
            ReadStd<I+1, Tp...>(t);
        }

        /**Read a standardized array from the stream. Use this for integral types etc to send information to different devices**/
        template<typename T, int S> void ReadStd(std::array<T, S>* a){
            for(int i = 0; i < S; i++)
                ReadStd<T>(&(*a)[i]);
        }

        /**Read a standardized vector from the stream. Use this for integral types etc to send information to different devices**/
        template<typename T> void ReadStd(std::vector<T>* a){
            auto s = ReadStd<uint32_t>();
            a->resize(s);
            for(int i = 0; i < s; i++)
                ReadStd<T>(&(*a)[i]);
        }

        /**Read a standardized value from the stream. Use this for integral types etc to send information to different devices**/
        template<typename ...TArgs> void ReadStd(Lambda<void (TArgs...)>& lam){
            apply(lam, ReadStd<tuple<TArgs...>>());
        }

        /**Try to read a value from IO. Return if it could or not **/
        template<typename T> inline bool TryRead(T* t, int count = 1){
            if(BytesAvailable() >= sizeof(T) * count){
                Read<T>(t, count);
                return true;
            }else return false;
        }

        /**Try to read a std value from IO. Return if it could or not **/
        template<typename T> inline bool TryReadStd(T* t, int count = 1){
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

    /**Simple Implementation of a in memory buffer from the IO
   **/
    struct SeekableIO : public IO{
        virtual size_t Position() = 0;
        virtual size_t Size() = 0;

        virtual void Seek(size_t position) = 0;
        void SeekDelta(size_t delta){ Seek(Position() + delta); }
        void SeekStart(){ Seek(0); }
        void SeekEnd(){ Seek(Size()); }
        int BytesAvailable() final { return Size() - Position(); }
    };

    class IOVector : public SeekableIO {
        size_t position = 0, max_size = 0;
        std::vector<uint8_t> memory;
    public:
        IOVector() : max_size(numeric_limits<long>::max()){}
        IOVector(int capacity, long max_size = numeric_limits<long>::max()) : memory(capacity), max_size(max_size) {}

        size_t Position() final { return position; }
        size_t Size() final { return memory.size(); }
        void Seek(size_t pos) final { position = pos; }

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

        inline size_t Capacity() { return memory.capacity(); }
        inline void SetSize(size_t s){ memory.resize(s); }
        inline void Reserve(size_t s) { memory.reserve(s); }
        void Print(IO& io){
            io.WriteByte('[');
            for(int i = position; i < memory.size(); i++){
                if(i != position)
                    io.Printf(", %i", memory[i]);
                else io.Printf("%i", memory[i]);
            }
            io.Printf(": Size=%i, Position=%i, Capacity=%i]\n\r", Size(), Position(), Capacity());
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

        /**Read the raw memory of the io at the current position**/
        template<typename T = uint8_t> T* Interpret(size_t pos){ return (T*) &memory[pos]; }

        /**Read the raw memory of the io at the specified position **/
        template<typename T = uint8_t> T* Interpret(){ return (T*) &memory[position]; }

        inline int ReadFrom(SeekableIO& b){return ReadFrom(b, b.BytesAvailable()); }
        inline int ReadFrom(IO& io, int bytes){ return IO::ReadFrom(io, bytes); }
        inline int ReadFrom(IO& io){ return IO::ReadFrom(io); }
        int WriteTo(IO& io){ return WriteTo(io, BytesAvailable()); }
        int WriteTo(IO& io, int count){ return io.WriteBytes(Interpret<uint8_t>(), min(count, BytesAvailable())); }
        inline void Reset(){ position = 0; }
        void RemoveRange(size_t start, size_t end){ memory.erase(memory.begin() + start, memory.begin() + end); }
        void InsertRange(size_t start, int length){ memory.insert(memory.begin() + start, length, 0); }
    };

    struct IOArray : public SeekableIO{
    private:
        ref<uint8_t> memory;
        size_t position, size, capacity;

        void WriteSize(int length){
            auto pl = position + length;
            if(pl > size)
                size = pl;
        }

    public:
        void Seek(size_t pos) final { position = pos; }

        size_t Position() final { return position; }
        uint8_t* Begin() { return memory.get() + position; }
        uint8_t* End() { return memory.get() + size; }
        size_t Size() final { return size; }
        inline size_t Capacity() const { return capacity; }

        explicit IOArray(int capacity = BUFSIZ) : memory(new uint8_t[capacity]), capacity(capacity), size(0), position(0){}
        IOArray(ref<uint8_t> heap_ref, int capacity, int size = 0) : memory(std::move(heap_ref)), capacity(capacity), size(size), position(0){}

        int WriteByte(uint8_t c) {
            if(capacity > position){
                WriteSize(1);
                memory.get()[position++] = c;
                return true;
            }
            return false;
        }

        int WriteBytes(uint8_t *ptr, int nbytes) override{
            if(nbytes == 1)
                return WriteByte(*ptr);
            if(nbytes > 1 && position + nbytes <= capacity){
                memmove(Begin(), ptr, nbytes);
                WriteSize(nbytes);
                position += nbytes;
                return true;
            }
            return false;
        }

        int ReadByte() { return memory.get()[position++]; }

        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) override{
            auto ba = BytesAvailable();

            if(ba > 0){
                if(buffer_size == 1){
                    *ptr = ReadByte();
                    return 1;
                }else{
                    auto read_bytes = min(buffer_size, ba);
                    memmove(ptr, Begin(), read_bytes);
                    position += read_bytes;
                    return read_bytes;
                }
            }

            return 0;
        }

        void Reserve(size_t s) {
            if(s > capacity){
                auto p = new uint8_t[s];
                memcpy(p, memory.get(), size);
                memory.reset(p, RefDeleter<uint8_t>(true));
                capacity = s;
            }
        }

        void SetBytesAvailable(size_t s, bool adjMemory = false){
            SetSize(s + position, adjMemory);
        }

        void SetSize(size_t s, bool adjMemory = false){
            if(adjMemory && s > capacity){
                Reserve(s);
            }
            size = s;
        }

        /**Read the raw memory of the io at the current position**/
        template<typename T = uint8_t> T* Interpret(size_t pos){ return (T*) &memory.get()[pos]; }
        /**Read the raw memory of the io at the specified position **/
        template<typename T = uint8_t> T* Interpret(){ return (T*) &memory.get()[position]; }

        void Clear(){
            position = 0;
            size = 0;
        }

        void ClearToPosition(){
            if(position != 0){
                RemoveRange(0, position);
                position = 0;
            }
        }

        void Print(IO& io){
            io.WriteByte('[');
            for(auto i = position; i < size; i++){
                if(i != position)
                    io.Printf(", %i", memory.get()[i]);
                else io.Printf("%i", memory.get()[i]);
            }
            io.Printf(": Size=%i, Position=%i, Capacity=%i]\n\r", Size(), Position(), Capacity());
        }

        inline int ReadFrom(IOVector& b){return ReadFrom(b, b.BytesAvailable()); }
        inline int ReadFrom(IO& io, int bytes){ return IO::ReadFrom(io, bytes); }
        inline int ReadFrom(IO& io){ return IO::ReadFrom(io); }
        int WriteTo(IO& io){ return WriteTo(io, BytesAvailable()); }
        int WriteTo(IO& io, int count){ return io.WriteBytes(Interpret<uint8_t>(), min(count, BytesAvailable())); }
        void RemoveRange(size_t start, int length){
            start = max((size_t) 0, start);
            auto end = start + length;

            memmove(memory.get() + start, memory.get() + end, size - end);        //Move the end to this location

            size -= length;
        }

        void InsertRange(size_t start, int length){
            start = max((size_t) 0, start);
            auto end = start + length;

            memmove(memory.get() + end, memory.get() + start, size - start);         //Move the end to this location

            size += length;
        }
    };

    template<typename... Chars>
    int IO::ReadStringUntilChars(Simple::SeekableIO& buffer, bool greedy, Chars ...stop_chars) {
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
    int IO::ReadArray(int *length, Simple::IOVector &b){
        auto pos = b.Position();
        *length = ReadStd<uint32_t>();
        WriteTo(b, *length * sizeof(T));
        return pos;
    }

    int IO::ReadString(int *length, Simple::IOVector &b){
        auto pos = ReadArray<char>(length, b);
        *length = *length + 1;
        b.WriteByte('\0');
        return pos;
    }

    /**Implementation of the IO to a FILE***/
    struct FileIO : public IO{
        FILE* out, *in;

        FileIO(FILE* out, FILE* in) : out(out), in(in){}
        int WriteByte(uint8_t c){ return putc(c, out) == EOF ? 0 : 1; }
        int WriteBytes(uint8_t *ptr, int nbytes) final { return fwrite(ptr, nbytes, 1, out); }
        int ReadByte() { return getc(in); }
        int ReadBytesUnlocked(uint8_t *ptr, int buffer_size) final { return fread(ptr, 1, buffer_size, in); }
        int BytesAvailable() final { return feof(out) ? 1 : 0; }
    };


    /**Check the endianness of the system to see if it is correct or not**/
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