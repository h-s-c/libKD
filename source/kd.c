/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2016 Kevin Schmidt
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
 * Implementation notes
 *
 * - Only one window is supported
 * - Networking is not supported
 * - KD_EVENT_QUIT events received by threads other then the mainthread
 *   only exit the thread
 * - To receive orientation changes AndroidManifest.xml should include
 *   android:configChanges="orientation|keyboardHidden|screenSize"
 *
 ******************************************************************************/

/******************************************************************************
 * Header workarounds
 ******************************************************************************/

#ifdef __unix__
    #ifdef __linux__
        #define _GNU_SOURCE
    #endif
    #ifdef __EMSCRIPTEN__
        #define _POSIX_SOURCE
    #endif
    #include <sys/param.h>
    #ifdef BSD
        #define _BSD_SOURCE
    #endif
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif

/* XSI streams are optional and not supported by MinGW */
#if defined(__MINGW32__)
    #define ENODATA    61
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/VEN_keyboard.h>
#include <KD/VEN_atomic.h>
#include <KD/VEN_queue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

/******************************************************************************
 * C11 includes
 ******************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(KD_THREAD_C11) && !defined(_MSC_VER)
    #include <threads.h>
#endif
#if defined(KD_ATOMIC_C11)
    #include <stdatomic.h>
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(KD_THREAD_POSIX)
#include <pthread.h>
#endif

#if  defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>

    #include <sys/stat.h>
    #include <sys/syscall.h>
    #include <sys/utsname.h>
    #if defined(__APPLE__)
        #include<sys/mount.h>
    #else
        #include<sys/vfs.h>
    #endif
    #include <fcntl.h>
    #include <dirent.h>
    #include <dlfcn.h>

    #if defined(__linux__)
        #include <sys/prctl.h>
    #endif

    /* POSIX reserved but OpenKODE uses this */
    #undef st_mtime

    #if defined(__ANDROID__)
        #include <android/log.h>
        #include <android/native_activity.h>
        #include <android/native_window.h>
        #include <android/window.h>
    #else
        #if defined(KD_WINDOW_SUPPORTED)
            #include <X11/Xlib.h>
            #include <X11/Xutil.h>
        #endif
    #endif
#endif

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <wincrypt.h>
    #include <bcrypt.h>
	#include <objbase.h>
	#include <direct.h>
	#include <io.h>
	/* Fix POSIX name warning */
	#define mkdir _mkdir
	#define rmdir _rmdir
	#define fileno _fileno
	#define access _access
    /* Windows has some POSIX support */
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dirent.h>
    /* MSVC 12 and lower is missing some things */
    #if defined(_MSC_VER) && _MSC_VER <= 1800
        struct timespec
        {
            long tv_sec;
            long tv_nsec;
        };
        int snprintf ( char * s, size_t n, const char * format, ... )
        {
            int ret;
            va_list arg;
            va_start(arg, format);
            ret = vsnprintf(s, n, format, arg);
            va_end(arg);
            return ret;
        }
    #endif
    /* MSVC threads.h */
    #if defined(KD_THREAD_C11) || defined(KD_THREAD_WIN32)
        static BOOL CALLBACK call_once_callback(KD_UNUSED PINIT_ONCE flag, PVOID param, KD_UNUSED PVOID *context)
        {
            void(*func)(void) = KD_NULL;
            kdMemcpy(&func, &param, sizeof(param));
            func();
            return TRUE;
        }
    #endif
    #if defined(KD_THREAD_C11)
        #include <thr/threads.h>
        #undef thrd_sleep
        int thrd_sleep(const struct timespec* time_point, KD_UNUSED struct timespec* remaining)
        {
            KDint64 timeout = time_point->tv_sec ? time_point->tv_sec * 1000000000 : 0;
            timeout += time_point->tv_nsec;
            HANDLE timer = CreateWaitableTimer(KD_NULL, 1, KD_NULL);
            if(!timer) { kdAssert(0); }
            LARGE_INTEGER li = {{ 0 }};
            li.QuadPart = -(timeout / 100);
            if(!SetWaitableTimer(timer, &li, 0, KD_NULL, KD_NULL, 0)) { kdAssert(0); }
            WaitForSingleObject(timer, INFINITE);
            CloseHandle(timer);
            return 0;
        }
        #if defined(_MSC_VER) && _MSC_VER == 1900
        typedef INIT_ONCE once_flag;
        void call_once(once_flag* flag, void(*func)(void))
        {
            void* pfunc = KD_NULL;
            kdMemcpy(&pfunc, &func, sizeof(func));
            InitOnceExecuteOnce(flag, call_once_callback, pfunc, NULL);
        }
        #endif
    #endif
	/* MSVC redefinition fix*/
	#ifndef inline
	#define inline __inline
    #endif
    /* Debugging helpers*/
    #pragma pack(push,8)
    struct THREADNAME_INFO
    {
        KDuint32    type;       // must be 0x1000
        const KDchar* name;       // pointer to name (in user addr space)
        KDuint32    threadid;   // thread ID (-1=caller thread)
        KDuint32    flags;      // reserved for future use, must be zero
    };
    #pragma pack(pop)
#endif

/******************************************************************************
* Middleware
******************************************************************************/

#ifdef KD_VFS_SUPPORTED
    #include <physfs.h>
#endif

/******************************************************************************
 * Errors
 ******************************************************************************/
static KD_THREADLOCAL KDint lasterror = 0;
 /* kdGetError: Get last error indication. */
KD_API KDint KD_APIENTRY kdGetError(void)
{
    return lasterror;
}

/* kdSetError: Set last error indication. */
KD_API void KD_APIENTRY kdSetError(KDint error)
{
    lasterror = error;
}

/******************************************************************************
 * Versioning and attribute queries
 ******************************************************************************/

