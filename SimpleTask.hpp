/**********************************************************************
   NAME: SimpleMemory.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Task
		Async coding and multitasking
*********************************************************************/

#ifndef SIMPLE_EVENT_C_H
#define SIMPLE_EVENT_C_H

#include <vector>
#include <stdint.h>
#include "SimpleLambda.hpp"

using namespace std;

namespace Simple{
    enum TaskReturn : uint8_t{
        Nothing = 0,
        Disposed = 1
    };


    /**Represents a piece of code that will be executed when yielded**/
    struct Task{
    private:
        static vector<Task*> tasks;
        volatile int ID = -1;
    public:
        virtual ~Task(){ Stop(); }

        inline bool Active(){ return ID != -1; }

        /**Fire the code**/
        virtual TaskReturn Fire() = 0;

        /**Allow the task to be executed**/
        virtual void Start(){
            if(!Active()){
                ID = tasks.size();
                tasks.push_back(this);
            }
        }

        /**Stop the task from being executed**/
        virtual void Stop(){
            if(Active()){
                tasks.erase(tasks.begin() + ID);
                ID = -1;
            }
        }

        static bool CanYield(){ return tasks.size() > 0; }
        static void Yield(Task* t){ t->Fire(); }

        /**Run all available tasks**/
        static void Yield() {
            for(int i = 0; i < tasks.size(); i++){
                if(tasks[i]->Fire() != TaskReturn::Nothing){
                    i--;
                }
            }
        }

        /**Wait for x seconds. In the meantime run background tasks**/
        static void Wait(uint32_t milliseconds);
    };

    vector<Task*> Task::tasks;

    /**Wait for x seconds. In the meantime run background tasks**/
    inline void Wait(uint32_t milliseconds){ Task::Wait(milliseconds); }

    /**Run all available tasks**/
    inline bool Yield(){
        Task::Yield();
        return Task::CanYield();
    }

    /**A task that can be optionally repeated. It will self dispose when it cannot repeat**/
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

    /**Wrapper over a lambda to provide async code**/
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

    /**Run a lambda asynchronously**/
    AsyncTask* Async(Lambda<void()> callback){
        auto task = new AsyncTask(std::move(callback));
        task->Start();
        return task;
    }

/**Run a lambda asynchronously**/
#define async(capture, ...)                                                             \
        make_local_lambda(CAT(__async, __LINE__), capture, void, (), __VA_ARGS__);    \
        Async(CAT(__async, __LINE__))
}

#endif