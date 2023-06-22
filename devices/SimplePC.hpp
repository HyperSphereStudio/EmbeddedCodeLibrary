/**********************************************************************
   NAME: SimplePC.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple PC
        PC implementation of the simple library
*********************************************************************/

#ifndef SANDBOX_SIMPLEPC_H
#define SANDBOX_SIMPLEPC_H

#include "../SimpleIO.hpp"
#include "../SimpleTimer.hpp"
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace Simple;

time_t Simple::NativeMillis(){
    return duration_cast<milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

FileIO Out(stdout, stdin);
FileIO Error(stderr, nullptr);



#endif //SANDBOX_SIMPLEPC_H
