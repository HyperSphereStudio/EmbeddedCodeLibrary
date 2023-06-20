#ifndef SIMPLE_TIMER_C_H
#define SIMPLE_TIMER_C_H

#include "SimpleTask.h"
#include "SimpleLambda.h"

namespace Simple{
    struct TimerController;

    class Timer : public RepeatableTask{
    public:
        static time_t START_CLOCK;
        Lambda<void (Timer&)> callback;
        volatile uint32_t start = 0, length;

        Timer(){}
        Timer(bool repeat, uint32_t length) : RepeatableTask(repeat), length(length){}
        Timer(bool repeat, uint32_t length, Lambda<void (Timer&)>& callback) : RepeatableTask(repeat), length(length), callback(callback){}

        static time_t NativeMillis();
        static uint32_t Millis(){ return (uint32_t) (NativeMillis() - START_CLOCK); }

        void Start() override{
            start = Millis();
            Task::Start();
        }

        TaskReturn Fire() override{
            if(start + length <= Millis())
                return FireTimerNow();
            return Nothing;
        }

        TaskReturn FireTimerNow(){
            callback(*this);
            if(Repeat)
                start = Millis();
            return RepeatableTask::Fire();
        }
    };

    time_t Timer::START_CLOCK = NativeMillis();
    void Task::Wait(uint32_t milliseconds){
        auto start = Timer::NativeMillis();
        auto diff = 0;
        while(diff < milliseconds){
            Yield();
            diff = Timer::NativeMillis() - start;
        }
    }
}

#endif