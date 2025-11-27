/**
 * @file Wrapper from Wii U threads to pthreads
 */

#ifndef COMPAT_WIIUTHREADS_H
#define COMPAT_WIIUTHREADS_H


// #define __PTHREAD_h
// #define _SYS__PTHREADTYPES_H_

// typedef unsigned int pthread_t;
// typedef unsigned int pthread_mutex_t;

#include <coreinit/thread.h>
#include <coreinit/mutex.h>
#include <coreinit/atomic.h>
#include <coreinit/condition.h>

// #include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <coreinit/alarm.h>
#include <coreinit/systeminfo.h>
#include <coreinit/time.h>

#include "libavutil/attributes.h"
#include "libavutil/common.h"
#include "libavutil/time.h"



// enum from wut_thread_specific.h
typedef enum __wut_thread_specific_id
{
   WUT_THREAD_SPECIFIC_0 = 0,
   WUT_THREAD_SPECIFIC_1 = 1,
} __wut_thread_specific_id;

// a bunch of defines from wut_gthread by GaryOderNichts
#define __WUT_MAX_KEYS               (128)
#define __WUT_STACK_SIZE             (128 * 1024)

#define __WUT_ONCE_VALUE_INIT        (0)
#define __WUT_ONCE_VALUE_STARTED     (1)
#define __WUT_ONCE_VALUE_DONE        (2)
#define PTHREAD_ONCE_INIT __WUT_ONCE_VALUE_INIT

#define __WUT_KEY_THREAD_SPECIFIC_ID WUT_THREAD_SPECIFIC_0

typedef volatile uint32_t __wut_once_t;
typedef struct
{
   uint32_t index;
} __wut_key_t;

// from wut_clock.h
// The Wii U OSTime epoch is at 2000, so we must map it to 1970 for gettime
#define WIIU_OSTIME_EPOCH_YEAR         (2000)
// The Wii U FSTime epoch is at 1980, so we must map it to 1970 for gettime
#define WIIU_FSTIME_EPOCH_YEAR         (1980)

#define EPOCH_YEAR                     (1970)
#define EPOCH_YEARS_SINCE_LEAP         2
#define EPOCH_YEARS_SINCE_CENTURY      70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define EPOCH_DIFF_YEARS(year)         (year - EPOCH_YEAR)
#define EPOCH_DIFF_DAYS(year)                                        \
   ((EPOCH_DIFF_YEARS(year) * 365) +                                 \
    (EPOCH_DIFF_YEARS(year) - 1 + EPOCH_YEARS_SINCE_LEAP) / 4 -      \
    (EPOCH_DIFF_YEARS(year) - 1 + EPOCH_YEARS_SINCE_CENTURY) / 100 + \
    (EPOCH_DIFF_YEARS(year) - 1 + EPOCH_YEARS_SINCE_LEAP_CENTURY) / 400)
#define EPOCH_DIFF_SECS(year) (60ull * 60ull * 24ull * (uint64_t)EPOCH_DIFF_DAYS(year))



// I have no idea why we have to typedef this
// But ok
// typedef struct {
//     TID tid;
//     void *(*start_routine)(void *);
//     void *arg;
//     void *result;
// } pthread_t;

// screw the above what if i just do this instead
// UPDATE: ok Ash the wii u god says to not do this so commented out 
// She instructed me to use the system type (uint32_t) and just cast in and out of it
// That is to say, treat pthread_t as OSThread*
// typedef OSThread pthread_t;



// OK apparently typedefing pthread is REALLY bad
// and causes about 99999999 errors 
// so basically since pthread_types are basically
// all uint32_t what if I just use them all as pointers
// cause the wii u is a 32 bit system

// to
// typedef uint32_t pthread_attr_t;

// There's something called a fast mutex
// "Similar to OSMutex but tries to acquire the mutex 
// without using the global scheduler lock, 
// and does not test for thread cancel"
// Use that instead? Idk so I'll use this for now,
// as it seems safer

