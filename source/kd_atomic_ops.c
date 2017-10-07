// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2017 Kevin Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 ******************************************************************************/

/******************************************************************************
 * KD includes
 ******************************************************************************/

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wpadded"
#endif
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

/* clang-format off */
#if defined(KD_ATOMIC_C11)
#   include <stdatomic.h> /* atomic_.. */
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(KD_ATOMIC_EMSCRIPTEN)
#   include <emscripten/threading.h> /* emscripten_atomic_.. */
#endif

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <intrin.h> /* _Interlocked.. */
#endif
/* clang-format on */

/******************************************************************************
 * OpenKODE Core extension: KD_VEN_atomic_ops
 ******************************************************************************/

#if defined(KD_ATOMIC_C11)
struct KDAtomicIntVEN {
    atomic_int value;
};
struct KDAtomicPtrVEN {
    atomic_uintptr_t value;
};
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_SYNC) || defined(KD_ATOMIC_MUTEX) || defined(KD_ATOMIC_EMSCRIPTEN)
struct KDAtomicIntVEN {
    KDint value;
#if defined(KD_ATOMIC_MUTEX)
    KDThreadMutex *mutex;
#endif
};
struct KDAtomicPtrVEN {
    void *value;
#if defined(KD_ATOMIC_MUTEX)
    KDThreadMutex *mutex;
#endif
};
#endif

KD_API KDAtomicIntVEN *KD_APIENTRY kdAtomicIntCreateVEN(KDint value)
{
    KDAtomicIntVEN *object = (KDAtomicIntVEN *)kdMalloc(sizeof(KDAtomicIntVEN));
    if(object == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_SYNC) || defined(KD_ATOMIC_MUTEX) || defined(KD_ATOMIC_EMSCRIPTEN)
    object->value = value;
#if defined(KD_ATOMIC_MUTEX)
    object->mutex = kdThreadMutexCreate(KD_NULL);
#endif
#endif
    return object;
}

KD_API KDAtomicPtrVEN *KD_APIENTRY kdAtomicPtrCreateVEN(void *value)
{
    KDAtomicPtrVEN *object = (KDAtomicPtrVEN *)kdMalloc(sizeof(KDAtomicPtrVEN));
    if(object == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, (KDuintptr)value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_SYNC) || defined(KD_ATOMIC_MUTEX) || defined(KD_ATOMIC_EMSCRIPTEN)
    object->value = value;
#if defined(KD_ATOMIC_MUTEX)
    object->mutex = kdThreadMutexCreate(KD_NULL);
#endif
#endif
    return object;
}

KD_API KDint KD_APIENTRY kdAtomicIntFreeVEN(KDAtomicIntVEN *object)
{
#if defined(KD_ATOMIC_MUTEX)
    kdThreadMutexFree(object->mutex);
#endif
    kdFree(object);
    return 0;
}

KD_API KDint KD_APIENTRY kdAtomicPtrFreeVEN(KDAtomicPtrVEN *object)
{
#if defined(KD_ATOMIC_MUTEX)
    kdThreadMutexFree(object->mutex);
#endif
    kdFree(object);
    return 0;
}

KD_API KDint KD_APIENTRY kdAtomicIntLoadVEN(KDAtomicIntVEN *object)
{
#if defined(KD_ATOMIC_C11)
    return atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_SYNC) || defined(KD_ATOMIC_MUTEX)
    KDint value = 0;
    do
    {
        value = object->value;
    } while(!kdAtomicIntCompareExchangeVEN(object, value, value));
    return value;
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_load_n(&object->value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return emscripten_atomic_load_u32(&object->value);
#endif
}

KD_API void *KD_APIENTRY kdAtomicPtrLoadVEN(KDAtomicPtrVEN *object)
{
#if defined(KD_ATOMIC_C11)
    return (void *)atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_SYNC) || defined(KD_ATOMIC_MUTEX)
    void *value = 0;
    do
    {
        value = object->value;
    } while(!kdAtomicPtrCompareExchangeVEN(object, value, value));
    return value;
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_load_n(&object->value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return (void *)emscripten_atomic_load_u32(&object->value);
#endif
}

