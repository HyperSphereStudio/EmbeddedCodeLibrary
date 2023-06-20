#ifndef Simple_DEBUG_C_H
#define Simple_DEBUG_C_H

#include "SimpleIO.h"

#ifdef DEBUG
    #define assert(x, ...) if(!(x)) printerrln( __VA_ARGS__ );
    #define lazyassert(x, msg) assert(x, msg)
    #define debug(fmt, ...) print(fmt, __VA_ARGS__)
    #define debugln(fmt, ...) println(fmt, __VA_ARGS__)
#else
    #define assert(x, ...) (x)
    #define lazyassert(x, msg)
    #define debug(fmt, ...)
    #define debugln(fmt, ...)
#endif

#endif