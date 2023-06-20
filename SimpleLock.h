#ifndef Simple_Lock_C_H
#define Simple_Lock_C_H

#include "SimpleCore.h"

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