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
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#define _GNU_SOURCE /* O_CLOEXEC */
#endif
#include "kdplatform.h"             // for KD_THREAD_POSIX, KD_API, KD_...
#include <KD/kd.h>                  // for kdSetError, kdFree, KDThread
#include <KD/KHR_thread_storage.h>  // for kdGetThreadStorageKHR, kdMap...
#include <KD/kdext.h>               // IWYU pragma: keep
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // for KDThread, _KDThreadInternal

/******************************************************************************
 * C includes
 ******************************************************************************/

#if defined(KD_THREAD_C11)
#include <threads.h>
#endif

#if defined(KD_THREAD_POSIX)
#include <errno.h>  // for EINVAL, ENOMEM, ESRCH
#include <time.h>   // for nanosleep
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(KD_THREAD_POSIX)
// IWYU pragma: no_include <bits/pthread_types.h>
#include <pthread.h>  // for pthread_attr_setdetachstate
#endif

#if defined(__linux__)
// IWYU pragma: no_include <bits/types/struct_timespec.h>
// IWYU pragma: no_include <bits/types/time_t.h>
// IWYU pragma: no_include <bits/types/struct_timespec.h>
// IWYU pragma: no_include <linux/time.h>
// IWYU pragma: no_include <time.h>
#include <sys/prctl.h>  // for prctl, PR_SET_NAME
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h> /* emscripten_sleep */
#include <emscripten/threading.h>  /* emscripten_has_threading_support */
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
 * Threads and synchronization
 ******************************************************************************/

/* kdThreadAttrCreate: Create a thread attribute object. */
struct KDThreadAttr {
#if defined(KD_THREAD_POSIX)
    pthread_attr_t nativeattr;
#endif
    KDchar debugname[256];
    KDsize stacksize;
    KDint detachstate;
#if KDSIZE_MAX == KDUINT64_MAX
    KDint8 padding[4];
#endif
};
KD_API KDThreadAttr *KD_APIENTRY kdThreadAttrCreate(void)
{
    KDThreadAttr *attr = (KDThreadAttr *)kdMalloc(sizeof(KDThreadAttr));
    if(attr == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }

    /* Spec default */
    attr->detachstate = KD_THREAD_CREATE_JOINABLE;
    /* Impl default */
    attr->stacksize = 100000;
    kdStrcpy_s(attr->debugname, 256, "KDThread");
#if defined(KD_THREAD_POSIX)
    pthread_attr_init(&attr->nativeattr);
    pthread_attr_setdetachstate(&attr->nativeattr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr->nativeattr, attr->stacksize);
#endif
    return attr;
}

/* kdThreadAttrFree: Free a thread attribute object. */
KD_API KDint KD_APIENTRY kdThreadAttrFree(KDThreadAttr *attr)
{
    kdFree(attr);
    return 0;
}

/* kdThreadAttrSetDetachState: Set detachstate attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetDetachState(KDThreadAttr *attr, KDint detachstate)
{
    if(detachstate == KD_THREAD_CREATE_JOINABLE)
    {
#if defined(KD_THREAD_POSIX)
        pthread_attr_setdetachstate(&attr->nativeattr, PTHREAD_CREATE_JOINABLE);
#endif
    }
    else if(detachstate == KD_THREAD_CREATE_DETACHED)
    {
#if defined(KD_THREAD_POSIX)
        pthread_attr_setdetachstate(&attr->nativeattr, PTHREAD_CREATE_DETACHED);
#endif
    }
    else
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
    attr->detachstate = detachstate;
    return 0;
}


/* kdThreadAttrSetStackSize: Set stacksize attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetStackSize(KDThreadAttr *attr, KDsize stacksize)
{
    attr->stacksize = stacksize;
#if defined(KD_THREAD_POSIX)
    KDint result = pthread_attr_setstacksize(&attr->nativeattr, attr->stacksize);
    if(result == EINVAL)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
#endif
    return 0;
}

/* kdThreadAttrSetDebugNameVEN: Set debugname attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetDebugNameVEN(KDThreadAttr *attr, const char *debugname)
{
    kdStrcpy_s(attr->debugname, 256, debugname);
    return 0;
}

/* kdThreadCreate: Create a new thread. */
struct _KDThreadInternal {
#if defined(KD_THREAD_C11)
    thrd_t nativethread;
#elif defined(KD_THREAD_POSIX)
    pthread_t nativethread;
#elif defined(KD_THREAD_WIN32)
    HANDLE nativethread;
#endif
    void *(*start_routine)(void *);
    void *arg;
    const KDThreadAttr *attr;
};

