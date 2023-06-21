#ifndef SIMPLE_EVENT_C_H
#define SIMPLE_EVENT_C_H

#include <vector>
#include <stdint.h>
#include "SimpleLambda.h"

using namespace std;

namespace Simple{
    enum TaskReturn : uint8_t{
        Nothing = 0,
        Disposed = 1
    };

    struct Task{
    private:
        static vector<Task*> tasks;
        volatile int ID = -1;
    public:
        virtual ~Task(){ Stop(); }

        virtual TaskReturn Fire() = 0;

        virtual void Start(){
            if(ID == -1){
                ID = tasks.size();
                tasks.push_back(this);
            }
        }

        virtual void Stop(){
            if(ID != -1){
                tasks.erase(tasks.begin() + ID);
                ID = -1;
            }
        }

        static bool CanYield(){ return tasks.size() > 0; }
        static void Yield(Task* t){ t->Fire(); }

        static void Yield() {
            for(int i = 0; i < tasks.size(); i++){
                if(tasks[i]->Fire() != TaskReturn::Nothing){
                    i--;
                }
            }
        }
        static void Wait(uint32_t milliseconds);
    };

    vector<Task*> Task::tasks;

    inline void Wait(uint32_t milliseconds){ Task::Wait(milliseconds); }
    inline bool Yield(){
        Task::Yield();
        return Task::CanYield();
    }

    struct RepeatableTask : public Task{
        bool Repeat;

        RepeatableTask(bool repeat = false) : Repeat(repeat){}

        TaskReturn Fire() override{
            if(Repeat){
                return TaskReturn::Nothing;
            }else{
                Task::Stop();
                return TaskReturn::Disposed;
            }
        }
    };


    struct AsyncTask : public Task{
        Lambda<void()> callback;

        ~AsyncTask() override{ Task::Stop(); }
        AsyncTask(Lambda<void()> callback) : callback(std::move(callback)){}

        TaskReturn Fire() final{
            callback();
            Stop();
            return TaskReturn::Disposed;
        }

        void Stop() final{ delete this; }
    };

    AsyncTask* Async(Lambda<void()> callback){
        auto task = new AsyncTask(std::move(callback));
        task->Start();
        return task;
    }

#define async(capture, ...)                                                             \
        define_local_lambda(CAT(__async, __LINE__), capture, void, (), __VA_ARGS__);    \
        Async(CAT(__async, __LINE__))
}

#endif