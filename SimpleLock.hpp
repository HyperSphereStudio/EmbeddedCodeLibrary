/**********************************************************************
   NAME: SimpleLock.hpp
   AUTHOR: Johnathan Bizzano
   DATE: 6/22/2023

    The Simple Project
		Medium Level (from Low) library that abstracts away from embedded device hardware

    Simple Lock
		Provide Multithreading support
*********************************************************************/

#ifndef Simple_Lock_C_H
#define Simple_Lock_C_H

#include "SimpleCore.hpp"

typedef struct SimpleLock{
    volatile uint8_t lock;
} SimpleLock;

void SimpleLock_Init(SimpleLock* lock){
    lock->lock = false;
}

void SimpleLock_Lock(SimpleLock* lock){
    while(lock->lock);
    lock->lock = true;
}

void SimpleLock_Unlock(SimpleLock* lock){
    lock->lock = false;
}

bool SimpleLock_IsLocked(SimpleLock* lock){
    return lock->lock;
}

void SimpleLock_Destroy(SimpleLock* lock){
    lock->lock = false;
}

#define SimpleLockBlock(lock, ...){ \
        while(!(lock)->lock);        \
            (lock)->lock = true;     \
        __VA_ARGS__                  \
        (lock)->lock = false;        \
}


#endif