KD_API void KD_APIENTRY kdAtomicIntStoreVEN(KDAtomicIntVEN *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    atomic_store(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    _InterlockedExchange((long *)&object->value, (long)value);
#elif defined(KD_ATOMIC_BUILTIN)
    __atomic_store_n(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    __sync_lock_test_and_set(&object->value, value);
#elif defined(KD_ATOMIC_MUTEX)
    kdThreadMutexLock(object->mutex);
    object->value = value;
    kdThreadMutexUnlock(object->mutex);
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    emscripten_atomic_store_u32(&object->value, value);
#endif
}

KD_API void KD_APIENTRY kdAtomicPtrStoreVEN(KDAtomicPtrVEN *object, void *value)
{
#if defined(KD_ATOMIC_C11)
    atomic_store(&object->value, (KDuintptr)value);
#elif defined(KD_ATOMIC_WIN32) && defined(_M_IX86)
    _InterlockedExchange((long *)&object->value, (long)value);
#elif defined(KD_ATOMIC_WIN32)
    _InterlockedExchangePointer(&object->value, value);
#elif defined(KD_ATOMIC_BUILTIN)
    __atomic_store_n(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    KD_UNUSED void *unused = __sync_lock_test_and_set(&object->value, value);
#elif defined(KD_ATOMIC_MUTEX)
    kdThreadMutexLock(object->mutex);
    object->value = value;
    kdThreadMutexUnlock(object->mutex);
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    emscripten_atomic_store_u32(&object->value, (KDuint32)value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchAddVEN(KDAtomicIntVEN *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_add(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long *)&object->value, (long)value);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_add_fetch(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    return __sync_fetch_and_add(&object->value, value);
#elif defined(KD_ATOMIC_MUTEX)
    kdThreadMutexLock(object->mutex);
    KDint retval = object->value;
    object->value = object->value + value;
    kdThreadMutexUnlock(object->mutex);
    return retval;
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return emscripten_atomic_add_u32(&object->value, value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchSubVEN(KDAtomicIntVEN *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_sub(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long *)&object->value, (long)-value);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_sub_fetch(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    return __sync_fetch_and_sub(&object->value, value);
#elif defined(KD_ATOMIC_MUTEX)
    KDint retval = 0;
    kdThreadMutexLock(object->mutex);
    retval = object->value;
    object->value = object->value - value;
    kdThreadMutexUnlock(object->mutex);
    return retval;
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return emscripten_atomic_sub_u32(&object->value, value);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN *object, KDint expected, KDint desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, &expected, desired);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchange((long *)&object->value, (long)desired, (long)expected) == (long)expected);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_compare_exchange_n(&object->value, &expected, desired, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    return __sync_bool_compare_and_swap(&object->value, expected, desired);
#elif defined(KD_ATOMIC_MUTEX)
    KDboolean retval = KD_FALSE;
    kdThreadMutexLock(object->mutex);
    if(object->value == expected)
    {
        object->value = desired;
        retval = KD_TRUE;
    }
    kdThreadMutexUnlock(object->mutex);
    return retval;
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return emscripten_atomic_cas_u32(&object->value, expected, desired);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN *object, void *expected, void *desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, (KDuintptr *)&expected, (KDuintptr)desired);
#elif defined(KD_ATOMIC_WIN32) && defined(_M_IX86)
    return (_InterlockedCompareExchange((long *)&object->value, (long)desired, (long)expected) == (long)expected);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchangePointer(&object->value, desired, expected) == expected);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_compare_exchange_n(&object->value, &expected, desired, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_SYNC)
    return __sync_bool_compare_and_swap(&object->value, expected, desired);
#elif defined(KD_ATOMIC_MUTEX)
    KDboolean retval = KD_FALSE;
    kdThreadMutexLock(object->mutex);
    if(object->value == expected)
    {
        object->value = desired;
        retval = KD_TRUE;
    }
    kdThreadMutexUnlock(object->mutex);
    return retval;
#elif defined(KD_ATOMIC_EMSCRIPTEN)
    return emscripten_atomic_cas_u32(&object->value, (KDuint32)expected, (KDuint32)desired);
#endif
}