// typedef struct {
//     OSMutex mutex;
//     BOOL initialized;
// } pthread_mutex_t;
// typedef void pthread_mutexattr_t;


#define OSMUTEX_UNINITIALIZED (pthread_mutex_t) 0x00000000
#define PTHREAD_MUTEX_INITIALIZER OSMUTEX_UNINITIALIZED

#define OSCONDITION_UNINITIALIZED (OSCondition) {0}
#define PTHREAD_COND_INITIALIZER OSCONDITION_UNINITIALIZED

// docs say something about OScondition
// using a thread queue
// need to figure out what's up with that
// typedef OSCondition pthread_cond_t;
// typedef void pthread_condattr_t;


// typedef __wut_once_t pthread_once_t;

uint32_t __attribute__((weak)) __wut_thread_default_stack_size = __WUT_STACK_SIZE;

// static inline int pthread_kill(pthread_t thread, int sig);

#define ONCE_VALUE_INIT 0
#define ONCE_VALUE_STARTED 1
#define ONCE_VALUE_DONE 2

// stuff for timed wait
// taken from wii u toolkit
struct __wut_cond_timedwait_data_t
{
    OSCondition *cond;
    bool timed_out;
};

static void __wut_thread_deallocator(OSThread *thread,
                                     void *stack)
{
   free(thread);
   free(stack);
}


static av_unused int pthread_create(pthread_t* out_thread, const pthread_attr_t *attr,
                          void *(*start_routine)(void*), void *arg)
{
    OSThread* thread = (OSThread *)memalign(16, sizeof(OSThread));
    if (!thread) {
        return ENOMEM;
    }

    memset(thread, 0, sizeof(OSThread));

    char *stack = (char *)memalign(16, __wut_thread_default_stack_size);
    if (!stack) {
        free(thread);
        return ENOMEM;
    }

    if (!OSCreateThread(thread,
                        (OSThreadEntryPointFn)start_routine,
                        (int)arg,
                        NULL,
                        stack + __wut_thread_default_stack_size,
                        __wut_thread_default_stack_size,
                        16,
                        OS_THREAD_ATTRIB_AFFINITY_ANY)) {
        free(thread);
        free(stack);
        return EINVAL;
    }

    *out_thread = (uint32_t) thread;
    OSSetThreadDeallocator(thread, &__wut_thread_deallocator);
    // OSSetThreadCleanupCallback(thread, &__wut_thread_cleanup);

    // Set a thread run quantum to 1 millisecond, to force the threads to
    // behave more like pre-emptive scheduling rather than co-operative.
    OSSetThreadRunQuantum(thread, 1000);

    OSResumeThread(thread);
    return 0;
}

static av_unused int pthread_join(pthread_t thread, void **value_ptr)
{
    if (!OSJoinThread((OSThread*) thread, (int *)value_ptr)) {
        return EINVAL;
    }
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *out_mutex,
                              const pthread_mutexattr_t *attr)
{
    OSMutex* mutex = (OSMutex *)memalign(16, sizeof(OSMutex));
    if (!mutex) {
        return ENOMEM;
    }
    OSInitMutex(mutex);
    *out_mutex = (uint32_t) mutex;
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    // no function to destroy mutex on cafe os
    // https://wut.devkitpro.org/mutex_8h.html
    // so just return 0 lmao
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    // a mutex is not guaranteed to be initialized
    // since some code will just say 
    // pthread_mutex_t foo = PTHREAD_MUTEX_INITIALIZER;
    // check for this, and if so, initialize mutex
    if (*mutex == OSMUTEX_UNINITIALIZED) {
        pthread_mutex_init(mutex, 0);
    }
    OSLockMutex((OSMutex*) *mutex);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    // a mutex is not guaranteed to be initialized
    // since some code will just say 
    // pthread_mutex_t foo = PTHREAD_MUTEX_INITIALIZER;
    // check for this, and if so, initialize mutex
    if (*mutex == OSMUTEX_UNINITIALIZED) {
        pthread_mutex_init(mutex, 0);
    }
    OSUnlockMutex((OSMutex*) *mutex);
    return 0;
}

