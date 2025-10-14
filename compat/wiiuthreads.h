#ifndef COMPAT_WIIUTHREADS_H
#define COMPAT_WIIUTHREADS_H

/**
 * @file Wrapper from Wii U threads to pthreads
 */

#include <coreinit/thread.h>
#include <coreinit/mutex.h>
#include <coreinit/atomic.h>

// I have no idea why we have to typedef this
// But ok
typedef struct {
    TID tid;
    void *(*start_routine)(void *);
    void *arg;
    void *result;
} pthread_t;

typedef OSThreadAttributes pthread_attr_t;

// There's something called a fast mutex
// "Similar to OSMutex but tries to acquire the mutex 
// without using the global scheduler lock, 
// and does not test for thread cancel"
// Use this? idk
typedef OSMutex pthread_mutex_t;

// docs say something about OScondition
// using a thread queue
// need to figure out what's up with that
typedef OSCondition pthread_cond_t;
typedef void pthread_condattr_t;


// what the heck is once lmao
// just gonna yoink this from os2threads.h
typedef struct {
    volatile int done;
    OSThread mtx;
} pthread_once_t;


#endif