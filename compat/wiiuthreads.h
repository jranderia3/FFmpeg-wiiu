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
// Use that instead? Idk so I'll use this for now,
// as it seems safer
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

#define ONCE_VALUE_INIT 0
#define ONCE_VALUE_STARTED 1
#define ONCE_VALUE_DONE 2

static int pthread_create(pthread_t *thread, const pthread_mutex_t *attr,
                          void *(*start_routine)(void*), void *arg)
{
   OSThread *thread = (OSThread *)memalign(16, sizeof(OSThread));
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
                       (OSThreadEntryPointFn)entryPoint,
                       (int)entryArgs,
                       NULL,
                       stack + __wut_thread_default_stack_size,
                       __wut_thread_default_stack_size,
                       16,
                       OS_THREAD_ATTRIB_AFFINITY_ANY)) {
      free(thread);
      free(stack);
      return EINVAL;
   }

   *outThread = thread;
   OSSetThreadDeallocator(thread, &__wut_thread_deallocator);
   OSSetThreadCleanupCallback(thread, &__wut_thread_cleanup);

   // Set a thread run quantum to 1 millisecond, to force the threads to
   // behave more like pre-emptive scheduling rather than co-operative.
   OSSetThreadRunQuantum(thread, 1000);

   OSResumeThread(thread);
   return 0;
}

static int pthread_join(pthread_t thread, void **value_ptr)
{
    if (!OSJoinThread(*thread, (int *)*retval)) {
        return EINVAL;
    }
    return 0;
}

static int pthread_mutex_init(pthread_mutex_t *mutex,
                              const pthread_mutexattr_t *attr)
{
    return OSInitMutex(mutex);
}

static int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    // no function to destroy mutex on cafe os
    // https://wut.devkitpro.org/mutex_8h.html
    // so just return 0 lmao
    return 0;
}

static int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    OSLockMutex(mutex);
    return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    OSUnlockMutex(mutex);
    return 0;
}

static int pthread_cond_init(pthread_cond_t *cond,
                             const pthread_condattr_t *attr)
{
    OSInitCond(cond);
}

static int pthread_cond_destroy(pthread_cond_t *cond)
{
    // no function to destroy cond on cafe os
    // https://wut.devkitpro.org/group__coreinit__cond.html
    // so just return 0 lmao
    return 0;
}

static int pthread_cond_signal(pthread_cond_t *cond)
{
    OSSignalCond(cond);
    return 0;
}

static int pthread_cond_broadcast(pthread_cond_t *cond)
{
    // OSSignalCond is already a broadcast:
    // OSSignalCond: 
    // Will wake up any threads waiting on the condition with OSWaitCond. 
    OSSignalCond(cond);
    return 0;
}

static int pthread_cond_timedwait(pthread_cond_t *cond, 
                                  pthread_mutex_t *mutex,
                                  const struct timespec *abstime)
{

}

static int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    OSWaitCond(cond, mutex);
    return 0;
}

static int pthread_once(pthread_once_t *once_control,
                        void (*init_routine)(void))
{
    
}

#endif