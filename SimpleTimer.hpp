/**********************************************************************
   NAME: SimpleTimer.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Timer
		Provide Timer Support
*********************************************************************/

#ifndef SIMPLE_TIMER_C_H
#define SIMPLE_TIMER_C_H

#include "SimpleTask.hpp"
#include "SimpleLambda.hpp"

namespace Simple {
    static time_t NativeMillis();

    struct TimerController;
    template<typename T = uint32_t>
    struct Time;

    /**A value that can be polled for a certain time in the future. It will decay until a certain point at which point the user can be notified**/
    template<typename T = uint32_t>
    class TimeDecay {
        friend Time<T>;
        T value;
        uint8_t cycled : 1;
        uint8_t decayed : 1;
    public:
        TimeDecay() {}
        inline void setDecay(Time<T> &t, T decay){ *this = t.createDecay(decay); }
        inline T getDelta(Time<T> &t, bool& sign){ return t.getDelta(*this, sign); }
        inline bool hasDecayed(Time<T> &t){ return t.hasDecayed(*this); }
        inline T Value() { return value; }
        inline bool hasCycled() { return cycled; }
    };

    /**A Simple Time Keeper**/
    template<typename T>
    class Time {
    private:
        friend TimeDecay<T>;
        bool cycleParity;
    public:
        Time() : cycleParity(false) {}
        T Millis()  { return static_cast<T>(NativeMillis()); }
        TimeDecay<T> createDecay(T decay) {
            TimeDecay<T> t;
            auto now = Millis();
            t.value = decay + now;
                                                    //Overflow
            t.cycled = t.value >= decay ? true : false;
            t.decayed = false;
            return t;
        }
        bool hasDecayed(TimeDecay<T> &t){
            if(t.decayed)
                return true;
            bool sign;
            getDelta(t, sign);
            return sign;
        }
        inline T getDelta(TimeDecay<T> &t, bool& sign){
            auto now = Millis();
            if(t.cycled || t.value >= now){
                t.cycled = true;
                if(t.value <= now){
                    sign = true;
                    t.decayed = true;
                    return now - t.value;
                }else{
                    sign = false;
                    return t.value - now;
                }
            }else{
               sign = false;
               return t.value - now;
            }
        }
    };

    /**Default Clock**/
    Time<> Clock;

    /**Simple Timer Implementation
     **/
    class Timer : public RepeatableTask {
        using TimeT = uint32_t;
    public:
        Lambda<void(Timer &)> callback;
        TimeDecay<> decay;
        TimeT length;

        Timer() {}

        Timer(bool repeat, TimeT length) : RepeatableTask(repeat), length(length) {}

        Timer(bool repeat, TimeT length, Lambda<void(Timer &)> &callback) : RepeatableTask(repeat), length(length),
                                                                            callback(callback) {}

        void Start() override {
            Reset();
            Task::Start();
        }

        /**Reset the internal clock to fire it in the future**/
        void Reset(){
            decay = Clock.createDecay(length);
        }

        TaskReturn Fire() override {
            if (Clock.hasDecayed(decay))
                return FireTimerNow();
            return Nothing;
        }

        TaskReturn FireTimerNow() {
            callback(*this);
            if (Repeat)
                decay = Clock.createDecay(length);
            return RepeatableTask::Fire();
        }
    };

    void Task::Wait(uint32_t milliseconds) {
        auto start = NativeMillis();
        auto diff = 0;
        while (diff < milliseconds) {
            Yield();
            diff = NativeMillis() - start;
        }
    }
}

#endif