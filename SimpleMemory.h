#ifndef SIMPLE_MEMORY_H
#define SIMPLE_MEMORY_H

#include <stdint.h>
#include <memory>

namespace Simple{
    template<typename T>
    struct RefDeleter{
        bool shouldDelete = false;

        void operator()(T* p){
            if(shouldDelete){
                delete p;
                shouldDelete = false;
            }
        }
    };
    template<typename T> struct NoDelete{ inline void operator()(T* p){} };

    template<typename T = uint8_t> using ref = std::shared_ptr<T>;
    template<typename T> inline ref<T> LocalRef(T* t){ return ref<T>(t, NoDelete<T>()); }
    template<typename T> inline ref<T> HeapRef(T* t){ return ref<T>(new T(*t)); }
    template<typename T> inline ref<T> Ref(T* t, bool owns){ return ref<T>(t, RefDeleter<T>(owns)); }
}
#endif