KDThread *__kdThreadInit(void)
{
    KDThread *thread = (KDThread *)kdMalloc(sizeof(KDThread));
    if(thread == KD_NULL)
    {
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
    thread->internal = (_KDThreadInternal *)kdMalloc(sizeof(_KDThreadInternal));
    thread->eventqueue = __kdQueueCreate(64);
    if(thread->eventqueue == KD_NULL)
    {
        kdFree(thread);
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
    thread->lastevent = KD_NULL;
    thread->lasterror = 0;
    thread->callbackindex = 0;
    thread->callbacks = (_KDCallback **)kdMalloc(sizeof(_KDCallback *));
    if(thread->callbacks == KD_NULL)
    {
        __kdQueueFree(thread->eventqueue);
        kdFree(thread);
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
    return thread;
}

void __kdThreadFree(KDThread *thread)
{
    for(KDint i = 0; i < thread->callbackindex; i++)
    {
        kdFree(thread->callbacks[i]);
    }
    kdFree(thread->callbacks);
    if(thread->lastevent)
    {
        kdFreeEvent(thread->lastevent);
    }
    while(__kdQueueSize(thread->eventqueue) > 0)
    {
        kdFreeEvent((KDEvent *)__kdQueuePull(thread->eventqueue));
    }
    __kdQueueFree(thread->eventqueue);
    kdFree(thread->internal);
    kdFree(thread);
}

KDThreadOnce __kd_threadinit_once = KD_THREAD_ONCE_INIT;
KDThreadStorageKeyKHR __kd_threadlocal = 0;
void __kdThreadInitOnce(void)
{
    __kd_threadlocal = kdMapThreadStorageKHR(&__kd_threadlocal);
}

#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
static void *__kdThreadRun(void *init)
{
    __kdMallocThreadInit();

    KDThread *thread = (KDThread *)init;
    kdThreadOnce(&__kd_threadinit_once, __kdThreadInitOnce);
    /* Set the thread name */
    KD_UNUSED const KDchar *threadname = thread->internal->attr ? thread->internal->attr->debugname : "KDThread";
#if defined(_MSC_VER) && !defined(KD_FREESTANDING)
    typedef HRESULT(WINAPI * SETTHREADDESCRIPTION)(HANDLE hThread, PCWSTR lpThreadDescription);
    SETTHREADDESCRIPTION __SetThreadDescription = KD_NULL;
    HMODULE kernel32 = GetModuleHandle("Kernel32.dll");
    if(kernel32)
    {
        __SetThreadDescription = (SETTHREADDESCRIPTION)GetProcAddress(kernel32, "SetThreadDescription");
    }
    if(__SetThreadDescription)
    {
        WCHAR wthreadname[256] = L"KDThread"; /* implicit memset */
        MultiByteToWideChar(0, 0, threadname, -1, wthreadname, 256);
        __SetThreadDescription(GetCurrentThread(), (const WCHAR *)wthreadname);
    }
    else
    {
#pragma warning(push)
#pragma warning(disable : 4204)
#pragma warning(disable : 6312)
#pragma warning(disable : 6322)
/* https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx */
#pragma pack(push, 8)
        struct THREADNAME_INFO {
            KDuint32 type;       // must be 0x1000
            const KDchar *name;  // pointer to name (in user addr space)
            KDuint32 threadid;   // thread ID (-1=caller thread)
            KDuint32 flags;      // reserved for future use, must be zero
        };
#pragma pack(pop)
        struct THREADNAME_INFO info = {.type = 0x1000, .name = threadname, .threadid = GetCurrentThreadId(), .flags = 0};
        if(IsDebuggerPresent())
        {
            __try
            {
                RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
            }

            __except(EXCEPTION_CONTINUE_EXECUTION)

            {
            }
        }
#pragma warning(pop)
    }
#elif defined(__linux__)
    prctl(PR_SET_NAME, (long)threadname, 0UL, 0UL, 0UL);
#endif

#if defined(KD_THREAD_POSIX) && defined(__APPLE__)
    pthread_setname_np(threadname);
#endif

    kdSetThreadStorageKHR(__kd_threadlocal, thread);
    void *result = thread->internal->start_routine(thread->internal->arg);
    if(thread->internal->attr && thread->internal->attr->detachstate == KD_THREAD_CREATE_DETACHED)
    {
        __kdThreadFree(thread);
    }

    __kdMallocThreadFinal();
    return result;
}
#endif

KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
#if !defined(KD_THREAD_C11) && !defined(KD_THREAD_POSIX) && !defined(KD_THREAD_WIN32)
    if((0))
    {
#elif defined(__EMSCRIPTEN__)
    if(emscripten_has_threading_support())
    {
#else
    if((1))
    {
#endif
        KDThread *thread = __kdThreadInit();
        if(thread == KD_NULL)
        {
            kdSetError(KD_EAGAIN);
            return KD_NULL;
        }
        thread->internal->start_routine = start_routine;
        thread->internal->arg = arg;
        thread->internal->attr = attr;

        KDint error = 0;
#if defined(__GNUC__)
#if(__GNUC__ > 7)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#endif

#if defined(KD_THREAD_C11)
        error = thrd_create(&thread->internal->nativethread, (thrd_start_t)__kdThreadRun, thread);
#elif defined(KD_THREAD_POSIX)
        error = pthread_create(&thread->internal->nativethread, attr ? &attr->nativeattr : KD_NULL, __kdThreadRun, thread);
#elif defined(KD_THREAD_WIN32)
        thread->internal->nativethread = CreateThread(KD_NULL, attr ? attr->stacksize : 0, (LPTHREAD_START_ROUTINE)__kdThreadRun, (LPVOID)thread, 0, KD_NULL);
        error = thread->internal->nativethread ? 0 : 1;
#endif

#if defined(__GNUC__)
#if(__GNUC__ > 7)
#pragma GCC diagnostic pop
#endif
#endif
        /* cppcheck-suppress knownConditionTrueFalse */
        if(error != 0)
        {
            __kdThreadFree(thread);
            kdSetError(KD_EAGAIN);
            return KD_NULL;
        }

        if(attr && attr->detachstate == KD_THREAD_CREATE_DETACHED)
        {
            kdThreadDetach(thread);
            return KD_NULL;
        }
        return thread;
    }
    else
    {
        kdSetError(KD_ENOSYS);
        return KD_NULL;
    }
}

/* kdThreadExit: Terminate this thread. */
KD_API KD_NORETURN void KD_APIENTRY kdThreadExit(void *retval)
{
    KD_UNUSED KDint result = 0;
    if(retval)
    {
        result = *(KDint *)retval;
    }

#if defined(KD_THREAD_C11)
    thrd_exit(result);
#elif defined(KD_THREAD_POSIX)
    pthread_exit(retval);
#elif defined(KD_THREAD_WIN32)
    ExitThread(result);
    while(1)
    {
        ;
    }
#else
    kdExit(result);
#endif
}

/* kdThreadJoin: Wait for termination of another thread. */
KD_API KDint KD_APIENTRY kdThreadJoin(KDThread *thread, void **retval)
{
    /* cppcheck-suppress unreadVariable */
    KDint ipretvalinit = 0;
    KD_UNUSED KDint *ipretval = &ipretvalinit;
    if(retval)
    {
        /* cppcheck-suppress unreadVariable */
        ipretval = *retval;
    }

    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDint error = 0;
    KDint result = 0;
#if defined(KD_THREAD_C11)
    error = thrd_join(thread->internal->nativethread, ipretval);
    if(error == thrd_error)
#elif defined(KD_THREAD_POSIX)
    error = pthread_join(thread->internal->nativethread, retval);
    if(error == EINVAL || error == ESRCH)
#elif defined(KD_THREAD_WIN32)
    error = WaitForSingleObject(thread->internal->nativethread, INFINITE);
    GetExitCodeThread(thread->internal->nativethread, (LPDWORD)ipretval);
    CloseHandle(thread->internal->nativethread);
    if(error != 0)
#else
    kdAssert(0);
#endif
    {
        kdSetError(KD_EINVAL);
        result = -1;
    }
    __kdThreadFree(thread);
    return result;
}

/* kdThreadDetach: Allow resources to be freed as soon as a thread terminates. */
KD_API KDint KD_APIENTRY kdThreadDetach(KDThread *thread)
{
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = thrd_detach(thread->internal->nativethread);
#elif defined(KD_THREAD_POSIX)
    error = pthread_detach(thread->internal->nativethread);
#elif defined(KD_THREAD_WIN32)
    CloseHandle(thread->internal->nativethread);
#else
    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDThread *dummythread = thread;
    kdAssert(0);
#endif
    /* cppcheck-suppress knownConditionTrueFalse */
    if(error != 0)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
    return 0;
}

/* kdThreadSelf: Return calling thread's ID. */
KD_API KDThread *KD_APIENTRY kdThreadSelf(void)
{
    return kdGetThreadStorageKHR(__kd_threadlocal);
}

/* kdThreadOnce: Wrap initialization code so it is executed only once. */
#ifndef KD_NO_STATIC_DATA
#if defined(KD_THREAD_WIN32)
static BOOL CALLBACK __kd_WindowsCallOneCallback(KD_UNUSED PINIT_ONCE flag, PVOID param, KD_UNUSED PVOID *context)
{
    void (*func)(void) = KD_NULL;
    kdMemcpy(&func, &param, sizeof(param));
    func();
    return TRUE;
}
#endif
KD_API KDint KD_APIENTRY kdThreadOnce(KDThreadOnce *once_control, void (*init_routine)(void))
{
#if defined(KD_THREAD_C11)
    call_once((once_flag *)once_control, init_routine);
#elif defined(KD_THREAD_POSIX)
    pthread_once((pthread_once_t *)once_control, init_routine);
#elif defined(KD_THREAD_WIN32)
    void *pfunc = KD_NULL;
    kdMemcpy(&pfunc, &init_routine, sizeof(init_routine));
    InitOnceExecuteOnce((PINIT_ONCE)once_control, __kd_WindowsCallOneCallback, pfunc, KD_NULL);
#else
    if(once_control->impl == 0)
    {
        once_control->impl = (void *)1;
        init_routine();
    }
#endif
    return 0;
}
#endif /* ndef KD_NO_STATIC_DATA */

/* kdThreadMutexCreate: Create a mutex. */
typedef struct _KDMutexAttr _KDMutexAttr;
struct _KDMutexAttr {
    /* This is useful for our kdMalloc implementation. */
    KDThreadMutex *staticmutex;
};
struct KDThreadMutex {
#if defined(KD_THREAD_C11)
    mtx_t nativemutex;
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_t nativemutex;
#elif defined(KD_THREAD_WIN32)
    SRWLOCK nativemutex;
#else
    KDboolean nativemutex;
#endif
    const _KDMutexAttr *mutexattr;
};
KD_API KDThreadMutex *KD_APIENTRY kdThreadMutexCreate(const void *mutexattr)
{
    KDThreadMutex *mutex = KD_NULL;
    const _KDMutexAttr *attr = (const _KDMutexAttr *)mutexattr;
    if(attr && attr->staticmutex)
    {
        mutex = attr->staticmutex;
        mutex->mutexattr = attr;
    }
    else
    {
        mutex = (KDThreadMutex *)kdMalloc(sizeof(KDThreadMutex));
    }
    if(mutex == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = mtx_init((mtx_t *)&mutex->nativemutex, mtx_plain);
#elif defined(KD_THREAD_POSIX)
    error = pthread_mutex_init((pthread_mutex_t *)&mutex->nativemutex, KD_NULL);
    if(error == ENOMEM)
    {
        kdSetError(KD_ENOMEM);
        if(attr && attr->staticmutex)
        {
            return KD_NULL;
        }
        kdFree(mutex);
        return KD_NULL;
    }
#elif defined(KD_THREAD_WIN32)
    InitializeSRWLock((SRWLOCK *)&mutex->nativemutex);
#else
    mutex->nativemutex = KD_FALSE;
#endif
    /* cppcheck-suppress knownConditionTrueFalse */
    if(error != 0)
    {
        kdSetError(KD_EAGAIN);
        if(attr && attr->staticmutex)
        {
            return KD_NULL;
        }
        kdFree(mutex);
        return KD_NULL;
    }
    return mutex;
}

/* kdThreadMutexFree: Free a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexFree(KDThreadMutex *mutex)
{
    if(mutex)
    {
/* No need to free anything on WIN32*/
#if defined(KD_THREAD_C11)
        mtx_destroy((mtx_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
        pthread_mutex_destroy((pthread_mutex_t *)&mutex->nativemutex);
#endif
        const _KDMutexAttr *attr = (const _KDMutexAttr *)mutex->mutexattr;
        if(attr && attr->staticmutex)
        {
            return 0;
        }
        kdFree(mutex);
    }
    return 0;
}

/* kdThreadMutexLock: Lock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexLock(KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    mtx_lock((mtx_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_lock((pthread_mutex_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    AcquireSRWLockExclusive((SRWLOCK *)&mutex->nativemutex);
#else
    mutex->nativemutex = KD_TRUE;
#endif
    return 0;
}

/* kdThreadMutexUnlock: Unlock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexUnlock(KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    mtx_unlock((mtx_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_unlock((pthread_mutex_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    ReleaseSRWLockExclusive((SRWLOCK *)&mutex->nativemutex);
#else
    mutex->nativemutex = KD_FALSE;
#endif
    return 0;
}

/* kdThreadCondCreate: Create a condition variable. */
struct KDThreadCond {
#if defined(KD_THREAD_C11)
    cnd_t nativecond;
#elif defined(KD_THREAD_POSIX)
    pthread_cond_t nativecond;
#elif defined(KD_THREAD_WIN32)
    CONDITION_VARIABLE nativecond;
#else
    KDint placebo;
#endif
};
KD_API KDThreadCond *KD_APIENTRY kdThreadCondCreate(KD_UNUSED const void *attr)
{
#if !defined(KD_THREAD_C11) && !defined(KD_THREAD_POSIX) && !defined(KD_THREAD_WIN32)
    if((0))
    {
#elif defined(__EMSCRIPTEN__)
    if(emscripten_has_threading_support())
    {
#else
    if((1))
    {
#endif
        KDThreadCond *cond = (KDThreadCond *)kdMalloc(sizeof(KDThreadCond));
        if(cond == KD_NULL)
        {
            kdSetError(KD_ENOMEM);
            return KD_NULL;
        }
        KDint error = 0;
#if defined(KD_THREAD_C11)
        error = cnd_init(&cond->nativecond);
        if(error == thrd_nomem)
        {
            kdSetError(KD_ENOMEM);
            kdFree(cond);
            return KD_NULL;
        }
#elif defined(KD_THREAD_POSIX)
        error = pthread_cond_init(&cond->nativecond, KD_NULL);
#elif defined(KD_THREAD_WIN32)
        InitializeConditionVariable(&cond->nativecond);
#endif
        /* cppcheck-suppress knownConditionTrueFalse */
        if(error != 0)
        {
            kdSetError(KD_EAGAIN);
            kdFree(cond);
            return KD_NULL;
        }
        return cond;
    }
    else
    {
        kdSetError(KD_ENOSYS);
        return KD_NULL;
    }
}

/* kdThreadCondFree: Free a condition variable. */
KD_API KDint KD_APIENTRY kdThreadCondFree(KDThreadCond *cond)
{
/* No need to free anything on WIN32*/
#if defined(KD_THREAD_C11)
    cnd_destroy(&cond->nativecond);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_destroy(&cond->nativecond);
#endif
    kdFree(cond);
    return 0;
}

/* kdThreadCondSignal, kdThreadCondBroadcast: Signal a condition variable. */
KD_API KDint KD_APIENTRY kdThreadCondSignal(KDThreadCond *cond)
{
#if defined(KD_THREAD_C11)
    cnd_signal(&cond->nativecond);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_signal(&cond->nativecond);
#elif defined(KD_THREAD_WIN32)
    WakeConditionVariable(&cond->nativecond);
#else
    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDThreadCond *dummycond = cond;
    kdAssert(0);
#endif
    return 0;
}

KD_API KDint KD_APIENTRY kdThreadCondBroadcast(KDThreadCond *cond)
{
#if defined(KD_THREAD_C11)
    cnd_broadcast(&cond->nativecond);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_broadcast(&cond->nativecond);
#elif defined(KD_THREAD_WIN32)
    WakeAllConditionVariable(&cond->nativecond);
#else
    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDThreadCond *dummycond = cond;
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadCondWait: Wait for a condition variable to be signalled. */
KD_API KDint KD_APIENTRY kdThreadCondWait(KDThreadCond *cond, KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    cnd_wait(&cond->nativecond, (mtx_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_wait(&cond->nativecond, (pthread_mutex_t *)&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    SleepConditionVariableSRW(&cond->nativecond, (SRWLOCK *)&mutex->nativemutex, INFINITE, 0);
#else
    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDThreadCond *dummycond = cond;
    /* cppcheck-suppress unreadVariable */
    KD_UNUSED KDThreadMutex *dummymutex = mutex;
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadSemCreate: Create a semaphore. */
struct KDThreadSem {
    KDThreadMutex *mutex;
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    KDThreadCond *condition;
#endif
    KDuint count;
    KDint8 padding[4];
};
KD_API KDThreadSem *KD_APIENTRY kdThreadSemCreate(KDuint value)
{
    KDThreadSem *sem = (KDThreadSem *)kdMalloc(sizeof(KDThreadSem));
    if(sem == KD_NULL)
    {
        kdSetError(KD_ENOSPC);
        return KD_NULL;
    }

    sem->mutex = kdThreadMutexCreate(KD_NULL);
    if(sem->mutex == KD_NULL)
    {
        kdFree(sem);
        kdSetError(KD_ENOSPC);
        return KD_NULL;
    }
    kdThreadMutexLock(sem->mutex);
    sem->count = value;
    kdThreadMutexUnlock(sem->mutex);

#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    sem->condition = kdThreadCondCreate(KD_NULL);
    if(sem->condition == KD_NULL)
    {
        kdThreadMutexFree(sem->mutex);
        kdFree(sem);
        kdSetError(KD_ENOSPC);
        return KD_NULL;
    }
#endif
    return sem;
}

/* kdThreadSemFree: Free a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemFree(KDThreadSem *sem)
{
    kdThreadMutexFree(sem->mutex);
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    kdThreadCondFree(sem->condition);
#endif
    kdFree(sem);
    return 0;
}

/* kdThreadSemWait: Lock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemWait(KDThreadSem *sem)
{
    kdThreadMutexLock(sem->mutex);
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    while(sem->count == 0)
    {
        kdThreadCondWait(sem->condition, sem->mutex);
    }
#endif
    --sem->count;
    kdThreadMutexUnlock(sem->mutex);
    return 0;
}

/* kdThreadSemPost: Unlock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemPost(KDThreadSem *sem)
{
    kdThreadMutexLock(sem->mutex);
    ++sem->count;
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    kdThreadCondSignal(sem->condition);
#endif
    kdThreadMutexUnlock(sem->mutex);
    return 0;
}

/* kdThreadSleepVEN: Blocks the current thread for nanoseconds. */
KD_API KDint KD_APIENTRY kdThreadSleepVEN(KDust timeout)
{
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX)
    struct timespec ts;
    kdMemset(&ts, 0, sizeof(ts));
    /* Determine seconds from the overall nanoseconds */
    if((timeout % 1000000000) == 0)
    {
        ts.tv_sec = (time_t)(timeout / 1000000000);
    }
    else
    {
        ts.tv_sec = (time_t)(timeout - (timeout % 1000000000)) / 1000000000;
    }

    /* Remaining nanoseconds */
    ts.tv_nsec = (KDint32)timeout - ((KDint32)ts.tv_sec * 1000000000);
#endif

#if defined(__EMSCRIPTEN__)
    emscripten_sleep((KDuint)timeout / 1000000);
#elif defined(KD_THREAD_C11)
    thrd_sleep(&ts, KD_NULL);
#elif defined(KD_THREAD_POSIX)
    nanosleep(&ts, KD_NULL);
#elif defined(KD_THREAD_WIN32)
    HANDLE timer = CreateWaitableTimerA(KD_NULL, 1, KD_NULL);
    if(!timer)
    {
        kdAssert(0);
    }
    LARGE_INTEGER li = {{0}};
    li.QuadPart = -(timeout / 100);
    if(!SetWaitableTimer(timer, &li, 0, KD_NULL, KD_NULL, 0))
    {
        kdAssert(0);
    }
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    KDust now, then;
    now = then = kdGetTimeUST();
    while((now - then) < timeout)
    {
        now = kdGetTimeUST();
    }
#endif
    return 0;
}
