#ifndef SANDBOX_SIMPLEPC_H
#define SANDBOX_SIMPLEPC_H

#include "../SimpleIO.h"
#include "../SimpleTimer.h"
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
