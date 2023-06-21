#ifndef SIMPLE_TIMER_C_H
#define SIMPLE_TIMER_C_H

#include "SimpleTask.h"
#include "SimpleLambda.h"

namespace Simple {
    static time_t NativeMillis();

    struct TimerController;
    template<typename T = uint32_t>
    struct TimeKeeper;

    template<typename T = uint32_t>
    class TimeDecay {
        friend TimeKeeper<T>;
        T value;
        bool sign;
    public:
        TimeDecay() {}

        void shift(typename TimeKeeper<T>::Time t) { value += t; }
    };

    template<typename T>
    class TimeKeeper {
    public:
        using Time = typename make_signed<T>::type;
    private:
        friend TimeDecay<T>;
        time_t ClockOffset;
        Time ClockDelta;
        bool currentSign = false;
    public:
        TimeKeeper() : ClockOffset(NativeMillis()), ClockDelta(0) {}

        Time Millis() {
            auto now = NativeMillis();
            auto delta = static_cast<Time>(now - ClockOffset);
            if(delta >= numeric_limits<Time>::max()){
                ClockDelta = -delta;
                ClockOffset = now;
                currentSign = !currentSign; //Set Dirty
            }
            return delta;
        }

        Time getDelta(TimeDecay<T> &t) {
            auto now = Millis();
            if (t.sign != currentSign) {
                t.value += ClockDelta;
                t.sign = currentSign;
            }
            return static_cast<T>(now - t.value);
        }

        bool hasDecayed(TimeDecay<T> &t) { return getDelta(t) >= 0; }

        TimeDecay<T> setDecay(Time decay) {
            TimeDecay<T> d;
            d.value = decay + Millis();
            d.sign = currentSign;
            return d;
        }
    };

    TimeKeeper<> Time;

    class Timer : public RepeatableTask {
        using TimeT = TimeKeeper<>::Time;
    public:
        Lambda<void(Timer &)> callback;
        TimeDecay<> decay;
        TimeT length;

        Timer() {}

        Timer(bool repeat, TimeT length) : RepeatableTask(repeat), length(length) {}

        Timer(bool repeat, TimeT length, Lambda<void(Timer &)> &callback) : RepeatableTask(repeat), length(length),
                                                                            callback(callback) {}

        void Start() override {
            decay = Time.setDecay(length);
            Task::Start();
        }

        TaskReturn Fire() override {
            if (Time.hasDecayed(decay))
                return FireTimerNow();
            return Nothing;
        }

        TaskReturn FireTimerNow() {
            callback(*this);
            if (Repeat)
                decay = Time.setDecay(length);
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