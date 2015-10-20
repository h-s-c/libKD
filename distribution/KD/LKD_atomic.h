#ifndef __kd_LKD_atomic_h_
#define __kd_LKD_atomic_h_
#include <KD/kd.h>


/* Workarounds */
#if !defined(__has_feature)
#define __has_feature(x) 0
#endif
#if !defined(__has_include)
#define __has_include(x) 0
#endif

#if (__STDC_VERSION__ >= 201112L) && !defined (__STDC_NO_ATOMICS__) && __has_include(<stdatomic.h>)
#ifdef __ANDROID__
        typedef uint32_t char32_t;
        typedef uint16_t char16_t;
    #endif

    #include <stdatomic.h>
#elif defined (__clang__) && __has_feature(c_atomic)
#define memory_order_acquire                                    __ATOMIC_ACQUIRE
    #define memory_order_release                                    __ATOMIC_RELEASE
    #define ATOMIC_VAR_INIT(value)                                  (value)
    #define atomic_init(object, value)                              __c11_atomic_init(object, value)
    #define atomic_load(object)                                     __c11_atomic_load(object, __ATOMIC_SEQ_CST)
    #define atomic_fetch_add(object, value)                         __c11_atomic_fetch_add(object, value, __ATOMIC_SEQ_CST)
    #define atomic_fetch_sub(object, value)                         __c11_atomic_fetch_sub(object, value, __ATOMIC_SEQ_CST)
    #define atomic_store(object, desired)                           __c11_atomic_store(object, desired, __ATOMIC_SEQ_CST)
    #define atomic_compare_exchange_weak(object, expected, desired) __c11_atomic_compare_exchange_weak(object, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
    #define atomic_thread_fence(order)                              __c11_atomic_thread_fence(order)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (__STDC_VERSION__ >= 201112L) && !defined (__STDC_NO_ATOMICS__)
typedef struct KDAtomicInt { _Atomic KDint value; } KDAtomicInt;
typedef struct KDAtomicPtr { _Atomic void *value; } KDAtomicPtr;
#define kdAtomicFenceAcquire() atomic_thread_fence(memory_order_release)
#define kdAtomicFenceRelease() atomic_thread_fence(memory_order_release)
#elif defined(_MSC_VER) || defined(__MINGW32__)
typedef struct KDAtomicInt { KDint value; } KDAtomicInt;
typedef struct KDAtomicPtr { void *value; } KDAtomicPtr;
void _ReadWriteBarrier();
#pragma intrinsic(_ReadWriteBarrier)
#define kdAtomicFenceAcquire() _ReadWriteBarrier()
#define kdAtomicFenceRelease() _ReadWriteBarrier()
#endif

KD_API KDAtomicInt* KD_APIENTRY kdAtomicIntCreate(KDint value);
KD_API KDAtomicPtr* KD_APIENTRY kdAtomicPtrCreate(void* value);

KD_API void KD_APIENTRY kdAtomicIntFree(KDAtomicInt *object);
KD_API void KD_APIENTRY kdAtomicPtrFree(KDAtomicPtr *object);

KD_API KDint KD_APIENTRY kdAtomicIntLoad(KDAtomicInt *object);
KD_API void* KD_APIENTRY kdAtomicPtrLoad(KDAtomicPtr *object);

KD_API void KD_APIENTRY kdAtomicIntStore(KDAtomicInt *object, KDint value);
KD_API void KD_APIENTRY kdAtomicPtrStore(KDAtomicPtr *object, void* value);

KD_API KDint KD_APIENTRY kdAtomicIntFetchAdd(KDAtomicInt *object, KDint value);
KD_API KDint KD_APIENTRY kdAtomicIntFetchSub(KDAtomicInt *object, KDint value);

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchange(KDAtomicInt *object, KDint expected, KDint desired);
KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchange(KDAtomicPtr *object, void* expected, void* desired);

#ifdef __cplusplus
}
#endif

#endif /* __kd_LKD_atomic_h_ */