static inline int pthread_cond_init(pthread_cond_t *cond,
                             const pthread_condattr_t *attr)
{
    OSCondition* condition = (OSCondition*) memalign(16, sizeof(OSCondition));
    if(!condition) {
        return ENOMEM;
    }
    OSInitCond(condition);
    *cond = (uint32_t) condition;
    // OSInitCond doesn't return so just hope it's successful
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *cond)
{
    // no function to destroy cond on cafe os
    // https://wut.devkitpro.org/group__coreinit__cond.html
    // so just return 0 lmao
    return 0;
}

static inline int pthread_cond_signal(pthread_cond_t *cond)
{
    OSSignalCond((OSCondition*)(*cond));
    return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t *cond)
{
    // OSSignalCond is already a broadcast as shown from docs:

    // "OSSignalCond: 
    // Will wake up any threads waiting on the condition with OSWaitCond."
    
    OSSignalCond((OSCondition*)(*cond));
    return 0;
}

static void
__wut_cond_timedwait_alarm_callback(OSAlarm *alarm,
                                    OSContext *context)
{
    struct __wut_cond_timedwait_data_t *data = (struct __wut_cond_timedwait_data_t *)OSGetAlarmUserData(alarm);
    data->timed_out                   = true;
    OSSignalCond(data->cond);
}

static inline int pthread_cond_timedwait(pthread_cond_t *cond, 
                                  pthread_mutex_t *mutex,
                                  const struct timespec *abstime)
{
    struct __wut_cond_timedwait_data_t data;
    data.timed_out = false;
    data.cond      = (OSCondition*) *cond;

    OSTime time    = OSGetTime();
    OSTime timeout =
      OSSecondsToTicks(abstime->tv_sec - EPOCH_DIFF_SECS(WIIU_OSTIME_EPOCH_YEAR)) +
      OSNanosecondsToTicks(abstime->tv_nsec);
    if (timeout <= time) {
        return ETIMEDOUT;
    }

    // Set an alarm
    OSAlarm alarm;
    OSCreateAlarm(&alarm);
    OSSetAlarmUserData(&alarm, &data);
    OSSetAlarm(&alarm, timeout - time,
                &__wut_cond_timedwait_alarm_callback);
    // Wait on the condition
    if (*mutex == OSMUTEX_UNINITIALIZED) {
        pthread_mutex_init(mutex, 0);
    }
    OSWaitCond((OSCondition*) *cond, (OSMutex*) *mutex);

    OSCancelAlarm(&alarm);
    return data.timed_out ? ETIMEDOUT : 0;

}

static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    OSWaitCond((OSCondition*)*cond, (OSMutex*) *mutex);
    return 0;
}


// seems to implement some type of state machine
// basically the way this works is that
// if the function hasn't been run yet, run it 
// and change "value"
// if that chec determines that the function
// has already run, just spin until it's done
static av_unused int wut_thread_once(__wut_once_t *once_control,
                        void (*init_routine)(void))
{
    uint32_t value = 0;
    if (OSCompareAndSwapAtomicEx(once_control,
                                __WUT_ONCE_VALUE_INIT,
                                __WUT_ONCE_VALUE_STARTED,
                                &value)) {
        init_routine();
        OSSwapAtomic(once_control, __WUT_ONCE_VALUE_DONE);
    } else if (value != __WUT_ONCE_VALUE_DONE) {
        while (!OSCompareAndSwapAtomic(once_control,
                                        __WUT_ONCE_VALUE_DONE,
                                        __WUT_ONCE_VALUE_DONE));
    }
    return 0;
    
}
#endif