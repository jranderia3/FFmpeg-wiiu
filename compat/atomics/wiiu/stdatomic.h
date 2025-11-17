#ifndef COMPAT_ATOMICS_WIIU_STDATOMIC_H
#define COMPAT_ATOMICS_WIIU_STDATOMIC_H

#include <stddef.h>
#include <stdint.h>
#include <coreinit/atomic.h>

#define atomic_init(obj, value) \
do {                            \
    *(obj) = (value);           \
} while(0)

#define kill_dependency(y) ((void)0)

// do i also have to use oscoherencybarrier here?
#define atomic_thread_fence(order) \
    OSMemoryBarrier();

// No equivalent on wii u
#define atomic_signal_fence(order) \
    ((void)0)

// No equivalent on wii u
#define atomic_is_lock_free(obj) 0

typedef intptr_t atomic_flag;
typedef intptr_t atomic_bool;
typedef intptr_t atomic_char;
typedef intptr_t atomic_schar;
typedef intptr_t atomic_uchar;
typedef intptr_t atomic_short;
typedef intptr_t atomic_ushort;
typedef intptr_t atomic_int;
typedef intptr_t atomic_uint;
typedef intptr_t atomic_long;
typedef intptr_t atomic_ulong;
typedef intptr_t atomic_llong;
typedef intptr_t atomic_ullong;
typedef intptr_t atomic_wchar_t;
typedef intptr_t atomic_int_least8_t;
typedef intptr_t atomic_uint_least8_t;
typedef intptr_t atomic_int_least16_t;
typedef intptr_t atomic_uint_least16_t;
typedef intptr_t atomic_int_least32_t;
typedef intptr_t atomic_uint_least32_t;
typedef intptr_t atomic_int_least64_t;
typedef intptr_t atomic_uint_least64_t;
typedef intptr_t atomic_int_fast8_t;
typedef intptr_t atomic_uint_fast8_t;
typedef intptr_t atomic_int_fast16_t;
typedef intptr_t atomic_uint_fast16_t;
typedef intptr_t atomic_int_fast32_t;
typedef intptr_t atomic_uint_fast32_t;
typedef intptr_t atomic_int_fast64_t;
typedef intptr_t atomic_uint_fast64_t;
typedef intptr_t atomic_intptr_t;
typedef intptr_t atomic_uintptr_t;
typedef intptr_t atomic_size_t;
typedef intptr_t atomic_ptrdiff_t;
typedef intptr_t atomic_intmax_t;
typedef intptr_t atomic_uintmax_t;

static inline void atomic_store(intptr_t *object, intptr_t desired)
{
    *object = desired;
    OSMemoryBarrier();
}

#define atomic_store_explicit(object, desired, order) \
    atomic_store(object, desired)

static inline intptr_t atomic_load(intptr_t *object)
{
    __machine_rw_barrier();
    return *object;
}

#define atomic_load_explicit(object, order) \
    atomic_load(object)

#define atomic_exchange(object, desired) \
    OSSwapAtomic(object, desired)

#define atomic_exchange_explicit(object, desired, order) \
    atomic_exchange(object, desired)


// Attempted to implement the behavior described at 
// https://pubs.opengroup.org/onlinepubs/9799919799/functions/atomic_compare_exchange_strong.html
// no shot this works lmfao
static inline int atomic_compare_exchange_strong(intptr_t *object, intptr_t *expected,
                                                 intptr_t desired)
{
    if (OSCompareAndSwapAtomic(object, *expected, desired)) {
        return TRUE;
    } else {
        *expected = *object;
        return FALSE;
    }
}

#define atomic_compare_exchange_strong_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_weak(object, expected, desired)

#define atomic_fetch_add(object, operand) \
    OSAddAtomic(object, operand)

#define atomic_fetch_sub(object, operand) \
    OSAddAtomic(object, -(operand))

#define atomic_fetch_or(object, operand) \
    OSOrAtomic(object, operand)

#define atomic_fetch_xor(object, operand) \
    OSXorAtomic(object, operand)

#define atomic_fetch_and(object, operand) \
    OSAndAtomic(object, operand)

#define atomic_fetch_add_explicit(object, operand, order) \
    atomic_fetch_add(object, operand)

#define atomic_fetch_sub_explicit(object, operand, order) \
    atomic_fetch_sub(object, operand)

#define atomic_fetch_or_explicit(object, operand, order) \
    atomic_fetch_or(object, operand)

#define atomic_fetch_xor_explicit(object, operand, order) \
    atomic_fetch_xor(object, operand)

#define atomic_fetch_and_explicit(object, operand, order) \
    atomic_fetch_and(object, operand)

#define atomic_flag_test_and_set(object) \
    OSTestAndSetAtomic(object, 0)

#define atomic_flag_test_and_set_explicit(object, order) \
    atomic_flag_test_and_set(object)

#define atomic_flag_clear(object) \
    OSTestAndClearAtomic(object, 0)

#define atomic_flag_clear_explicit(object, order) \
    atomic_flag_clear(object)

#endif