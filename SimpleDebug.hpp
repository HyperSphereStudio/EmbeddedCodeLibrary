/**********************************************************************
   NAME: SimpleDebug.h
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Debug
		Provide Optional Debug Capabilities
*********************************************************************/

#ifndef Simple_DEBUG_C_H
#define Simple_DEBUG_C_H

#include "SimpleIO.hpp"

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