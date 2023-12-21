/**********************************************************************
   NAME: SimpleMemory.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Memory
		Provide abstract Ref support (local refs, global refs, static refs etc)
*********************************************************************/

#ifndef SIMPLE_MEMORY_H
#define SIMPLE_MEMORY_H

#include <stdint.h>
#include <memory>

namespace Simple{
    template<typename T>
    struct RefDeleter{
        bool shouldDelete = false;

        RefDeleter(bool shouldDelete = false) : shouldDelete(shouldDelete){}

        void operator()(T* p){
            if(shouldDelete){
                delete p;
                shouldDelete = false;
            }
        }
    };
    template<typename T> struct NoDelete{ inline void operator()(T* p){} };

    template<typename T = uint8_t> using ref = std::shared_ptr<T>;

    /**Create a local reference**/
    template<typename T> inline ref<T> LocalRef(T* t){ return ref<T>(t, NoDelete<T>()); }

    /**Create a heap reference**/
    template<typename T> inline ref<T> HeapRef(T* t){ return ref<T>(new T(*t)); }

    /**Create a reference with an owner**/
    template<typename T> inline ref<T> Ref(T* t, bool owns){ return ref<T>(t, RefDeleter<T>(owns)); }

    struct Empty{};
}
#endif