/* kdQueryAttribi: Obtain the value of a numeric OpenKODE Core attribute. */
KD_API KDint KD_APIENTRY kdQueryAttribi(KD_UNUSED KDint attribute, KD_UNUSED KDint *value)
{
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdQueryAttribcv: Obtain the value of a string OpenKODE Core attribute. */
KD_API const KDchar *KD_APIENTRY kdQueryAttribcv(KDint attribute)
{
    if(attribute == KD_ATTRIB_VENDOR)
    {
        return "libKD (zlib license)";
    }
    else if(attribute == KD_ATTRIB_VERSION)
    {
        return "1.0.3 (libKD 0.1.0)";
    }
    else if(attribute == KD_ATTRIB_PLATFORM)
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
        return "Windows";
#elif defined(__unix__) || defined(__APPLE__)
        static struct utsname name;
        uname(&name);
        return name.sysname;
#endif
    }
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/* kdQueryIndexedAttribcv: Obtain the value of an indexed string OpenKODE Core attribute. */
KD_API const KDchar *KD_APIENTRY kdQueryIndexedAttribcv(KD_UNUSED KDint attribute, KD_UNUSED KDint __index)
{
    /* Some C implementations define an index function in string.h which leads to errors with GCC 4.6 and -Wshadow */
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/******************************************************************************
 * Threads and synchronization
 ******************************************************************************/

/* kdThreadAttrCreate: Create a thread attribute object. */
struct KDThreadAttr
{
#if defined(KD_THREAD_POSIX)
    pthread_attr_t nativeattr;
#endif
    KDint detachstate;
    KDsize stacksize;
    KDchar debugname[256];
};
KD_API KDThreadAttr *KD_APIENTRY kdThreadAttrCreate(void)
{
    KDThreadAttr *attr = (KDThreadAttr*)kdMalloc(sizeof(KDThreadAttr));
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
    attr->detachstate = detachstate;
#if defined(KD_THREAD_POSIX)
    if(detachstate == KD_THREAD_CREATE_JOINABLE)
    {
        pthread_attr_setdetachstate(&attr->nativeattr, PTHREAD_CREATE_JOINABLE);
    }
    else if(detachstate == KD_THREAD_CREATE_DETACHED)
    {
        pthread_attr_setdetachstate(&attr->nativeattr, PTHREAD_CREATE_DETACHED);
    }
    else
    {
        kdAssert(0);
    }
#endif
    return 0;
}


/* kdThreadAttrSetStackSize: Set stacksize attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetStackSize(KDThreadAttr *attr, KDsize stacksize)
{
    attr->stacksize = stacksize;
#if defined(KD_THREAD_POSIX)
    pthread_attr_setstacksize(&attr->nativeattr, attr->stacksize);
#endif
    return 0;
}

/* __kdThreadAttrSetDebugName: Set debugname attribute. */
KD_UNUSED static KDint __kdThreadAttrSetDebugName(KDThreadAttr *attr, const char * debugname)
{
    kdStrcpy_s(attr->debugname, 256, debugname);
    return 0;
}

/* kdThreadCreate: Create a new thread. */
static KD_THREADLOCAL KDThread *__kd_thread = KD_NULL;
typedef struct
{
    void *(*start_routine)(void *);
    void *arg;
    KDThread *thread;
} __KDThreadStartArgs;

struct KDThread
{
#if defined(KD_THREAD_C11)
    thrd_t nativethread;
#elif defined(KD_THREAD_POSIX)
    pthread_t nativethread;
#elif defined(KD_THREAD_WIN32)
    HANDLE nativethread;
#endif
    KDQueueVEN *eventqueue;
    const KDThreadAttr *attr;
    __KDThreadStartArgs *start_args;
};

static KD_THREADLOCAL KDEvent *__kd_lastevent = KD_NULL;
static void* __kdThreadStart(void *args)
{
    __KDThreadStartArgs *start_args = (__KDThreadStartArgs *) args;

    /* Set the thread name */
    KD_UNUSED const char* threadname = start_args->thread->attr ? start_args->thread->attr->debugname : "KDThread";
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4204)
#pragma warning(disable:6312)
#pragma warning(disable:6322)
    /* https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx */
    struct THREADNAME_INFO info =
    {
        .type = 0x1000 ,
        .name = threadname,
        .threadid = GetCurrentThreadId(),
        .flags = 0
    };
    if(IsDebuggerPresent())
    {
        __try
        {
            RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
#pragma warning(pop)
    }
#elif defined(KD_THREAD_POSIX)
#if defined(__linux__)
    prctl(PR_SET_NAME, (unsigned long)threadname, 0UL, 0UL, 0UL);
#elif defined(__APPLE__)
    pthread_setname_np(threadname);
#endif
#endif

    /* We have to clean up start_args somehow */
    start_args->thread->start_args = start_args;

    __kd_thread = start_args->thread;

    void* result = KD_NULL;

    result = start_args->start_routine(start_args->arg);

    if(__kd_lastevent != KD_NULL)
    {
        kdFreeEvent(__kd_lastevent);
        __kd_lastevent = KD_NULL;
    }
    return result;
}

KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
    KDThread *thread = (KDThread *)kdMalloc(sizeof(KDThread));
    thread->eventqueue = kdQueueCreateVEN(100);
    thread->attr = attr;

    __KDThreadStartArgs *start_args = (__KDThreadStartArgs*)kdMalloc(sizeof(__KDThreadStartArgs));
    start_args->start_routine = start_routine;
    start_args->arg = arg;
    start_args->thread = thread;

    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = thrd_create(&thread->nativethread, (thrd_start_t)__kdThreadStart, start_args);
#elif defined(KD_THREAD_POSIX)
    error = pthread_create(&thread->nativethread, attr ? &attr->nativeattr : KD_NULL, __kdThreadStart, start_args);
#elif defined(KD_THREAD_WIN32)
    thread->nativethread = CreateThread(KD_NULL, attr ? attr->stacksize : 0, (LPTHREAD_START_ROUTINE)__kdThreadStart, (LPVOID)start_args, 0, KD_NULL);
    error = thread->nativethread ? 0 : 1;
#else
	kdAssert(0);
#endif

    if(error != 0)
    {
        kdSetError(KD_EAGAIN);
        kdQueueFreeVEN(thread->eventqueue);
        kdFree(thread);
        return KD_NULL;
    }

    if(attr != KD_NULL && attr->detachstate == KD_THREAD_CREATE_DETACHED)
    {
        kdThreadDetach(thread);
        kdQueueFreeVEN(thread->eventqueue);
        kdFree(thread);
        return KD_NULL;
    }

    return thread;
}

/* kdThreadExit: Terminate this thread. */
KD_API KD_NORETURN void KD_APIENTRY kdThreadExit(void *retval)
{
    KD_UNUSED KDint result = 0;
    if(retval != KD_NULL)
    {
        result = *(KDint*)retval;
    }
#if defined(KD_THREAD_C11)
    thrd_exit(result);
#elif defined(KD_THREAD_POSIX)
    pthread_exit(retval);
#elif defined(KD_THREAD_WIN32)
    ExitThread(result);
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
    while(1);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

/* kdThreadJoin: Wait for termination of another thread. */
KD_API KDint KD_APIENTRY kdThreadJoin(KDThread *thread, void **retval)
{
    KDint error = 0;
    KDint resinit = 0;
    KD_UNUSED KDint* result = &resinit;
    if(retval != KD_NULL)
    {
        result = *retval;
    }
#if defined(KD_THREAD_C11)
    error = thrd_join(thread->nativethread, result);
    if(error == thrd_error)
#elif defined(KD_THREAD_POSIX)
    error = pthread_join(thread->nativethread, retval);
    if(error == EINVAL || error == ESRCH)
#elif defined(KD_THREAD_WIN32)
    error = WaitForSingleObject(thread->nativethread, INFINITE);
    GetExitCodeThread(thread->nativethread, (LPDWORD)result);
    CloseHandle(thread->nativethread);
    if(error != 0)
#else
    kdAssert(0);
#endif
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
    kdQueueFreeVEN(thread->eventqueue);
    kdFree(thread->start_args);
    kdFree(thread);
    return 0;
}

/* kdThreadDetach: Allow resources to be freed as soon as a thread terminates. */
KD_API KDint KD_APIENTRY kdThreadDetach(KDThread *thread)
{
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = thrd_detach(thread->nativethread);
#elif defined(KD_THREAD_POSIX)
    KDint detachstate = 0;
    error = pthread_attr_getdetachstate(&thread->attr->nativeattr, &detachstate);
    /* Already detached */
    if(error == 0 && detachstate == PTHREAD_CREATE_DETACHED)
    {
        error = pthread_detach(thread->nativethread);
    }
#elif defined(KD_THREAD_WIN32)
    CloseHandle(thread->nativethread);
#else
    kdAssert(0);
#endif
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
    return __kd_thread;
}

/* kdThreadOnce: Wrap initialization code so it is executed only once. */
#ifndef KD_NO_STATIC_DATA

KD_API KDint KD_APIENTRY kdThreadOnce(KDThreadOnce *once_control, void (*init_routine)(void))
{
#if defined(KD_THREAD_C11)
    call_once((once_flag *)once_control, init_routine);
#elif defined(KD_THREAD_POSIX)
    pthread_once((pthread_once_t *)once_control, init_routine);
#elif defined(KD_THREAD_WIN32)
    void* pfunc = KD_NULL;
    kdMemcpy(&pfunc, &init_routine, sizeof(init_routine));
    InitOnceExecuteOnce((PINIT_ONCE)once_control, call_once_callback, pfunc, NULL);
#else
    kdAssert(0);
#endif
    return 0;
}
#endif /* ndef KD_NO_STATIC_DATA */

/* kdThreadMutexCreate: Create a mutex. */
struct KDThreadMutex
{
#if defined(KD_THREAD_C11)
    mtx_t nativemutex;
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_t nativemutex;
#elif defined(KD_THREAD_WIN32)
    SRWLOCK nativemutex;
#endif
};
KD_API KDThreadMutex *KD_APIENTRY kdThreadMutexCreate(KD_UNUSED const void *mutexattr)
{
    /* TODO: Write KDThreadMutexAttr extension */
    KDThreadMutex *mutex = (KDThreadMutex*)kdMalloc(sizeof(KDThreadMutex));
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = mtx_init(&mutex->nativemutex, mtx_plain);
#elif defined(KD_THREAD_POSIX)
    error = pthread_mutex_init(&mutex->nativemutex, KD_NULL);
    if(error == ENOMEM)
    {
        kdSetError(KD_ENOMEM);
        kdFree(mutex);
        return KD_NULL;
    }
#elif defined(KD_THREAD_WIN32)
    InitializeSRWLock(&mutex->nativemutex);
#else
    kdAssert(0);
#endif
    if(error != 0)
    {
        kdSetError(KD_EAGAIN);
        kdFree(mutex);
        return KD_NULL;
    }
    return mutex;
}

/* kdThreadMutexFree: Free a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexFree(KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    mtx_destroy(&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_destroy(&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    // No need to free anything
#else
    kdAssert(0);
#endif
    kdFree(mutex);
    return 0;
}

/* kdThreadMutexLock: Lock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexLock(KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    mtx_lock(&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_lock(&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    AcquireSRWLockExclusive(&mutex->nativemutex);
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadMutexUnlock: Unlock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexUnlock(KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    mtx_unlock(&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_unlock(&mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    ReleaseSRWLockExclusive(&mutex->nativemutex);
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadCondCreate: Create a condition variable. */
struct KDThreadCond
{
#if defined(KD_THREAD_C11)
    cnd_t nativecond;
#elif defined(KD_THREAD_POSIX)
    pthread_cond_t nativecond;
#elif defined(KD_THREAD_WIN32)
    CONDITION_VARIABLE nativecond;
#endif
};
KD_API KDThreadCond *KD_APIENTRY kdThreadCondCreate(KD_UNUSED const void *attr)
{

    KDThreadCond *cond = (KDThreadCond*)kdMalloc(sizeof(KDThreadCond));
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error =  cnd_init(&cond->nativecond);
    if(error ==  thrd_nomem)
    {
        kdSetError(KD_ENOMEM);
        kdFree(cond);
        return KD_NULL;
    }
#elif defined(KD_THREAD_POSIX)
    error = pthread_cond_init(&cond->nativecond, KD_NULL);
#elif defined(KD_THREAD_WIN32)
    InitializeConditionVariable(&cond->nativecond);
#else
    kdAssert(0);
#endif
    if(error != 0)
    {
        kdSetError(KD_EAGAIN);
        kdFree(cond);
        return KD_NULL;
    }
    return cond;
}

/* kdThreadCondFree: Free a condition variable. */
KD_API KDint KD_APIENTRY kdThreadCondFree(KDThreadCond *cond)
{
#if defined(KD_THREAD_C11)
    cnd_destroy(&cond->nativecond);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_destroy(&cond->nativecond);
#elif defined(KD_THREAD_WIN32)
    // No need to free anything
#else
    kdAssert(0);
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
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadCondWait: Wait for a condition variable to be signalled. */
KD_API KDint KD_APIENTRY kdThreadCondWait(KDThreadCond *cond, KDThreadMutex *mutex)
{
#if defined(KD_THREAD_C11)
    cnd_wait(&cond->nativecond, &mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
    pthread_cond_wait(&cond->nativecond, &mutex->nativemutex);
#elif defined(KD_THREAD_WIN32)
    SleepConditionVariableSRW(&cond->nativecond, &mutex->nativemutex, INFINITE, 0);
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadSemCreate: Create a semaphore. */
struct KDThreadSem
{
    KDuint          count;
    KDThreadMutex*   mutex;
    KDThreadCond*    condition;
};
KD_API KDThreadSem *KD_APIENTRY kdThreadSemCreate(KDuint value)
{
    KDThreadSem *sem = (KDThreadSem*)kdMalloc(sizeof(KDThreadSem));
    sem->count = value;
    sem->mutex = kdThreadMutexCreate(KD_NULL);
    sem->condition = kdThreadCondCreate(KD_NULL);
    return sem;
}

/* kdThreadSemFree: Free a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemFree(KDThreadSem *sem)
{
    kdThreadMutexFree(sem->mutex);
    kdThreadCondFree(sem->condition);
    kdFree(sem);
    return 0;
}

/* kdThreadSemWait: Lock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemWait(KDThreadSem *sem)
{
    kdThreadMutexLock(sem->mutex);
    while(sem->count == 0)
    {
        kdThreadCondWait(sem->condition, sem->mutex);
    }
    --sem->count;
    kdThreadMutexUnlock(sem->mutex);
    return 0;
}

/* kdThreadSemPost: Unlock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemPost(KDThreadSem *sem)
{
    kdThreadMutexLock(sem->mutex);
    ++sem->count;
    kdThreadCondSignal(sem->condition);
    kdThreadMutexUnlock(sem->mutex);
    return 0;
}

/******************************************************************************
 * Events
 ******************************************************************************/
/* __KDSleep: Sleep for nanoseconds. */
void __KDSleep(KDust timeout)
{
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define LONG_CAST (long)
#else
#define LONG_CAST
#endif
    struct timespec ts = {0};
    /* Determine seconds from the overall nanoseconds */
    if((timeout % 1000000000) == 0)
    {
        ts.tv_sec = LONG_CAST timeout / 1000000000;
    }
    else
    {
        ts.tv_sec = LONG_CAST (timeout - (timeout % 1000000000)) / 1000000000;
    }
#undef LONG_CAST
    /* Remaining nanoseconds */
    ts.tv_nsec = (KDint32)timeout - ((KDint32)ts.tv_sec * 1000000000);

#if defined(KD_THREAD_C11)
    thrd_sleep(&ts, NULL);
#elif defined(KD_THREAD_POSIX)
#ifdef __EMSCRIPTEN__
    emscripten_sleep_with_yield(timeout / 1000000);
#else
    nanosleep(&ts, NULL);
#endif
#elif defined(KD_THREAD_WIN32)
    HANDLE timer = CreateWaitableTimer(KD_NULL, 1, KD_NULL);
    if(!timer) { kdAssert(0); }
    LARGE_INTEGER li = {{0}};
    li.QuadPart = -(timeout/100);
    if(!SetWaitableTimer(timer, &li, 0, KD_NULL, KD_NULL, 0)) { kdAssert(0); }
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    kdAssert(0);
#endif
}

/* kdWaitEvent: Get next event from thread's event queue. */
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    if(__kd_lastevent != KD_NULL)
    {
        kdFreeEvent(__kd_lastevent);
        __kd_lastevent = KD_NULL;
    }
    if(timeout != -1)
    {
        __KDSleep(timeout);
    }
    kdPumpEvents();
    if(kdQueueSizeVEN(kdThreadSelf()->eventqueue) > 0)
    {
        __kd_lastevent = (KDEvent *)kdQueuePopHeadVEN(kdThreadSelf()->eventqueue);
    }
    return __kd_lastevent;
}
/* kdSetEventUserptr: Set the userptr for global events. */
KD_API void KD_APIENTRY kdSetEventUserptr(KD_UNUSED void *userptr)
{
}

/* kdDefaultEvent: Perform default processing on an unrecognized event. */
KD_API void KD_APIENTRY kdDefaultEvent(const KDEvent *event)
{
    if(event)
    {
        if(event->type == KD_EVENT_QUIT)
        {
            kdThreadExit(KD_NULL);
        }
    }
}

/* kdPumpEvents: Pump the thread's event queue, performing callbacks. */
typedef struct
{
    KDCallbackFunc *func;
    KDint eventtype;
    void *eventuserptr;
} __KDCallback;
static KD_THREADLOCAL KDuint __kd_callbacks_index = 0;
static KD_THREADLOCAL __KDCallback __kd_callbacks[999] = {{0}};
static KDboolean __kdExecCallback(KDEvent* event)
{
    for (KDuint callback = 0; callback < __kd_callbacks_index; callback++)
    {
        if(__kd_callbacks[callback].func != KD_NULL)
        {
            KDboolean typematch = __kd_callbacks[callback].eventtype == event->type || __kd_callbacks[callback].eventtype == 0;
            KDboolean userptrmatch = __kd_callbacks[callback].eventuserptr == event->userptr;
            if(typematch && userptrmatch)
            {
                __kd_callbacks[callback].func(event);
                kdFreeEvent(event);
                return 1;
            }
        }
    }
    return 0;
}

#if defined(KD_WINDOW_SUPPORTED)
struct KDWindow
{
#if defined(KD_WINDOW_NULL)
    KDint nativewindow;
    void *nativedisplay;
#elif defined(KD_WINDOW_ANDROID)
    struct ANativeWindow *nativewindow;
    void *nativedisplay;
#elif defined(KD_WINDOW_WIN32)
    HWND nativewindow;
#elif defined(KD_WINDOW_X11)
    Window nativewindow;
    Display *nativedisplay;
#endif
    EGLint format;
};
#if defined(KD_WINDOW_ANDROID)
static AInputQueue *__kd_androidinputqueue = KD_NULL;
static KDThreadMutex *__kd_androidinputqueue_mutex = KD_NULL;
#endif
static KDWindow *__kd_window = KD_NULL;
#endif
KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
#ifdef __EMSCRIPTEN__
    /* Give back control to the browser */
    emscripten_sleep_with_yield(1);
#endif
    KDsize queuesize = kdQueueSizeVEN(kdThreadSelf()->eventqueue);
    for (KDuint i = 0; i < queuesize; i++)
    {
        KDEvent *callbackevent = kdQueuePopHeadVEN(kdThreadSelf()->eventqueue);
        if(callbackevent != KD_NULL)
        {
            if(!__kdExecCallback(callbackevent))
            {
                /* Not a callback */
                kdPostEvent(callbackevent);
            }
        }
    }
#if defined(KD_WINDOW_SUPPORTED)
#if defined(KD_WINDOW_ANDROID)
    AInputEvent* aevent = NULL;
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    if(__kd_androidinputqueue != KD_NULL)
    {
        while(AInputQueue_getEvent(__kd_androidinputqueue, &aevent) >= 0)
        {
            AInputQueue_preDispatchEvent(__kd_androidinputqueue, aevent);
            KDEvent *event = kdCreateEvent();
            switch(AInputEvent_getType(aevent))
            {
                case(AINPUT_EVENT_TYPE_KEY):
                {
                    switch(AKeyEvent_getKeyCode(aevent))
                    {
                        case(AKEYCODE_BACK):
                        default:
                        {
                            kdFreeEvent(event);
                            break;
                        }
                    }
                    break;
                }
                default:
                {
                    kdFreeEvent(event);
                    break;
                }
            }
            AInputQueue_finishEvent(__kd_androidinputqueue, aevent, 1);
        }
    }
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
#elif defined(KD_WINDOW_WIN32)
    if(__kd_window)
    {
        MSG msg = { 0 };
        while(PeekMessage(&msg, __kd_window->nativewindow, 0, 0, PM_REMOVE) != 0)
        {
            KDEvent *event = kdCreateEvent();
            switch (msg.message)
            {
                case WM_CLOSE:
                case WM_DESTROY:
                case WM_QUIT:
                {
                    ShowWindow(__kd_window->nativewindow, SW_HIDE);
                    event->type = KD_EVENT_QUIT;
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case WM_INPUT:
                {
                    KDchar buffer[sizeof(RAWINPUT)] = {0};
                    KDsize size = sizeof(RAWINPUT);
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, buffer, (PUINT)&size, sizeof(RAWINPUTHEADER));
                    RAWINPUT* raw = (RAWINPUT*)buffer;
                    if (raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN ||
                           raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN ||
                           raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN)
                        {
                            event->type                     = KD_EVENT_INPUT_POINTER;
                            event->data.inputpointer.index  = KD_INPUT_POINTER_SELECT;
                            event->data.inputpointer.select = 1;
                            event->data.inputpointer.x      = raw->data.mouse.lLastX;
                            event->data.inputpointer.y      = raw->data.mouse.lLastY;
                            if(!__kdExecCallback(event))
                            {
                                kdPostEvent(event);
                            }
                        }
                        else if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP ||
                           raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP ||
                           raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP)
                        {
                            event->type                     = KD_EVENT_INPUT_POINTER;
                            event->data.inputpointer.index  = KD_INPUT_POINTER_SELECT;
                            event->data.inputpointer.select = 0;
                            event->data.inputpointer.x      = raw->data.mouse.lLastX;
                            event->data.inputpointer.y      = raw->data.mouse.lLastY;
                            if(!__kdExecCallback(event))
                            {
                                kdPostEvent(event);
                            }
                        }
                        else if(raw->data.keyboard.Flags & MOUSE_MOVE_ABSOLUTE)
                        {
                            event->type                     = KD_EVENT_INPUT_POINTER;
                            event->data.inputpointer.index  = KD_INPUT_POINTER_X;
                            event->data.inputpointer.x      = raw->data.mouse.lLastX;
                            if(!__kdExecCallback(event))
                            {
                                kdPostEvent(event);
                            }
                            KDEvent *event2 = kdCreateEvent();
                            event2->type                     = KD_EVENT_INPUT_POINTER;
                            event2->data.inputpointer.index  = KD_INPUT_POINTER_Y;
                            event2->data.inputpointer.y      = raw->data.mouse.lLastY;
                            if(!__kdExecCallback(event2))
                            {
                                kdPostEvent(event2);
                            }
                        }
                        break;
                    }
                    else if (raw->header.dwType == RIM_TYPEKEYBOARD)
                    {
                        event->type = KD_EVENT_INPUT_KEY_VEN;
                        KDEventInputKeyVEN *keyevent = (KDEventInputKeyVEN *)(&event->data);

                        /* Press or release */
                        #if defined (_MSC_VER)
                        #pragma warning(push)
                        #pragma warning(disable:6313)
                        #endif
                        if(raw->data.keyboard.Flags & RI_KEY_MAKE)
                        #if defined (_MSC_VER)
                        #pragma warning(pop)
                        #endif
                        {
                            keyevent->flags = KD_KEY_PRESS_VEN;
                        }
                        else
                        {
                            keyevent->flags = 0;
                        }

                        switch (raw->data.keyboard.VKey)
                        {
                            case(VK_UP):
                            {
                                keyevent->keycode = KD_KEY_UP_VEN;
                                break;
                            }
                            case(VK_DOWN):
                            {
                                keyevent->keycode = KD_KEY_DOWN_VEN;
                                break;
                            }
                            case(VK_LEFT):
                            {
                                keyevent->keycode = KD_KEY_LEFT_VEN;
                                break;
                            }
                            case(VK_RIGHT):
                            {
                                keyevent->keycode = KD_KEY_RIGHT_VEN;
                                break;
                            }
                            default:
                            {
                                event->type = KD_EVENT_INPUT_KEYCHAR_VEN;
                                KDEventInputKeyCharVEN *keycharevent = (KDEventInputKeyCharVEN *) (&event->data);
                                GetKeyNameText((long)MapVirtualKey(raw->data.keyboard.VKey, MAPVK_VK_TO_VSC) << 16, (char*)&keycharevent->character, sizeof(KDint32));
                                break;
                            }
                        }
                    }
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                default:
                {
                    kdFreeEvent(event);
                    DispatchMessage(&msg);
                    break;
                }
            }
        }
    }
#elif defined(KD_WINDOW_X11)
    if(__kd_window)
    {
        XSelectInput(__kd_window->nativedisplay, __kd_window->nativewindow, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
        while(XPending(__kd_window->nativedisplay) > 0)
        {
            KDEvent *event = kdCreateEvent();
            XEvent xevent = {0};
            XNextEvent(__kd_window->nativedisplay,&xevent);
            switch(xevent.type)
            {
                case ButtonPress:
                {
                    event->type                     = KD_EVENT_INPUT_POINTER;
                    event->data.inputpointer.index  = KD_INPUT_POINTER_SELECT;
                    event->data.inputpointer.select = 1;
                    event->data.inputpointer.x      = xevent.xbutton.x;
                    event->data.inputpointer.y      = xevent.xbutton.y;
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case ButtonRelease:
                {
                    event->type                     = KD_EVENT_INPUT_POINTER;
                    event->data.inputpointer.index  = KD_INPUT_POINTER_SELECT;
                    event->data.inputpointer.select = 0;
                    event->data.inputpointer.x      = xevent.xbutton.x;
                    event->data.inputpointer.y      = xevent.xbutton.y;
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case KeyRelease:
                case KeyPress:
                {
                    KeySym keysym;
                    XLookupString(&xevent.xkey, NULL, 25, &keysym, NULL);
                    event->type = KD_EVENT_INPUT_KEY_VEN;
                    KDEventInputKeyVEN *keyevent = (KDEventInputKeyVEN *)(&event->data);

                    /* Press or release */
                    if(xevent.type == KeyPress)
                    {
                        keyevent->flags = KD_KEY_PRESS_VEN;
                    }
                    else
                    {
                        keyevent->flags = 0;
                    }

                    switch(keysym)
                    {
                        case(XK_Up):
                        {

                            keyevent->keycode = KD_KEY_UP_VEN;
                            break;
                        }
                        case(XK_Down):
                        {

                            keyevent->keycode = KD_KEY_DOWN_VEN;
                            break;
                        }
                        case(XK_Left):
                        {

                            keyevent->keycode = KD_KEY_LEFT_VEN;
                            break;
                        }
                        case(XK_Right):
                        {

                            keyevent->keycode = KD_KEY_RIGHT_VEN;
                            break;
                        }
                        default:
                        {
                            event->type = KD_EVENT_INPUT_KEYCHAR_VEN;
                            KDEventInputKeyCharVEN *keycharevent = (KDEventInputKeyCharVEN *) (&event->data);
                            keycharevent->character = (KDint32) keysym;
                            break;
                        }
                    }
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case MotionNotify:
                {
                    event->type                     = KD_EVENT_INPUT_POINTER;
                    event->data.inputpointer.index  = KD_INPUT_POINTER_X;
                    event->data.inputpointer.x      = xevent.xmotion.x;
                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    KDEvent *event2 = kdCreateEvent();
                    event2->type                     = KD_EVENT_INPUT_POINTER;
                    event2->data.inputpointer.index  = KD_INPUT_POINTER_Y;
                    event2->data.inputpointer.y      = xevent.xmotion.y;
                    if(!__kdExecCallback(event2))
                    {
                        kdPostEvent(event2);
                    }
                    break;
                }
                case ConfigureNotify:
                {
                    event->type      = KD_EVENT_WINDOWPROPERTY_CHANGE;

                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case ClientMessage:
                {
                    if((Atom)xevent.xclient.data.l[0] == XInternAtom(__kd_window->nativedisplay, "WM_DELETE_WINDOW", False))
                    {
                        event->type      = KD_EVENT_QUIT;
                        if(!__kdExecCallback(event))
                        {
                            kdPostEvent(event);
                        }
                        break;
                    }
                }
                case MappingNotify:
                {
                    XRefreshKeyboardMapping((XMappingEvent*)&xevent);
                    break;
                }
                default:
                {
                    kdFreeEvent(event);
                    break;
                }
            }
        }
    }
#endif
#endif
    return 0;
}

/* kdInstallCallback: Install or remove a callback function for event processing. */
KD_API KDint KD_APIENTRY kdInstallCallback(KDCallbackFunc *func, KDint eventtype, void *eventuserptr)
{
    for(KDuint callback = 0; callback < __kd_callbacks_index; callback++)
    {
        KDboolean typematch = __kd_callbacks[callback].eventtype == eventtype || __kd_callbacks[callback].eventtype == 0;
        KDboolean userptrmatch = __kd_callbacks[callback].eventuserptr == eventuserptr;
        if(typematch && userptrmatch)
        {
            __kd_callbacks[callback].func = func;
            return 0;
        }
    }
    __kd_callbacks[__kd_callbacks_index].func           = func;
    __kd_callbacks[__kd_callbacks_index].eventtype      = eventtype;
    __kd_callbacks[__kd_callbacks_index].eventuserptr   = eventuserptr;
    __kd_callbacks_index++;
    return 0;
}

/* kdCreateEvent: Create an event for posting. */
KD_API KDEvent *KD_APIENTRY kdCreateEvent(void)
{
    KDEvent *event = (KDEvent*)kdMalloc(sizeof(KDEvent));
    event->timestamp = 0;
    event->type = -1;
    event->userptr = KD_NULL;
    return event;
}

/* kdPostEvent, kdPostThreadEvent: Post an event into a queue. */
KD_API KDint KD_APIENTRY kdPostEvent(KDEvent *event)
{
    return kdPostThreadEvent(event, kdThreadSelf());
}
KD_API KDint KD_APIENTRY kdPostThreadEvent(KDEvent *event, KDThread *thread)
{
    if(event->timestamp == 0)
    {
        event->timestamp = kdGetTimeUST();
    }
    kdQueuePushTailVEN(thread->eventqueue, (void*)event);
    return 0;
}

/* kdFreeEvent: Abandon an event instead of posting it. */
KD_API void KD_APIENTRY kdFreeEvent(KDEvent *event)
{
    kdFree(event);
    event = KD_NULL;
}

/******************************************************************************
 * System events
 ******************************************************************************/
/* Header only */

/******************************************************************************
 * Application startup and exit.
 ******************************************************************************/
extern const char *__progname;
static KDThread *__kd_mainthread = KD_NULL;
const char* __kdAppName(KD_UNUSED const char *argv0)
{
#ifdef __GLIBC__
    return __progname;
#else
    /* TODO: argv[0] is not a reliable way to get the appname */
    if(argv0 == KD_NULL)
    {
        return "";
    }
    return argv0;
#endif
}

#ifdef __ANDROID__
/* All Android events are send to the mainthread */
static ANativeActivity *__kd_androidactivity =  KD_NULL;
static KDThreadMutex *__kd_androidactivity_mutex =  KD_NULL;
static void __kd_AndroidOnDestroy(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_QUIT;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnStart(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnResume(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void* __kd_AndroidOnSaveInstanceState(ANativeActivity *activity, size_t *len)
{
    /* TODO: Save state */
    return KD_NULL;
}

static void __kd_AndroidOnPause(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_PAUSE;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnStop(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_PAUSE;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnConfigurationChanged(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_ORIENTATION;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnLowMemory(ANativeActivity *activity)
{
    /* TODO: Avoid getting killed by Android */
}

static void __kd_AndroidOnWindowFocusChanged(ANativeActivity *activity, int focused)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_FOCUS;
    event->data.windowfocus.focusstate = focused;
    kdPostThreadEvent(event, __kd_mainthread);
}

static ANativeWindow *__kd_androidwindow =  KD_NULL;
static KDThreadMutex *__kd_androidwindow_mutex =  KD_NULL;
static void __kd_AndroidOnNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = window;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_REDRAW;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = KD_NULL;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_CLOSE;
    kdPostThreadEvent(event, __kd_mainthread);
}

static void __kd_AndroidOnInputQueueCreated(ANativeActivity *activity, AInputQueue *queue)
{
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    __kd_androidinputqueue = queue;
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
}

static void __kd_AndroidOnInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue)
{
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    __kd_androidinputqueue = KD_NULL;
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
}
#endif

typedef struct
{
    int argc;
    char **argv;
} __KDMainArgs;
static void* __kdMainInjector( void *arg)
{
    __KDMainArgs *mainargs = (__KDMainArgs *) arg;
    if(mainargs == KD_NULL)
    {
        kdAssert(0);
    }
#ifdef KD_VFS_SUPPORTED
    struct PHYSFS_Allocator allocator= {0};
    allocator.Deinit = KD_NULL;
    allocator.Free = kdFree;
    allocator.Init = KD_NULL;
    allocator.Malloc = (void *(*)(PHYSFS_uint64))kdMalloc;
    allocator.Realloc = (void *(*)(void*, PHYSFS_uint64))kdRealloc;
    PHYSFS_setAllocator(&allocator);
    PHYSFS_init(mainargs->argv[0]);
    const KDchar *prefdir = PHYSFS_getPrefDir("libKD", __kdAppName(mainargs->argv[0]));
    PHYSFS_setWriteDir(prefdir);
    PHYSFS_mount(prefdir, "/", 0);
    PHYSFS_mkdir("/res");
    PHYSFS_mkdir("/data");
    PHYSFS_mkdir("/tmp");
#endif

    __kd_mainthread = __kd_thread;

#ifdef __ANDROID__
    kdMain(mainargs->argc, (const KDchar *const *)mainargs->argv);
    kdThreadMutexFree(__kd_androidactivity_mutex);
    kdThreadMutexFree(__kd_androidwindow_mutex);
    kdThreadMutexFree(__kd_androidinputqueue_mutex);
#elif __EMSCRIPTEN__
    kdMain(mainargs->argc, (const KDchar *const *)mainargs->argv);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    HMODULE handle = GetModuleHandle(0);
    typedef KDint(KD_APIENTRY *KDMAIN)(KDint argc, const KDchar *const *argv);
    KDMAIN kdmain = (KDMAIN)GetProcAddress(handle, "kdMain");
    (*kdmain)(mainargs->argc, (const KDchar *const *)mainargs->argv);
#else
    typedef KDint(KD_APIENTRY *KDMAIN)(KDint argc, const KDchar *const *argv);
    KDMAIN kdmain = KD_NULL;
    void *app = dlopen(NULL, RTLD_NOW);
    /* ISO C forbids assignment between function pointer and ‘void *’ */
    void *rawptr = dlsym(app, "kdMain");
    kdMemcpy(&kdmain, &rawptr, sizeof(rawptr));
    if(dlerror() != NULL)
    {
        kdLogMessage("Cant dlopen self. Dont strip symbols from me.\n");
        kdAssert(0);
    }
    (*kdmain)(mainargs->argc, (const KDchar *const *)mainargs->argv);
    dlclose(app);
#endif

#ifdef KD_VFS_SUPPORTED
    PHYSFS_deinit();
#endif
    return 0;
}

#ifdef __ANDROID__
void ANativeActivity_onCreate(ANativeActivity *activity, void* savedState, size_t savedStateSize)
{
    __kd_androidwindow_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_androidinputqueue_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_androidactivity_mutex = kdThreadMutexCreate(KD_NULL);

    activity->callbacks->onDestroy = __kd_AndroidOnDestroy;
    activity->callbacks->onStart = __kd_AndroidOnStart;
    activity->callbacks->onResume = __kd_AndroidOnResume;
    activity->callbacks->onSaveInstanceState = __kd_AndroidOnSaveInstanceState;
    activity->callbacks->onPause = __kd_AndroidOnPause;
    activity->callbacks->onStop = __kd_AndroidOnStop;
    activity->callbacks->onConfigurationChanged = __kd_AndroidOnConfigurationChanged;
    activity->callbacks->onLowMemory = __kd_AndroidOnLowMemory;
    activity->callbacks->onWindowFocusChanged = __kd_AndroidOnWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = __kd_AndroidOnNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = __kd_AndroidOnNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = __kd_AndroidOnInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = __kd_AndroidOnInputQueueDestroyed;

    kdThreadMutexLock(__kd_androidactivity_mutex);
    __kd_androidactivity = activity;
    ANativeActivity_setWindowFlags(__kd_androidactivity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
    kdThreadMutexUnlock(__kd_androidactivity_mutex);

    __KDMainArgs mainargs = {0};
    mainargs.argc = 0;
    mainargs.argv = KD_NULL;
    KDThread *thread = kdThreadCreate(KD_NULL, __kdMainInjector, &mainargs);
    kdThreadDetach(thread);
}
#else
KD_API int main(int argc, char **argv)
{
    __KDMainArgs mainargs = {0};
    mainargs.argc = argc;
    mainargs.argv = argv;

#ifdef __EMSCRIPTEN__
    __kdMainInjector(&mainargs);
#else
    KDThreadAttr *mainattr = kdThreadAttrCreate();
    __kdThreadAttrSetDebugName(mainattr, "KDThread (Main)");
    KDThread * mainthread = kdThreadCreate(mainattr, __kdMainInjector, &mainargs);
    kdThreadJoin(mainthread, KD_NULL);
    kdThreadAttrFree(mainattr);
#endif
    return 0;
}
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#if defined(KD_FREESTANDING)
int WINAPI mainCRTStartup(void)
{
    return main(0, KD_NULL);
}
int WINAPI WinMainCRTStartup(void)
{
    return main(0, KD_NULL);
}
#endif
#endif

/* kdExit: Exit the application. */
KD_API KD_NORETURN void KD_APIENTRY kdExit(KDint status)
{
    exit(status);
}

/******************************************************************************
 * Utility library functions
 ******************************************************************************/

/* kdAbs: Compute the absolute value of an integer. */
KD_API KDint KD_APIENTRY kdAbs(KDint i)
{
    return ( i >= 0 ) ? i : -i;
}

/* kdStrtof: Convert a string to a floating point number. */
#if defined(_MSC_VER) && _MSC_VER <= 1800
#pragma warning(push)
#pragma warning(disable:4756)
#endif
KD_API KDfloat32 KD_APIENTRY kdStrtof(const KDchar *s, KDchar **endptr)
{
    KDfloat32 retval = strtof(s, endptr);

    if(retval == HUGE_VALF)
    {
        kdSetError(KD_ERANGE);
        return copysignf(KD_HUGE_VALF, retval );
    }
    return retval;
}
#if defined(_MSC_VER) && _MSC_VER <= 1800
#pragma warning(pop)
#endif

/* kdStrtol, kdStrtoul: Convert a string to an integer. */
KD_API KDint KD_APIENTRY kdStrtol(const KDchar *s, KDchar **endptr, KDint base)
{
    KDint64 retval = strtoimax(s, endptr, base);
    if(retval ==  INTMAX_MIN)
    {
        kdSetError(KD_ERANGE);
        return  KDINT_MIN;
    }
    else if(retval ==  INTMAX_MAX)
    {
        kdSetError(KD_ERANGE);
        return  KDINT_MAX;
    }
    return (KDint)retval;
}

KD_API KDuint KD_APIENTRY kdStrtoul(const KDchar *s, KDchar **endptr, KDint base)
{
    KDuint64 retval = strtoumax(s, endptr, base);
    if(retval == UINTMAX_MAX)
    {
        kdSetError(KD_ERANGE);
        return  KDUINT_MAX;
    }
    return (KDuint)retval;
}

/* kdLtostr, kdUltostr: Convert an integer to a string. */
KD_API KDssize KD_APIENTRY kdLtostr(KDchar *buffer, KDsize buflen, KDint number)
{
    if(buflen == 0)
    {
        return -1;
    }
    return snprintf(buffer, buflen, "%i", number);
}
KD_API KDssize KD_APIENTRY kdUltostr(KDchar *buffer, KDsize buflen, KDuint number, KDint base)
{
    if(buflen == 0)
    {
        return -1;
    }
    if(base == 10 || base == 0 )
    {
        return snprintf(buffer, buflen, "%u", number);
    }
    else if(base == 8 )
    {
        return snprintf(buffer, buflen, "%o", number);
    }
    else  if(base == 16 )
    {
        return snprintf(buffer, buflen, "%x", number);
    } 
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdFtostr: Convert a float to a string. */
KD_API KDssize KD_APIENTRY kdFtostr(KDchar *buffer, KDsize buflen, KDfloat32 number)
{
    if(buflen == 0)
    {
        return -1;
    }
    return snprintf(buffer, buflen, "%f", number);
}

/* kdCryptoRandom: Return random data. */
KD_API KDint KD_APIENTRY kdCryptoRandom(KD_UNUSED KDuint8 *buf, KD_UNUSED KDsize buflen)
{
#if defined(_MSC_VER) && defined(_M_ARM)
    kdSetError(KD_EOPNOTSUPP);
    return -1;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    HCRYPTPROV provider = 0;
    KDboolean error = !CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    if(error == 0)
    {
        error = !CryptGenRandom(provider, (KDuint32)buflen, buf);
    }
    CryptReleaseContext(provider, 0);
    return error ? -1 : 0;
#elif defined(__OpenBSD__)
    return getentropy(buf, buflen);
#elif defined(__EMSCRIPTEN__)
    for(KDsize i = 0; i < buflen; i++)
    {
        buf[i] = (KDuint8)(emscripten_random() * 255) % 256;
    }
    return 0;
#elif defined(__unix__) || defined(__APPLE__)
    FILE *urandom = fopen("/dev/urandom", "r");
    KDsize result = fread((void *)buf, sizeof(KDuint8), buflen, urandom);
    fclose(urandom);
    if(result != buflen)
    {
        kdSetError(KD_ENOMEM);
        return -1;
    }
    return 0;
#endif
}

/******************************************************************************
 * Locale specific functions
 ******************************************************************************/

/* kdGetLocale: Determine the current language and locale. */
KD_API const KDchar *KD_APIENTRY kdGetLocale(void)
{
    return setlocale(LC_ALL,NULL);
}

/******************************************************************************
 * Memory allocation
 ******************************************************************************/

/* kdMalloc: Allocate memory. */
KD_API void *KD_APIENTRY kdMalloc(KDsize size)
{
    return malloc(size);
}

/* kdFree: Free allocated memory block. */
KD_API void KD_APIENTRY kdFree(void *ptr)
{
    free(ptr);
}

/* kdRealloc: Resize memory block. */
KD_API void *KD_APIENTRY kdRealloc(void *ptr, KDsize size)
{
    return realloc(ptr, size);
}

/******************************************************************************
 * Thread-local storage.
 ******************************************************************************/

static KD_THREADLOCAL void* tlsptr = KD_NULL;
/* kdGetTLS: Get the thread-local storage pointer. */
KD_API void *KD_APIENTRY kdGetTLS(void)
{
    return tlsptr;
}

/* kdSetTLS: Set the thread-local storage pointer. */
KD_API void KD_APIENTRY kdSetTLS(void *ptr)
{
    tlsptr = ptr;
}

/******************************************************************************
 * Mathematical functions
 ******************************************************************************/

 /* kdAcosf: Arc cosine function. */
KD_API KDfloat32 KD_APIENTRY kdAcosf(KDfloat32 x)
{
    return acosf(x);
}

/* kdAsinf: Arc sine function. */
KD_API KDfloat32 KD_APIENTRY kdAsinf(KDfloat32 x)
{
    return asinf(x);
}

/* kdAtanf: Arc tangent function. */
KD_API KDfloat32 KD_APIENTRY kdAtanf(KDfloat32 x)
{
    return atanf(x);
}

/* kdAtan2f: Arc tangent function. */
KD_API KDfloat32 KD_APIENTRY kdAtan2f(KDfloat32 y, KDfloat32 x)
{
    return atan2f(y, x);
}

/* kdCosf: Cosine function. */
KD_API KDfloat32 KD_APIENTRY kdCosf(KDfloat32 x)
{
    return cosf(x);
}

/* kdSinf: Sine function. */
KD_API KDfloat32 KD_APIENTRY kdSinf(KDfloat32 x)
{
    return sinf(x);
}

/* kdTanf: Tangent function. */
KD_API KDfloat32 KD_APIENTRY kdTanf(KDfloat32 x)
{
    return tanf(x);
}

/* kdExpf: Exponential function. */
KD_API KDfloat32 KD_APIENTRY kdExpf(KDfloat32 x)
{
    return expf(x);
}

/* kdLogf: Natural logarithm function. */
KD_API KDfloat32 KD_APIENTRY kdLogf(KDfloat32 x)
{
    return logf(x);
}

/* kdFabsf: Absolute value. */
KD_API KDfloat32 KD_APIENTRY kdFabsf(KDfloat32 x)
{
    return fabsf(x);
}

/* kdPowf: Power function. */
KD_API KDfloat32 KD_APIENTRY kdPowf(KDfloat32 x, KDfloat32 y)
{
    return powf(x, y);
}

/* kdSqrtf: Square root function. */
KD_API KDfloat32 KD_APIENTRY kdSqrtf(KDfloat32 x)
{
    return sqrtf(x);
}

/* kdCeilf: Return ceiling value. */
KD_API KDfloat32 KD_APIENTRY kdCeilf(KDfloat32 x)
{
    return ceilf(x);
}

/* kdFloorf: Return floor value. */
KD_API KDfloat32 KD_APIENTRY kdFloorf(KDfloat32 x)
{
    return floorf(x);
}

/* kdRoundf: Round value to nearest integer. */
KD_API KDfloat32 KD_APIENTRY kdRoundf(KDfloat32 x)
{
    return roundf(x);
}

/* kdInvsqrtf: Inverse square root function. */
KD_API KDfloat32 KD_APIENTRY kdInvsqrtf(KDfloat32 x)
{
    return 1.0f/kdSqrtf(x);
}

/* kdFmodf: Calculate floating point remainder. */
KD_API KDfloat32 KD_APIENTRY kdFmodf(KDfloat32 x, KDfloat32 y)
{
    return fmodf(x, y);
}

/******************************************************************************
 * String and memory functions
 *
 * Notes:
 * - Copied from the Public Domain C Library (https://bitbucket.org/pdclib)
 ******************************************************************************/

 /* kdMemchr: Scan memory for a byte value. */
KD_API void *KD_APIENTRY kdMemchr(const void *src, KDint byte, KDsize len)
{
    const KDuint8* p = (const KDuint8*) src;
    while(len--)
    {
        if(*p == (KDuint8)byte)
        {
            return (void*)p;
        }
        ++p;
    }
    return KD_NULL;
}

/* kdMemcmp: Compare two memory regions. */
KD_API KDint KD_APIENTRY kdMemcmp(const void *src1, const void *src2, KDsize len)
{
    const KDuint8* p1 = (const KDuint8*) src1;
    const KDuint8* p2 = (const KDuint8*) src2;
    while(len-- )
    {
        if(*p1 != *p2)
        {
            return *p1 - *p2;
        }
        ++p1;
        ++p2;
    }
    return 0;
}

/* kdMemcpy: Copy a memory region, no overlapping. */
KD_API void *KD_APIENTRY kdMemcpy(void *buf, const void *src, KDsize len)
{
    KDuint8* dest = (KDuint8*) buf;
    const KDuint8 * tmp = (const KDuint8 *) src;
    while (len--)
    {
        *dest++ = *tmp++;
    }
    return buf;
}

/* kdMemmove: Copy a memory region, overlapping allowed. */
KD_API void *KD_APIENTRY kdMemmove(void *buf, const void *src, KDsize len)
{
    KDuint8* dest = (KDuint8*) buf;
    const KDuint8 * tmp = (const KDuint8 *)src;
    if(dest <= tmp)
    {
        while(len--)
        {
            *dest++ = *tmp++;
        }
    }
    else
    {
        tmp += len;
        dest += len;
        while(len--)
        {
            *--dest = *--tmp;
        }
    }
    return buf;
}

/* kdMemset: Set bytes in memory to a value. */
KD_API void *KD_APIENTRY kdMemset(void *buf, KDint byte, KDsize len)
{
    KDuint8* p = (KDuint8*) buf;
    while (len--)
    {
        *p++ = (KDuint8) byte;
    }
    return buf;
}

/* kdStrchr: Scan string for a byte value. */
KD_API KDchar *KD_APIENTRY kdStrchr(const KDchar *str, KDint ch)
{
    do
    {
        if(*str == (KDchar)ch )
        {
            return (KDchar*)str;
        }
    } while( *str++ );
    return KD_NULL;
}

/* kdStrcmp: Compares two strings. */
KD_API KDint KD_APIENTRY kdStrcmp(const KDchar *str1, const KDchar *str2)
{
    while ((*str1) && (*str1 == *str2))
    {
        ++str1;
        ++str2;
    }
    return (*(KDuint8*)str1 - *(KDuint8*)str2);
}

/* kdStrlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrlen(const KDchar *str)
{
    KDsize rc = 0;
    while ( str[rc] )
    {
        ++rc;
    }
    return rc;
}

/* kdStrnlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrnlen(const KDchar *str, KDsize maxlen)
{
    KDsize i = 0;
    for(; (i < maxlen) && str[i]; ++i);
    return i;
}

/* kdStrncat_s: Concatenate two strings. */
KD_API KDint KD_APIENTRY kdStrncat_s(KDchar *buf, KDsize buflen, const KDchar *src, KD_UNUSED KDsize srcmaxlen)
{
    KDsize needed = 0;
    KDsize j = 0;

    while(buf[needed])
    {  
        needed++;
    }
    while(needed < buflen)
    {
        buf[needed] = src[j];
        if(buf[needed])
        {
            needed++; 
            j++;    
        }
        else
        {
            break;
        }
    }
    while(src[j++]) 
    {
        needed++;
    }
    needed++;

    if(needed > buflen && buflen)
    {
      buf[buflen - 1] = 0;
    }

    return (KDint)needed;
}

/* kdStrncmp: Compares two strings with length limit. */
KD_API KDint KD_APIENTRY kdStrncmp(const KDchar *str1, const KDchar *str2, KDsize maxlen)
{
    while(*str1 && maxlen && (*str1 == *str2))
    {
        ++str1;
        ++str2;
        --maxlen;
    }
    if(maxlen == 0)
    {
        return 0;
    }
    else
    {
        return (*(KDuint8*)str1 - *(KDuint8*)str2);
    }
}


/* kdStrcpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrcpy_s(KDchar *buf, KDsize buflen, const KDchar *src)
{
    KDsize needed = 0;
    while(needed < buflen)
    {
        buf[needed] = src[needed];
        if(buf[needed])
        {
            needed++;
        }
        else
        {
            break;
        }
    }

    while(src[needed++]);

    if(needed > buflen && buflen)
    {
      buf[buflen - 1] = 0;
    }

    return (KDint)needed;
}

/* kdStrncpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrncpy_s(KDchar *buf, KDsize buflen, const KDchar *src, KD_UNUSED KDsize srclen)
{
    return kdStrcpy_s(buf, buflen, src);
}

/******************************************************************************
 * Time functions
 ******************************************************************************/

/* kdGetTimeUST: Get the current unadjusted system time. */
KD_API KDust KD_APIENTRY kdGetTimeUST(void)
{
    return clock();
}

/* kdTime: Get the current wall clock time. */
KD_API KDtime KD_APIENTRY kdTime(KDtime *timep)
{
    return time((time_t*)timep);
}

/* kdGmtime_r, kdLocaltime_r: Convert a seconds-since-epoch time into broken-down time. */
KD_API KDTm *KD_APIENTRY kdGmtime_r(const KDtime *timep, KDTm *result)
{
    KDTm *retval = (KDTm*)gmtime((const time_t*)timep);
    *result = *retval;
    return retval;
}
KD_API KDTm *KD_APIENTRY kdLocaltime_r(const KDtime *timep, KDTm *result)
{
    KDTm *retval = (KDTm*)localtime((const time_t*)timep);
    *result = *retval;
    return retval;
}

/* kdUSTAtEpoch: Get the UST corresponding to KDtime 0. */
KD_API KDust KD_APIENTRY kdUSTAtEpoch(void)
{
    kdSetError(KD_EOPNOTSUPP);
    return 0;
}

/******************************************************************************
 * Timer functions
 ******************************************************************************/

/* kdSetTimer: Set timer. */
typedef struct
{
    KDint64     interval;
    KDint       periodic;
    void        *eventuserptr;
    KDThread    *destination;
} __KDTimerPayload;
static void* __kdTimerHandler(void *arg)
{
    __KDTimerPayload* payload = (__KDTimerPayload*)arg;
    for(;;)
    {
        __KDSleep(payload->interval);

        /* Post event to the original thread */
        KDEvent *timerevent = kdCreateEvent();
        timerevent->type = KD_EVENT_TIMER;
        timerevent->userptr = payload->eventuserptr;
        kdPostThreadEvent(timerevent, payload->destination);

        /* Abort if this is a oneshot timer*/
        if(payload->periodic == KD_TIMER_ONESHOT)
        {
            break;
        }

        /* Check for quit event send by kdCancelTimer */
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_QUIT)
            {
                break;
            }
            kdDefaultEvent(event);
        }
    }
    return 0;
}
struct KDTimer
{
    KDThread* thread;
    __KDTimerPayload* payload;
};
KD_API KDTimer *KD_APIENTRY kdSetTimer(KDint64 interval, KDint periodic, void *eventuserptr)
{
    __KDTimerPayload* payload = (__KDTimerPayload*)kdMalloc(sizeof(__KDTimerPayload));
    payload->interval = interval;
    payload->periodic = periodic;
    payload->eventuserptr = eventuserptr;
    payload->destination = kdThreadSelf();

    KDTimer *timer = (KDTimer*)kdMalloc(sizeof(KDTimer));
    timer->thread = kdThreadCreate(KD_NULL, __kdTimerHandler, payload);
    timer->payload = payload;
    return timer;
}

/* kdCancelTimer: Cancel and free a timer. */
KD_API KDint KD_APIENTRY kdCancelTimer(KDTimer *timer)
{
    /* Post quit event to the timer thread */
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_QUIT;
    kdPostThreadEvent(event, timer->thread);
    kdThreadJoin(timer->thread, KD_NULL);
    kdFree(timer->payload);
    kdFree(timer);
    return 0;
}

/******************************************************************************
 * File system
 ******************************************************************************/

/* kdFopen: Open a file from the file system. */
struct KDFile
{
#ifdef KD_VFS_SUPPORTED
    PHYSFS_File *file;
#else
    FILE *file;
#endif
};
KD_API KDFile *KD_APIENTRY kdFopen(const KDchar *pathname, const KDchar *mode)
{
    KDFile *file = (KDFile*)kdMalloc(sizeof(KDFile));
#ifdef KD_VFS_SUPPORTED
    if(kdStrcmp(mode, "wb") == 0)
    {
        file->file = PHYSFS_openWrite(pathname);
    }
    else if(kdStrcmp(mode, "rb") == 0)
    {
        file->file = PHYSFS_openRead(pathname);
    }
    else if(kdStrcmp(mode, "ab") == 0)
    {
        file->file = PHYSFS_openAppend(pathname);
    }
#else
    file->file = fopen(pathname, mode);
#endif
    return file;
}

/* kdFclose: Close an open file. */
KD_API KDint KD_APIENTRY kdFclose(KDFile *file)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_close(file->file);
#else
    retval = fclose(file->file);
#endif
    kdFree(file);
    return retval;
}

/* kdFflush: Flush an open file. */
KD_API KDint KD_APIENTRY kdFflush(KDFile *file)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_flush(file->file);
#else
    retval = fflush(file->file);
#endif
    return retval;
}

/* kdFread: Read from a file. */
KD_API KDsize KD_APIENTRY kdFread(void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDsize retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = (KDsize)PHYSFS_readBytes(file->file, buffer, size);
#else
    retval = fread(buffer, size, count, file->file);
#endif
    return retval;
}

/* kdFwrite: Write to a file. */
KD_API KDsize KD_APIENTRY kdFwrite(const void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDsize retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = (KDsize)PHYSFS_writeBytes(file->file, buffer, size);
#else
    retval = fwrite(buffer, size, count, file->file);
#endif
    return retval;
}

/* kdGetc: Read next byte from an open file. */
KD_API KDint KD_APIENTRY kdGetc(KDFile *file)
{
    KDint byte = 0;
#ifdef KD_VFS_SUPPORTED
    PHYSFS_readBytes(file->file, &byte, 1);
#else
    byte = fgetc(file->file);
#endif
    return byte;
}

/* kdPutc: Write a byte to an open file. */
KD_API KDint KD_APIENTRY kdPutc(KDint c, KDFile *file)
{
    KDint byte = 0;
#ifdef KD_VFS_SUPPORTED
    PHYSFS_writeBytes(file->file, &c, 1);
    byte = c;
#else
    byte = fputc(c, file->file);
#endif
    return byte;
}

/* kdFgets: Read a line of text from an open file. */
KD_API KDchar *KD_APIENTRY kdFgets(KDchar *buffer, KDsize buflen, KDFile *file)
{
    KDchar *line = KD_NULL;
#ifdef KD_VFS_SUPPORTED
    /* TODO: Loop this until '\n' or 'EOF' */
    kdAssert(0);
    PHYSFS_readBytes(file->file, buffer, 1);
    line = buffer;
#else
    line = fgets(buffer, (KDint)buflen, file->file);
#endif
    return line;
}

/* kdFEOF: Check for end of file. */
KD_API KDint KD_APIENTRY kdFEOF(KDFile *file)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_eof(file->file);
#else
    retval = feof(file->file);
#endif
    return retval;
}

/* kdFerror: Check for an error condition on an open file. */
KD_API KDint KD_APIENTRY kdFerror(KDFile *file)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    PHYSFS_ErrorCode errorcode = PHYSFS_getLastErrorCode();
    if(errorcode != PHYSFS_ERR_OK)
    {
        retval = KD_EOF;
    }
#else
    if(ferror(file->file) != 0)
    {
        retval = KD_EOF;
    }
#endif
    return retval;
}

/* kdClearerr: Clear a file's error and end-of-file indicators. */
KD_API void KD_APIENTRY kdClearerr(KDFile *file)
{
#ifdef KD_VFS_SUPPORTED
    PHYSFS_setErrorCode(PHYSFS_ERR_OK);
#else
    clearerr(file->file);
#endif
}

typedef struct
{
#ifdef _MSC_VER
    KDint   seekorigin_kd;
#else
    KDuint  seekorigin_kd;
#endif
    int     seekorigin;
} __KDSeekOrigin;

#ifndef KD_VFS_SUPPORTED
static __KDSeekOrigin seekorigins_c[] =
{
    {KD_SEEK_SET, SEEK_SET},
    {KD_SEEK_CUR, SEEK_CUR},
    {KD_SEEK_END, SEEK_END}
};
#endif

/* kdFseek: Reposition the file position indicator in a file. */
KD_API KDint KD_APIENTRY kdFseek(KDFile *file, KDoff offset, KDfileSeekOrigin origin)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    /* TODO: Support other seek origins*/
    if(origin == KD_SEEK_SET)
    {
        retval = PHYSFS_seek(file->file, (PHYSFS_uint64)offset);
    }
    else
    {
        kdAssert(0);
    }
#else
    for (KDuint i = 0; i < sizeof(seekorigins_c) / sizeof(seekorigins_c[0]); i++)
    {
        if(seekorigins_c[i].seekorigin_kd == origin)
        {
			retval = fseek(file->file, (KDint32)offset, seekorigins_c[i].seekorigin);
            break;
        }
    }
#endif
    return retval;
}

/* kdFtell: Get the file position of an open file. */
KD_API KDoff KD_APIENTRY kdFtell(KDFile *file)
{
    KDoff position = 0;
#ifdef KD_VFS_SUPPORTED
    position = (KDoff)PHYSFS_tell(file->file);
#else
    position = ftell(file->file);
#endif
    return position;
}

/* kdMkdir: Create new directory. */
KD_API KDint KD_APIENTRY kdMkdir(const KDchar *pathname)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_mkdir(pathname);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    retval = mkdir(pathname);
#else
    retval = mkdir(pathname, S_IRWXU);
#endif
    return retval;
}

/* kdRmdir: Delete a directory. */
KD_API KDint KD_APIENTRY kdRmdir(const KDchar *pathname)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_delete(pathname);
#else
    retval = rmdir(pathname);
#endif
    return retval;
}

/* kdRename: Rename a file. */
KD_API KDint KD_APIENTRY kdRename(const KDchar *src, const KDchar *dest)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    /* TODO: Implement kdRename */
    kdAssert(0);
#else
    retval = rename(src, dest);
#endif
    return retval;
}

/* kdRemove: Delete a file. */
KD_API KDint KD_APIENTRY kdRemove(const KDchar *pathname)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    retval = PHYSFS_delete(pathname);
#else
    retval = remove(pathname);
#endif
    return retval;
}

/* kdTruncate: Truncate or extend a file. */
KD_API KDint KD_APIENTRY kdTruncate(KD_UNUSED const KDchar *pathname, KD_UNUSED KDoff length)
{
    KDint retval = 0;
#ifdef KD_VFS_SUPPORTED
    /* TODO: Implement kdTruncate */
    kdAssert(0);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    /* TODO: Implement kdTruncate on Windows */
    kdAssert(0);
#else
    retval = truncate(pathname, length);
#endif
    return retval;
}

/* kdStat, kdFstat: Return information about a file. */
KD_API KDint KD_APIENTRY kdStat(const KDchar *pathname, struct KDStat *buf)
{
    KDint retval = -1;
#ifdef KD_VFS_SUPPORTED
    struct PHYSFS_Stat physfsstat = {0};
    if(PHYSFS_stat(pathname, &physfsstat) != 0)
    {
        retval = 0;
        if(physfsstat.filetype == PHYSFS_FILETYPE_DIRECTORY)
        {
            buf->st_mode  = 0x4000;
        }
        else if(physfsstat.filetype == PHYSFS_FILETYPE_REGULAR )
        {
            buf->st_mode  = 0x8000;
        }
        else
        {
            buf->st_mode  = 0x0000;
            retval = -1;
        }
        buf->st_size  = (KDoff)physfsstat.filesize;
        buf->st_mtime = (KDtime)physfsstat.modtime;
    }
#else
    struct stat posixstat = {0};
    retval = stat(pathname, &posixstat);

    buf->st_mode  = posixstat.st_mode;
    buf->st_size  = posixstat.st_size;
#if  defined(__ANDROID__) || defined(_MSC_VER) || defined(__MINGW32__)
    buf->st_mtime = posixstat.st_mtime;
#elif  defined(__APPLE__)
    buf->st_mtime = posixstat.st_mtimespec.tv_sec;
#else
    buf->st_mtime = posixstat.st_mtim.tv_sec;
#endif
#endif
    return retval;
}

KD_API KDint KD_APIENTRY kdFstat(KDFile *file, struct KDStat *buf)
{
    KDint retval = -1;
#ifdef KD_VFS_SUPPORTED
    buf->st_mode  = 0x0000;
    buf->st_size  = (KDoff)PHYSFS_fileLength(file->file);
    buf->st_mtime = 0;
#else
    struct stat posixstat = {0};
    retval = fstat(fileno(file->file), &posixstat);

    buf->st_mode  = posixstat.st_mode;
    buf->st_size  = posixstat.st_size;
#if  defined(__ANDROID__) || defined(_MSC_VER) || defined(__MINGW32__)
    buf->st_mtime = posixstat.st_mtime;
#elif  defined(__APPLE__)
    buf->st_mtime = posixstat.st_mtimespec.tv_sec;
#else
    buf->st_mtime = posixstat.st_mtim.tv_sec;
#endif
#endif
    return retval;
}

typedef struct
{
    KDint   accessmode_kd;
    int     accessmode;
} __KDAccessMode;

#if defined(_MSC_VER)
static __KDAccessMode accessmode[] =
{
	{ KD_R_OK, 04 },
	{ KD_W_OK, 02 },
	{ KD_X_OK, 00 }
};
#else
static __KDAccessMode accessmode[] =
{
    {KD_R_OK, R_OK},
    {KD_W_OK, W_OK},
    {KD_X_OK, X_OK}
};
#endif

/* kdAccess: Determine whether the application can access a file or directory. */
KD_API KDint KD_APIENTRY kdAccess(const KDchar *pathname, KDint amode)
{
    KDint retval = -1;
#ifdef KD_VFS_SUPPORTED
    /* TODO: Implement kdAccess */
    kdAssert(0);
#else
    for (KDuint i = 0; i < sizeof(accessmode) / sizeof(accessmode[0]); i++)
    {
        if(accessmode[i].accessmode_kd == amode)
        {
            retval = access(pathname, accessmode[i].accessmode);
            break;
        }
    }
#endif
    return retval;
}

/* kdOpenDir: Open a directory ready for listing. */
struct KDDir
{
#ifdef KD_VFS_SUPPORTED
    char **dir;
#else
    DIR *dir;
#endif
};
KD_API KDDir *KD_APIENTRY kdOpenDir(const KDchar *pathname)
{
    KDDir *dir = (KDDir*)kdMalloc(sizeof(KDDir));
#ifdef KD_VFS_SUPPORTED
    dir->dir = PHYSFS_enumerateFiles(pathname);
#else
    dir->dir = opendir(pathname);
#endif
    return dir;
}

/* kdReadDir: Return the next file in a directory. */
static KD_THREADLOCAL KDDirent *__kd_lastdirent = KD_NULL;
KD_API KDDirent *KD_APIENTRY kdReadDir(KDDir *dir)
{
#ifdef KD_VFS_SUPPORTED
    KDboolean next = 0;
    for (KDchar **i = dir->dir; *i != KD_NULL; i++)
    {
        if(__kd_lastdirent == KD_NULL || next == 1)
        {
            __kd_lastdirent->d_name = *i;
        }
        else
        {
            if(kdStrcmp(__kd_lastdirent->d_name, *i) == 0)
            {
                next = 1;
            }
        }
    }
#else
    struct dirent* posixdirent = readdir(dir->dir);
    __kd_lastdirent->d_name = posixdirent->d_name;
#endif
    return __kd_lastdirent;
}

/* kdCloseDir: Close a directory. */
KD_API KDint KD_APIENTRY kdCloseDir(KDDir *dir)
{
#ifdef KD_VFS_SUPPORTED
    PHYSFS_freeList(&dir->dir);
#else
    closedir(dir->dir);
#endif
    kdFree(dir);
    return 0;
}

/* kdGetFree: Get free space on a drive. */
KD_API KDoff KD_APIENTRY kdGetFree(const KDchar *pathname)
{
#ifdef KD_VFS_SUPPORTED
     const KDchar *temp = PHYSFS_getRealDir(pathname);
#else
     const KDchar *temp = pathname;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
     KDuint64 freespace = 0;
     GetDiskFreeSpaceEx(temp, (PULARGE_INTEGER)&freespace, KD_NULL, KD_NULL);
     return freespace;
#else
     struct statfs buf = {0};
     statfs(temp, &buf);
     return (buf.f_bsize/1024L) * buf.f_bavail;
#endif
}

 /******************************************************************************
 * Network sockets
 ******************************************************************************/

/* kdNameLookup: Look up a hostname. */
KD_API KDint KD_APIENTRY kdNameLookup(KD_UNUSED KDint af, KD_UNUSED const KDchar *hostname, KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdNameLookupCancel: Selectively cancels ongoing kdNameLookup operations. */
KD_API void KD_APIENTRY kdNameLookupCancel(KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
}

/* kdSocketCreate: Creates a socket. */
struct KDSocket
{
    int placebo;
};
KD_API KDSocket *KD_APIENTRY kdSocketCreate(KD_UNUSED KDint type, KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/* kdSocketClose: Closes a socket. */
KD_API KDint KD_APIENTRY kdSocketClose(KD_UNUSED KDSocket *socket)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketBind: Bind a socket. */
KD_API KDint KD_APIENTRY kdSocketBind(KD_UNUSED KDSocket *socket, KD_UNUSED const struct KDSockaddr *addr, KD_UNUSED KDboolean reuse)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketGetName: Get the local address of a socket. */
KD_API KDint KD_APIENTRY kdSocketGetName(KD_UNUSED KDSocket *socket, KD_UNUSED struct KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketConnect: Connects a socket. */
KD_API KDint KD_APIENTRY kdSocketConnect(KD_UNUSED KDSocket *socket, KD_UNUSED const KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketListen: Listen on a socket. */
KD_API KDint KD_APIENTRY kdSocketListen(KD_UNUSED KDSocket *socket, KD_UNUSED KDint backlog)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketAccept: Accept an incoming connection. */
KD_API KDSocket *KD_APIENTRY kdSocketAccept(KD_UNUSED KDSocket *socket, KD_UNUSED KDSockaddr *addr, KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/* kdSocketSend, kdSocketSendTo: Send data to a socket. */
KD_API KDint KD_APIENTRY kdSocketSend(KD_UNUSED KDSocket *socket, KD_UNUSED const void *buf, KD_UNUSED KDint len)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdSocketSendTo(KD_UNUSED KDSocket *socket, KD_UNUSED const void *buf, KD_UNUSED KDint len, KD_UNUSED const KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketRecv, kdSocketRecvFrom: Receive data from a socket. */
KD_API KDint KD_APIENTRY kdSocketRecv(KD_UNUSED KDSocket *socket, KD_UNUSED void *buf, KD_UNUSED KDint len)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdSocketRecvFrom(KD_UNUSED KDSocket *socket, KD_UNUSED void *buf, KD_UNUSED KDint len, KD_UNUSED KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdHtonl: Convert a 32-bit integer from host to network byte order. */
KD_API KDuint32 KD_APIENTRY kdHtonl(KD_UNUSED KDuint32 hostlong)
{
    kdSetError(KD_EOPNOTSUPP);
    return 0;
}

/* kdHtons: Convert a 16-bit integer from host to network byte order. */
KD_API KDuint16 KD_APIENTRY kdHtons(KD_UNUSED KDuint16 hostshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return (KDuint16)0;
}

/* kdNtohl: Convert a 32-bit integer from network to host byte order. */
KD_API KDuint32 KD_APIENTRY kdNtohl(KD_UNUSED KDuint32 netlong)
{
    kdSetError(KD_EOPNOTSUPP);
    return 0;
}

/* kdNtohs: Convert a 16-bit integer from network to host byte order. */
KD_API KDuint16 KD_APIENTRY kdNtohs(KD_UNUSED KDuint16 netshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return (KDuint16)0;
}

/* kdInetAton: Convert a &#8220;dotted quad&#8221; format address to an integer. */
KD_API KDint KD_APIENTRY kdInetAton(KD_UNUSED const KDchar *cp, KD_UNUSED KDuint32 *inp)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdInetNtop: Convert a network address to textual form. */
KD_API const KDchar *KD_APIENTRY kdInetNtop(KD_UNUSED KDuint af, KD_UNUSED const void *src, KD_UNUSED KDchar *dst, KD_UNUSED KDsize cnt)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/******************************************************************************
 * Input/output
 ******************************************************************************/
/* kdStateGeti, kdStateGetl, kdStateGetf: get state value(s) */
KD_API KDint KD_APIENTRY kdStateGeti(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDint32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;  
}

KD_API KDint KD_APIENTRY kdStateGetl(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDint64 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdStateGetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDfloat32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}


/* kdOutputSeti, kdOutputSetf: set outputs */
KD_API KDint KD_APIENTRY kdOutputSeti(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDint32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdOutputSetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDfloat32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/******************************************************************************
 * Windowing
 ******************************************************************************/
#ifdef KD_WINDOW_SUPPORTED
/* kdCreateWindow: Create a window. */
#if defined(KD_WINDOW_WIN32)
LRESULT CALLBACK windowcallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
    return 0;
}
#endif
KD_API KDWindow *KD_APIENTRY kdCreateWindow(KD_UNUSED EGLDisplay display, KD_UNUSED EGLConfig config, KD_UNUSED void *eventuserptr)
{
    if(!__kd_window)
    {
        KDWindow *window = (KDWindow *)kdMalloc(sizeof(KDWindow));
#if defined(KD_WINDOW_ANDROID)
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->format);
#elif defined(KD_WINDOW_WIN32)
        WNDCLASS windowclass = { 0 };
        HINSTANCE instance = GetModuleHandle(KD_NULL);
        GetClassInfo(instance, "", &windowclass);
        windowclass.lpszClassName = "OpenKODE";
        windowclass.lpfnWndProc = windowcallback;
        windowclass.hInstance = instance;
        windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
        RegisterClass(&windowclass);
        window->nativewindow = CreateWindow("OpenKODE", "OpenKODE", WS_POPUP|WS_VISIBLE, 0, 0,
                                            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                                            KD_NULL, KD_NULL, instance, KD_NULL);
        /* Activate raw input */
        RAWINPUTDEVICE device[2];
        /* Mouse */
        device[0].usUsagePage = 1;
        device[0].usUsage = 2;
        device[0].dwFlags = RIDEV_NOLEGACY;
        device[0].hwndTarget = window->nativewindow;
        /* Keyboard */
        device[1].usUsagePage = 1;
        device[1].usUsage = 6;
        device[1].dwFlags = RIDEV_NOLEGACY;
        device[1].hwndTarget = window->nativewindow;
        RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE));
#elif defined(KD_WINDOW_X11)
        XInitThreads();
        window->nativedisplay = XOpenDisplay(NULL);
        window->nativewindow = XCreateSimpleWindow(window->nativedisplay,
            XRootWindow(window->nativedisplay, XDefaultScreen(window->nativedisplay)), 0, 0,
            (KDuint)XWidthOfScreen(XDefaultScreenOfDisplay(window->nativedisplay)),
            (KDuint)XHeightOfScreen(XDefaultScreenOfDisplay(window->nativedisplay)), 0,
            XBlackPixel(window->nativedisplay, XDefaultScreen(window->nativedisplay)),
            XWhitePixel(window->nativedisplay, XDefaultScreen(window->nativedisplay)));
        XStoreName(window->nativedisplay, window->nativewindow, "OpenKODE");
        Atom wm_del_win_msg = XInternAtom(window->nativedisplay, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(window->nativedisplay, window->nativewindow, &wm_del_win_msg, 1);
        Atom mwm_prop_hints = XInternAtom(window->nativedisplay, "_MOTIF_WM_HINTS", True);
        const KDuint8 mwm_hints[5] = { 2, 0, 0, 0, 0 };
        XChangeProperty(window->nativedisplay, window->nativewindow, mwm_prop_hints, mwm_prop_hints, 32, 0, (const KDuint8*)&mwm_hints, 5);
        Atom netwm_prop_hints = XInternAtom(window->nativedisplay, "_NET_WM_STATE", False);
        Atom netwm_hints[3];
        netwm_hints[0] = XInternAtom(window->nativedisplay, "_NET_WM_STATE_FULLSCREEN", False);
        netwm_hints[1] = XInternAtom(window->nativedisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        netwm_hints[2] = XInternAtom(window->nativedisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        netwm_hints[2] = XInternAtom(window->nativedisplay, "_NET_WM_STATE_FOCUSED", False);
        XChangeProperty(window->nativedisplay, window->nativewindow, netwm_prop_hints, 4, 32, 0, (const KDuint8*)&netwm_hints, 3);
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOW_REDRAW;
        kdPostThreadEvent(event, __kd_mainthread);
#endif
        __kd_window = window;
    }
    return __kd_window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
#if defined(KD_WINDOW_WIN32)
    DestroyWindow(window->nativewindow);
#elif defined(KD_WINDOW_X11)
    XCloseDisplay(window->nativedisplay);
#endif
    kdFree(window);
    __kd_window = KD_NULL;
    return 0;
}

/* kdSetWindowPropertybv, kdSetWindowPropertyiv, kdSetWindowPropertycv: Set a window property to request a change in the on-screen representation of the window. */
KD_API KDint KD_APIENTRY kdSetWindowPropertybv(KD_UNUSED KDWindow *window, KD_UNUSED KDint pname, KD_UNUSED const KDboolean *param)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdSetWindowPropertyiv(KD_UNUSED KDWindow *window, KDint pname, KD_UNUSED const KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#if defined(KD_WINDOW_ANDROID) || defined(KD_WINDOW_NULL)
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif defined(KD_WINDOW_X11)
        XMoveResizeWindow(window->nativedisplay, window->nativewindow, 0, 0, (KDuint)param[0], (KDuint)param[1]);
        XFlush(window->nativedisplay);
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        kdPostThreadEvent(event, kdThreadSelf());
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdSetWindowPropertycv(KD_UNUSED KDWindow *window, KDint pname, KD_UNUSED const KDchar *param)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#if defined(KD_WINDOW_ANDROID) || defined(KD_WINDOW_NULL)
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif defined(KD_WINDOW_X11)
        XStoreName(window->nativedisplay, window->nativewindow, param);
        XFlush(window->nativedisplay);
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        kdPostThreadEvent(event, kdThreadSelf());
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdGetWindowPropertybv, kdGetWindowPropertyiv, kdGetWindowPropertycv: Get the current value of a window property. */
KD_API KDint KD_APIENTRY kdGetWindowPropertybv(KD_UNUSED KDWindow *window, KD_UNUSED KDint pname, KD_UNUSED KDboolean *param)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertyiv(KD_UNUSED KDWindow *window, KDint pname, KD_UNUSED KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#if defined(KD_WINDOW_NULL)
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif defined(KD_WINDOW_ANDROID)
        param[0] = ANativeWindow_getWidth(window->nativewindow);
        param[1] = ANativeWindow_getHeight(window->nativewindow);
#elif defined(KD_WINDOW_X11)
        param[0] = XWidthOfScreen(XDefaultScreenOfDisplay(window->nativedisplay));
        param[1] = XHeightOfScreen(XDefaultScreenOfDisplay(window->nativedisplay));
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertycv(KD_UNUSED KDWindow *window, KDint pname, KD_UNUSED KDchar *param, KD_UNUSED KDsize *size)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#if defined(KD_WINDOW_ANDROID) || defined(KD_WINDOW_NULL)
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif defined(KD_WINDOW_X11)
        XFetchName(window->nativedisplay, window->nativewindow, &param);
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdRealizeWindow: Realize the window as a displayable entity and get the native window handle for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizeWindow(KDWindow *window, EGLNativeWindowType *nativewindow)
{
#if defined(KD_WINDOW_ANDROID)
    for(;;)
    {
        kdThreadMutexLock(__kd_androidwindow_mutex);
        if(__kd_androidwindow != KD_NULL)
        {
            window->nativewindow = __kd_androidwindow;
            kdThreadMutexUnlock(__kd_androidwindow_mutex);
            break;
        }
        kdThreadMutexUnlock(__kd_androidwindow_mutex);
    }
    ANativeWindow_setBuffersGeometry(window->nativewindow, 0, 0, window->format);
#elif defined(KD_WINDOW_X11)
    XMapWindow(window->nativedisplay, window->nativewindow);
    XFlush(window->nativedisplay);
#endif
    *nativewindow = window->nativewindow;
    return 0;
}
#endif

/******************************************************************************
 * Assertions and logging
 ******************************************************************************/

/* kdHandleAssertion: Handle assertion failure. */
KD_API void KD_APIENTRY kdHandleAssertion(KD_UNUSED const KDchar *condition, KD_UNUSED const KDchar *filename, KD_UNUSED KDint linenumber)
{

}

/* kdLogMessage: Output a log message. */
#ifndef KD_NDEBUG
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, __kdAppName(KD_NULL), string);
#else
    printf("%s", string);
    fflush(stdout);
#endif
}
#endif

/******************************************************************************
 * Extensions
 ******************************************************************************/

/******************************************************************************
 * Atomics
 ******************************************************************************/

#if defined(KD_ATOMIC_C11)
struct KDAtomicIntVEN { atomic_int value; };
struct KDAtomicPtrVEN { atomic_uintptr_t value; };
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_LEGACY)
struct KDAtomicIntVEN { KDint value; };
struct KDAtomicPtrVEN { void *value; };
#endif

KD_API KDAtomicIntVEN* KD_APIENTRY kdAtomicIntCreateVEN(KDint value)
{
    KDAtomicIntVEN* object = (KDAtomicIntVEN*)kdMalloc(sizeof(KDAtomicIntVEN));
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_LEGACY)
    object->value = value;
#endif
    return object;
}

KD_API KDAtomicPtrVEN* KD_APIENTRY kdAtomicPtrCreateVEN(void* value)
{
    KDAtomicPtrVEN* object = (KDAtomicPtrVEN*)kdMalloc(sizeof(KDAtomicPtrVEN));
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, (uintptr_t)value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_BUILTIN) || defined(KD_ATOMIC_LEGACY)
    object->value = value;
#endif
    return object;
}

KD_API void KD_APIENTRY kdAtomicIntFreeVEN(KDAtomicIntVEN *object)
{
    kdFree(object);
}

KD_API void KD_APIENTRY kdAtomicPtrFreeVEN(KDAtomicPtrVEN *object)
{
    kdFree(object);
}

KD_API KDint KD_APIENTRY kdAtomicIntLoadVEN(KDAtomicIntVEN *object)
{
#if defined(KD_ATOMIC_C11)
    return atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_LEGACY)
    int value = 0;
    do {
        value = object->value;
    } while(!kdAtomicIntCompareExchangeVEN(object, value, value));
    return value;
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_load_n(&object->value, __ATOMIC_SEQ_CST);
#endif
}

KD_API void* KD_APIENTRY kdAtomicPtrLoadVEN(KDAtomicPtrVEN *object)
{
#if defined(KD_ATOMIC_C11)
    return (void*)atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32) || defined(KD_ATOMIC_LEGACY)
    void* value = 0;
    do {
        value = object->value;
    } while(!kdAtomicPtrCompareExchangeVEN(object, value, value));
    return value;
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_load_n(&object->value, __ATOMIC_SEQ_CST);
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
#elif defined(KD_ATOMIC_LEGACY)
    __sync_lock_test_and_set(&object->value, value);
#endif
}

KD_API void KD_APIENTRY kdAtomicPtrStoreVEN(KDAtomicPtrVEN *object, void* value)
{
#if defined(KD_ATOMIC_C11)
    atomic_store(&object->value, (uintptr_t)value);
#elif defined(KD_ATOMIC_WIN32) && defined(_M_IX86)
    _InterlockedExchange((long *)&object->value, (long) value);
#elif defined(KD_ATOMIC_WIN32)
    _InterlockedExchangePointer(&object->value, value);
#elif defined(KD_ATOMIC_BUILTIN)
    __atomic_store_n(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_LEGACY)
    KD_UNUSED void* unused = __sync_lock_test_and_set(&object->value, value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchAddVEN(KDAtomicIntVEN *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_add(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long*)&object->value, (long)value);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_add_fetch(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_LEGACY)
    return __sync_fetch_and_add(&object->value, value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchSubVEN(KDAtomicIntVEN *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_sub(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long*)&object->value, (long)-value);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_sub_fetch(&object->value, value, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_LEGACY)
    return __sync_fetch_and_sub(&object->value, value);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN *object, KDint expected, KDint desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, &expected, desired);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchange((long*)&object->value, (long)desired, (long)expected) == (long)expected);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_compare_exchange_n(&object->value, &expected, desired, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_LEGACY)
    return __sync_bool_compare_and_swap(&object->value, expected, desired);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN *object, void* expected, void* desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, (uintptr_t*)&expected, (uintptr_t)desired);
#elif defined(KD_ATOMIC_WIN32) && defined(_M_IX86)
    return (_InterlockedCompareExchange((long*)&object->value, (long)desired, (long)expected) == (long)expected);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchangePointer(&object->value, desired, expected) == expected);
#elif defined(KD_ATOMIC_BUILTIN)
    return __atomic_compare_exchange_n(&object->value, &expected, desired, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif defined(KD_ATOMIC_LEGACY)
    return __sync_bool_compare_and_swap(&object->value, expected, desired);
#endif
}

/******************************************************************************
 * Queue (threadsafe)
 ******************************************************************************/

typedef struct __KDQueueNode __KDQueueNode;
struct __KDQueueNode
{
    __KDQueueNode   *next;
    __KDQueueNode   *prev;
    void            *value;
};

struct KDQueueVEN
{
    KDThreadMutex   *mutex;
    __KDQueueNode   *head;
    __KDQueueNode   *tail;
    KDsize          size;
};

KD_API KDQueueVEN* KD_APIENTRY kdQueueCreateVEN(KD_UNUSED KDsize maxsize)
{
    KDQueueVEN* queue = (KDQueueVEN*)kdMalloc(sizeof(KDQueueVEN));
    queue->mutex = kdThreadMutexCreate(KD_NULL);
    queue->head = KD_NULL;
    queue->tail = KD_NULL;
    queue->size = 0;
    return queue;
}

KD_API void KD_APIENTRY kdQueueFreeVEN(KDQueueVEN* queue)
{
    kdThreadMutexFree(queue->mutex);
    kdFree(queue);
}

KD_API KDsize KD_APIENTRY kdQueueSizeVEN(KDQueueVEN *queue)
{
    kdThreadMutexLock(queue->mutex);
    KDsize size = queue->size;
    kdThreadMutexUnlock(queue->mutex);
    return size;
}

KD_API void KD_APIENTRY kdQueuePushHeadVEN(KDQueueVEN *queue, void *value)
{
    __KDQueueNode *node = (__KDQueueNode*)kdMalloc(sizeof(__KDQueueNode));
    node->value = value;
    node->prev = KD_NULL;
    node->next = KD_NULL;

    kdThreadMutexLock(queue->mutex);
    if((node->next = queue->head) != KD_NULL) 
    {
        node->next->prev = node;
    }
    queue->head = node;
    if(!queue->tail) 
    {
        queue->tail = node;
    }
    queue->size++;
    kdThreadMutexUnlock(queue->mutex);
}

KD_API void KD_APIENTRY kdQueuePushTailVEN(KDQueueVEN *queue, void *value)
{
    __KDQueueNode *node = (__KDQueueNode*)kdMalloc(sizeof(__KDQueueNode));
    node->value = value;
    node->prev = KD_NULL;
    node->next = KD_NULL;

    kdThreadMutexLock(queue->mutex);
    if((node->prev = queue->tail) != KD_NULL) 
    {
        queue->tail->next = node;
    }
    queue->tail = node;
    if(!queue->head) 
    {
        queue->head = node;
    }
    queue->size++;
    kdThreadMutexUnlock(queue->mutex);
}

KD_API void* KD_APIENTRY kdQueuePopHeadVEN(KDQueueVEN *queue)
{
    __KDQueueNode *node = KD_NULL;
    void *value = KD_NULL;

    kdThreadMutexLock(queue->mutex);
    if(queue->head) 
    {
        node = queue->head;
        if((queue->head = node->next) != KD_NULL) 
        {
            queue->head->prev = KD_NULL;
        }
        if(queue->tail == node) 
        {
            queue->tail = KD_NULL;
        }
        queue->size--;
    }
    kdThreadMutexUnlock(queue->mutex);

    if(node)
    {
        value = node->value;
        kdFree(node);
    }

    return value;
}

KD_API void* KD_APIENTRY kdQueuePopTailVEN(KDQueueVEN *queue)
{
    __KDQueueNode *node = KD_NULL;
    void *value = KD_NULL;

    kdThreadMutexLock(queue->mutex);
    if(queue->head)
    {
        node = queue->tail;
        if((queue->tail = node->prev) != KD_NULL)
        {
            queue->tail->next = KD_NULL;
        }
        if(queue->head == node)
        {
            queue->head = KD_NULL;
        }
        queue->size--;
    }
    kdThreadMutexUnlock(queue->mutex);

    if(node)
    {
        value = node->value;
        kdFree(node);
    }

    return value;
}

/******************************************************************************
 * String and memory functions (extensions)
 ******************************************************************************/

/* kdStrstr: Locate substring. */
KD_API KDchar* KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2)
{
    const KDchar * p1 = str1;
    const KDchar * p2;
    while (*str1)
    {
        p2 = str2;
        while (*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }
        if (!*p2)
        {
            return (KDchar *)str1;
        }
        ++str1;
        p1 = str1;
    }
    return KD_NULL;
}
