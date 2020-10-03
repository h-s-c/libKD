
// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2020 Kevin Schmidt
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#include "kdplatform.h"             // for KD_APIENTRY, KD_THREAD_POSIX, KD_API
#include <KD/kd.h>                  // for kdThreadMutexUnlock, kdSetError, kdThreadMu...
#include <KD/KHR_thread_storage.h>  // IWYU pragma: keep
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // for KDThreadStorageKeyKHR, KDThread

/******************************************************************************
 * C includes
 ******************************************************************************/

#if defined(KD_THREAD_C11)
#include <threads.h>
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(KD_THREAD_POSIX)
// IWYU pragma: no_include <bits/pthread_types.h>
#include <pthread.h>  // for pthread_getspecific, pthread_key_create
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#endif

/******************************************************************************
 * Thread-local storage.
 ******************************************************************************/

/* kdGetTLS: Get the thread-local storage pointer. */
KD_API void *KD_APIENTRY kdGetTLS(void)
{
    return kdThreadSelf()->tlsptr;
}

/* kdSetTLS: Set the thread-local storage pointer. */
KD_API void KD_APIENTRY kdSetTLS(void *ptr)
{
    kdThreadSelf()->tlsptr = ptr;
}

/* kdMapThreadStorageKHR: Maps an arbitrary pointer to a global thread storage key. */
struct _KDThreadStorage
{
    KDThreadStorageKeyKHR key;
#if defined(KD_THREAD_C11)
    tss_t nativekey;
#elif defined(KD_THREAD_POSIX)
    pthread_key_t nativekey;
#elif defined(KD_THREAD_WIN32)
    DWORD nativekey;
#else
    void *nativekey;
#endif
    void *id;
};
typedef struct _KDThreadStorage _KDThreadStorage;

static _KDThreadStorage __kd_tls[999];
static KDuint __kd_tls_index = 0;
KDThreadMutex *__kd_tls_mutex = KD_NULL;
KD_API KDThreadStorageKeyKHR KD_APIENTRY KD_APIENTRY kdMapThreadStorageKHR(const void *id)
{
    KDThreadStorageKeyKHR retval = 0;
    kdThreadMutexLock(__kd_tls_mutex);
    for(KDuint i = 0; i < __kd_tls_index; i++)
    {
        if(__kd_tls[i].id == id)
        {
            kdThreadMutexUnlock(__kd_tls_mutex);
            return __kd_tls[i].key;
        }
    }

    /* Key is only 0 when an error occurs. */
    __kd_tls[__kd_tls_index].key = __kd_tls_index + 1;
    kdMemcpy(&__kd_tls[__kd_tls_index].id, &id, sizeof(void *));
#if defined(KD_THREAD_C11)
    if(tss_create(&__kd_tls[__kd_tls_index].nativekey, KD_NULL) != 0)
#elif defined(KD_THREAD_POSIX)
    if(pthread_key_create(&__kd_tls[__kd_tls_index].nativekey, KD_NULL) != 0)
#elif defined(KD_THREAD_WIN32)
    __kd_tls[__kd_tls_index].nativekey = FlsAlloc(NULL);
    if(__kd_tls[__kd_tls_index].nativekey == TLS_OUT_OF_INDEXES)
#else
    if((0))
#endif
    {
        kdSetError(KD_ENOMEM);
    }
    else
    {
        retval = __kd_tls[__kd_tls_index].key;
        __kd_tls_index++;
    }
    kdThreadMutexUnlock(__kd_tls_mutex);
    return retval;
}

/* kdSetThreadStorageKHR: Stores thread-local data. */
KD_API KDint KD_APIENTRY KD_APIENTRY kdSetThreadStorageKHR(KDThreadStorageKeyKHR key, void *data)
{
    KDint retval = -1;
#if defined(KD_THREAD_C11)
    retval = (tss_set(__kd_tls[key - 1].nativekey, data) == thrd_error) ? -1 : 0;
#elif defined(KD_THREAD_POSIX)
    retval = (pthread_setspecific(__kd_tls[key - 1].nativekey, data) == 0) ? 0 : -1;
#elif defined(KD_THREAD_WIN32)
    retval = (FlsSetValue(__kd_tls[key - 1].nativekey, data) == 0) ? -1 : 0;
#else
    retval = 0;
    __kd_tls[key - 1].nativekey = data;
#endif
    /* cppcheck-suppress knownConditionTrueFalse */
    if(retval == -1)
    {
        kdSetError(KD_ENOMEM);
    }
    return retval;
}

/* kdGetThreadStorageKHR: Retrieves previously stored thread-local data. */
KD_API void *KD_APIENTRY KD_APIENTRY kdGetThreadStorageKHR(KDThreadStorageKeyKHR key)
{
#if defined(KD_THREAD_C11)
    return tss_get(__kd_tls[key - 1].nativekey);
#elif defined(KD_THREAD_POSIX)
    return pthread_getspecific(__kd_tls[key - 1].nativekey);
#elif defined(KD_THREAD_WIN32)
    return FlsGetValue(__kd_tls[key - 1].nativekey);
#else
    return __kd_tls[key - 1].nativekey;
#endif
}

void __kdCleanupThreadStorageKHR(void)
{
    kdThreadMutexLock(__kd_tls_mutex);
    for(KDuint i = 0; i < __kd_tls_index; i++)
    {
#if defined(KD_THREAD_C11)
        tss_delete(__kd_tls[i].nativekey);
#elif defined(KD_THREAD_POSIX)
        pthread_key_delete(__kd_tls[i].nativekey);
#elif defined(KD_THREAD_WIN32)
        FlsFree(__kd_tls[i].nativekey);
#endif
    }
    kdThreadMutexUnlock(__kd_tls_mutex);
}
