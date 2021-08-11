
/*******************************************************
 * OpenKODE Core extension: VEN_atomic_ops
 *******************************************************/

#ifndef __kd_VEN_atomic_ops_h_
#define __kd_VEN_atomic_ops_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(KD_ATOMIC_C11)
#include <stdatomic.h>

typedef _Atomic(KDint32) KDAtomicIntVEN;
typedef _Atomic(void*) KDAtomicPtrVEN;

static KD_INLINE KDint32 kdAtomicIntLoadVEN(KDAtomicIntVEN* src) { return atomic_load_explicit(src, memory_order_relaxed); }
static KD_INLINE void  kdAtomicIntStoreVEN(KDAtomicIntVEN* dst, KDint32 val) { atomic_store_explicit(dst, val, memory_order_relaxed); }
static KD_INLINE KDint32 kdAtomicIntFetchAddVEN(KDAtomicIntVEN* val, KDint32 add) { return atomic_fetch_add_explicit(val, add, memory_order_relaxed); }
static KD_INLINE KDboolean kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN* dst, KDint32 val, KDint32 ref) { return atomic_compare_exchange_weak_explicit(dst, &ref, val, memory_order_relaxed, memory_order_relaxed); }
static KD_INLINE void* kdAtomicPtrLoadVEN(KDAtomicPtrVEN* src) { return (void*)atomic_load_explicit(src, memory_order_relaxed); }
static KD_INLINE void  kdAtomicPtrStoreVEN(KDAtomicPtrVEN* dst, void* val) { atomic_store_explicit(dst, val, memory_order_relaxed); }
static KD_INLINE KDboolean kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN* dst, void* val, void* ref) { return atomic_compare_exchange_weak_explicit(dst, &ref, val, memory_order_relaxed, memory_order_relaxed); }

#elif defined(KD_ATOMIC_WIN32)
#include <intrin.h>

typedef long KDAtomicIntVEN;
typedef void*  KDAtomicPtrVEN;

static KD_INLINE void  kdAtomicIntStoreVEN(KDAtomicIntVEN* dst, KDint32 val) { _InterlockedExchange(dst, (long)val); }
static KD_INLINE KDint32 kdAtomicIntIncrementVEN(KDAtomicIntVEN* val) { return (KDint32)_InterlockedIncrement(val); }
static KD_INLINE KDint32 kdAtomicIntDecrementVEN(KDAtomicIntVEN* val) { return (KDint32)_InterlockedDecrement(val); }
static KD_INLINE KDint32 kdAtomicIntFetchAddVEN(KDAtomicIntVEN* val, KDint32 add) { return (KDint32)_InterlockedExchangeAdd(val, (long)add); }
static KD_INLINE KDboolean kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN* dst, KDint32 val, KDint32 ref) { return (_InterlockedCompareExchange((volatile long*)dst, (long)val, (long)ref) == (long)ref) ? 1 : 0; }
#if defined(_M_IX86)
static KD_INLINE void  kdAtomicPtrStoreVEN(KDAtomicPtrVEN* dst, void * val) { _InterlockedExchange((volatile long*)dst, (long)val); }
static KD_INLINE KDboolean kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN* dst, void* val, void* ref) { return (_InterlockedCompareExchange((volatile long*)dst, (long)val, (long)ref) == (long)ref) ? 1 : 0; }
#else
static KD_INLINE void  kdAtomicPtrStoreVEN(KDAtomicPtrVEN* dst, void * val) { _InterlockedExchangePointer(dst, val); }
static KD_INLINE KDboolean kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN* dst, void* val, void* ref) { return (_InterlockedCompareExchangePointer((void * volatile*)dst, val, ref) == ref) ? 1 : 0; }
#endif

#elif defined(KD_ATOMIC_SYNC)

typedef KDint32 KDAtomicIntVEN;
typedef void* KDAtomicPtrVEN;

static KD_INLINE void  kdAtomicIntStoreVEN(KDAtomicIntVEN* dst, KDint32 val) { __sync_lock_test_and_set(dst, val); }
static KD_INLINE KDint32 kdAtomicIntFetchAddVEN(KDAtomicIntVEN* val, KDint32 add) { return __sync_fetch_and_add(val, add); }
static KD_INLINE KDboolean kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN* dst, KDint32 val, KDint32 ref) { return __sync_bool_compare_and_swap(dst, ref, val); }
static KD_INLINE void  kdAtomicPtrStoreVEN(KDAtomicPtrVEN* dst, void* val) { KD_UNUSED void *unused = __sync_lock_test_and_set(dst, val); }
static KD_INLINE KDboolean kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN* dst, void* val, void* ref) { return __sync_bool_compare_and_swap(dst, ref, val); }

#elif defined(KD_ATOMIC_EMSCRIPTEN)
#include <emscripten/threading.h>

typedef KDint32 KDAtomicIntVEN;
typedef void* KDAtomicPtrVEN;

static KD_INLINE KDint32 kdAtomicIntLoadVEN(KDAtomicIntVEN* src) { return (KDint32)emscripten_atomic_load_u32(src); }
static KD_INLINE void  kdAtomicIntStoreVEN(KDAtomicIntVEN* dst, KDint32 val) { emscripten_atomic_store_u32(dst, (KDuint32)val); }
static KD_INLINE KDint32 kdAtomicIntFetchAddVEN(KDAtomicIntVEN* val, KDint32 add) { return (KDint32)emscripten_atomic_add_u32(val, (KDuint32)add); }
static KD_INLINE KDboolean kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN* dst, KDint32 val, KDint32 ref) { return emscripten_atomic_cas_u32(dst, (KDuint32)ref, (KDuint32)val) == (KDuint32)ref; }
static KD_INLINE void* kdAtomicPtrLoadVEN(KDAtomicPtrVEN* src) { return (void *)(KDuint32)emscripten_atomic_load_u32(&src); }
static KD_INLINE void  kdAtomicPtrStoreVEN(KDAtomicPtrVEN* dst, void* val) { emscripten_atomic_store_u32(&dst, (KDuint32)val); }
static KD_INLINE KDboolean kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN* dst, void* val, void* ref) { return emscripten_atomic_cas_u32(&dst, (KDuint32)ref, (KDuint32)val) == (KDuint32)ref; }

#endif

#if defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_SYNC)
static KD_INLINE KDint32 kdAtomicIntLoadVEN(KDAtomicIntVEN* src) 
{ 
    KDint32 value = 0;
    do
    {
        value = *src;
    } while(!kdAtomicIntCompareExchangeVEN(src, value, value));
    return value;
}

static KD_INLINE void* kdAtomicPtrLoadVEN(KDAtomicPtrVEN* src) 
{
    void* value = 0;
    do
    {
        value = *src;
    } while(!kdAtomicPtrCompareExchangeVEN(src, value, value));
    return value;
}
#endif

#if !defined(KD_ATOMIC_WIN32)
static KD_INLINE KDint32 kdAtomicIntIncrementVEN(KDAtomicIntVEN* val) { return kdAtomicIntFetchAddVEN(val, 1) + 1; }
static KD_INLINE KDint32 kdAtomicIntDecrementVEN(KDAtomicIntVEN* val) { return kdAtomicIntFetchAddVEN(val, -1) - 1; }
#endif

#ifdef __cplusplus
}
#endif

#endif /* __kd_VEN_atomic_ops_h_ */
