/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2015 Kevin Schmidt
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
 * - KD_EVENT_QUIT events received by threads other then the mainthread
 *   only exit the thread
 * - Android needs this in its manifest for libKD to get orientation changes:
 *   android:configChanges="orientation|keyboardHidden|screenSize"
 *
 ******************************************************************************/

/******************************************************************************
 * Header workarounds
 ******************************************************************************/

#define __STDC_WANT_LIB_EXT1__ 1

#ifdef __unix__
    #ifdef __linux__
        #define _GNU_SOURCE
    #endif
    #include <sys/param.h>
    #ifdef BSD
        #define _BSD_SOURCE
    #endif
#endif

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif
#if !defined(__has_include)
#define __has_include(x) 0
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/ATX_keyboard.h>
#include <KD/LKD_atomic.h>
#include <KD/LKD_guid.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

/******************************************************************************
 * C11 includes
 ******************************************************************************/

#include <errno.h>
/* XSI streams are optional and not supported by MinGW */
#if defined(__MINGW32__)
#define ENODATA    61
#endif

#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __STDC_VERSION__ >= 201112L
    #if !__STDC_NO_THREADS__ && __has_include(<threads.h>)
        #include <threads.h>
        #define KD_THREAD_C11
    #else
        #include <pthread.h>
        #define KD_THREAD_POSIX
    #endif

    #if !__STDC_NO_ATOMICS__
        #if __has_include(<stdatomic.h>)
            #ifdef __ANDROID__
                /* Some NDK versions are missing these. */
                typedef uint32_t char32_t;
                typedef uint16_t char16_t;
            #endif
            #include <stdatomic.h>
            #define KD_ATOMIC_C11
        #elif defined (__clang__) && __has_feature(c_atomic)
            /* Some versions of clang are missing the header. */
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
            #define KD_ATOMIC_C11
        #endif
    #endif

    #if !defined(__STDC_LIB_EXT1__) && !defined(_MSC_VER)
        size_t strlcpy(char *dst, const char *src, size_t dstsize);
        size_t strlcat(char *dst, const char *src, size_t dstsize);
        #define strncat_s(buf, buflen, src, srcmaxlen) strlcat(buf, src, buflen)
        #define strncpy_s(buf, buflen, src, srcmaxlen) strlcpy(buf, src, buflen)
        #define strcpy_s(buf, buflen, src) strlcpy(buf, src, buflen)
    #endif
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#ifdef __unix__
    #include <unistd.h>

    #ifdef _POSIX_VERSION
        #include <sys/stat.h>
        #include <sys/syscall.h>
        #include <sys/utsname.h>
        #include <sys/vfs.h>
        #include <fcntl.h>
        #include <dirent.h>
        #include <dlfcn.h>
        #include <time.h>

        /* POSIX reserved but OpenKODE uses this */
        #undef st_mtime
    #endif

    #ifdef __ANDROID__
        #include <android/log.h>
        #include <android/native_activity.h>
        #include <android/native_window.h>
        #include <android/window.h>
    #else
        #ifdef KD_WINDOW_SUPPORTED
            #include <X11/Xlib.h>
            #include <X11/Xutil.h>
        #endif
    #endif
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define KD_ATOMIC_WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
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
    #if defined(__MINGW32__)
    #include <time.h>
    #endif
	/* MSVC has an internal C11 threads version which is based on an earlier draft. */
    #if !defined(__MINGW32__)
        #include <thr/threads.h>
        #define KD_THREAD_C11
        #define _Thread_local __declspec(thread)
        #undef thrd_sleep
        int thrd_sleep(const struct timespec* time_point, struct timespec* remaining)
        {
            const struct xtime time = { .sec = time_point->tv_sec, .nsec = time_point->tv_nsec};
            _Thrd_sleep(&time);
            return 0;
        }
        typedef struct {
            LONG volatile status;
            CRITICAL_SECTION lock;
        } once_flag;
        static void call_once(once_flag *flag, void(*func)(void))
        {
            while (flag->status < 3)
            {
                switch (flag->status)
                {
                case 0:
                    if (InterlockedCompareExchange(&(flag->status), 1, 0) == 0) {
                        InitializeCriticalSection(&(flag->lock));
                        EnterCriticalSection(&(flag->lock));
                        flag->status = 2;
                        func();
                        flag->status = 3;
                        LeaveCriticalSection(&(flag->lock));
                        return;
                    }
                    break;
                case 1:
                    break;
                case 2:
                    EnterCriticalSection(&(flag->lock));
                    LeaveCriticalSection(&(flag->lock));
                    break;
                }
            }
        }
    #endif

	/* MSVC redefinition fix*/
	#ifndef inline
	#define inline __inline
	#endif
#endif

/******************************************************************************
* Middleware
******************************************************************************/

#ifdef KD_VFS_SUPPORTED
    #include <physfs.h>
#endif

/******************************************************************************
 * Internal eventqueue
 ******************************************************************************/

/* __KDQueue (lock-less FIFO)*/
typedef struct __KDQueueItem {
	SLIST_ENTRY entry;
	void* data;
} __KDQueueItem;

typedef struct __KDQueue {
	PSLIST_ENTRY	firstentry;
	PSLIST_HEADER	head;
} __KDQueue;

static __KDQueue *__kdQueueCreate(void)
{
	__KDQueue *queue = (__KDQueue*)kdMalloc(sizeof(__KDQueue));
	queue->head = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
	InitializeSListHead(queue->head);
	return queue;
}


static inline void __kdQueueFree(__KDQueue *queue)
{
    if(queue)
    {
		InterlockedFlushSList(queue->head);
		_aligned_free(queue->head);
		kdFree(queue);
		queue = KD_NULL;
    }
}

static inline KDint __kdQueueSize(__KDQueue *queue)
{
	return QueryDepthSList(queue->head);
}

static KDint  __kdQueuePost(__KDQueue *queue, void *value)
{
	__KDQueueItem *item = (__KDQueueItem*)_aligned_malloc(sizeof(__KDQueueItem), MEMORY_ALLOCATION_ALIGNMENT);
	item->data = value;
	queue->firstentry = InterlockedPushEntrySList(queue->head, &(item->entry));
    return 0;
}

void *__kdQueueGet(__KDQueue *queue)
{
	void* value = KD_NULL;
	__KDQueueItem *item = (__KDQueueItem *)InterlockedPopEntrySList(queue->head);
	if(item)
	{
		value = item->data;
		_aligned_free(item);
		return value;
	}
	return KD_NULL;
}

/******************************************************************************
 * Errors
 ******************************************************************************/
typedef struct
{
    KDint       errorcode_kd;
    int         errorcode;
    const char *errorcode_text;
} __KDErrors;

static __KDErrors errorcodes_posix[] =
{
    { KD_EACCES,            EACCES,         "Permission denied."},
    { KD_EADDRINUSE,        EADDRINUSE,     "Adress in use."},
    { KD_EADDRNOTAVAIL,     EADDRNOTAVAIL,  "Address not available on the local platform."},
    { KD_EAFNOSUPPORT,      EAFNOSUPPORT,   "Address family not supported."},
    { KD_EAGAIN,            EAGAIN,         "Resource unavailable, try again."},
    { KD_EALREADY,          EALREADY,       "A connection attempt is already in progress for this socket."},
    { KD_EBADF,             EBADF,          "File not opened in the appropriate mode for the operation."},
    { KD_EBUSY,             EBUSY,          "Device or resource busy."},
    { KD_ECONNREFUSED,      ECONNREFUSED,   "Connection refused."},
    { KD_ECONNRESET,        ECONNRESET,     "Connection reset."},
    { KD_EDEADLK,           EDEADLK,        "Resource deadlock would occur."},
    { KD_EDESTADDRREQ,      EDESTADDRREQ,   "Destination address required."},
    { KD_EEXIST,            EEXIST,         "File exists."},
    { KD_EFBIG,             EFBIG,          "File too large."},
    { KD_EHOSTUNREACH,      EHOSTUNREACH,   "Host is unreachable."},
    { KD_EHOST_NOT_FOUND,   0,              "The specified name is not known."}, /* EHOSTNOTFOUND is not standard */
    { KD_EINVAL,            EINVAL,         "Invalid argument."},
    { KD_EIO,               EIO,            "I/O error."},
    { KD_EILSEQ,            EILSEQ,         "Illegal byte sequence."},
    { KD_EISCONN,           EISCONN,        "Socket is connected."},
    { KD_EISDIR,            EISDIR,         "Is a directory."},
    { KD_EMFILE,            EMFILE,         "Too many openfiles."},
    { KD_ENAMETOOLONG,      ENAMETOOLONG,   "Filename too long."},
    { KD_ENOENT,            ENOENT,         "No such file or directory."},
    { KD_ENOMEM,            ENOMEM,         "Not enough space."},
    { KD_ENOSPC,            ENOSPC,         "No space left on device."},
    { KD_ENOSYS,            ENOSYS,         "Function not supported."},
    { KD_ENOTCONN,          ENOTCONN,       "The socket is not connected."},
    { KD_ENO_DATA,          ENODATA,        "The specified name is valid but does not have an address."},
    { KD_ENO_RECOVERY,      0,              "A non-recoverable error has occurred on the name server."}, /* ENORECOVERY is not standard */
    { KD_EOPNOTSUPP,        EOPNOTSUPP,     "Operation not supported."},
    { KD_EOVERFLOW,         EOVERFLOW,      "Overflow."},
    { KD_EPERM,             EPERM,          "Operation not permitted."},
    { KD_ERANGE,            ERANGE,         "Result out of range."},
    { KD_ETIMEDOUT,         ETIMEDOUT,      "Connection timed out."},
    { KD_ETRY_AGAIN,        0,              "A temporary error has occurred on an authoratitive name server, and the lookup may succeed if retried later."}, /* ETRYAGAIN is not standard */
};

KDint __kdTranslateError(int errorcode)
{
    for (KDuint i = 0; i < sizeof(errorcodes_posix) / sizeof(errorcodes_posix[0]); i++)
    {
        if (errorcodes_posix[i].errorcode == errorcode)
        {
            return errorcodes_posix[i].errorcode_kd;
        }
    }
    return 0;
}

static _Thread_local KDint lasterror = 0;
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
KD_API KDint KD_APIENTRY kdQueryAttribi(KDint attribute, KDint *value)
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
        return "1.0.3 (libKD 0.1-alpha)";
    }
    else if(attribute == KD_ATTRIB_PLATFORM)
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
		return "Windows";
#elif __unix__
		static struct utsname name;
		uname(&name);
		return name.sysname;
#endif
    }
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/* kdQueryIndexedAttribcv: Obtain the value of an indexed string OpenKODE Core attribute. */
KD_API const KDchar *KD_APIENTRY kdQueryIndexedAttribcv(KDint attribute, KDint index)
{
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/******************************************************************************
 * Threads and synchronization
 ******************************************************************************/

/* kdThreadAttrCreate: Create a thread attribute object. */
typedef struct KDThreadAttr
{
    KDint detachstate;
    KDsize stacksize;
} KDThreadAttr;
KD_API KDThreadAttr *KD_APIENTRY kdThreadAttrCreate(void)
{
    KDThreadAttr *attr = (KDThreadAttr*)kdMalloc(sizeof(KDThreadAttr));
    /* Spec default */
    attr->detachstate = KD_THREAD_CREATE_JOINABLE;
    /* Impl default */
    attr->stacksize = 100000;
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
    return 0;
}


/* kdThreadAttrSetStackSize: Set stacksize attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetStackSize(KDThreadAttr *attr, KDsize stacksize)
{
    attr->stacksize = stacksize;
    return 0;
}

/* kdThreadCreate: Create a new thread. */
static _Thread_local KDThread *__kd_thread = KD_NULL;
typedef struct KDThread
{
#if defined(KD_THREAD_C11)
    thrd_t nativethread;
#elif defined(KD_THREAD_POSIX)
    pthread_t nativethread;
#endif
} KDThread;
typedef struct __KDThreadStartArgs
{
    void *(*start_routine)(void *);
    void *arg;
    KDThread *thread;
} __KDThreadStartArgs;
static void* __kdThreadStart(void *args)
{
    __KDThreadStartArgs* start_args = (__KDThreadStartArgs*)args;
    __kd_thread = start_args->thread;
    return start_args->start_routine(start_args->arg);
}
KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
    KDThread *thread = (KDThread *)kdMalloc(sizeof(KDThread));
    __KDThreadStartArgs *start_args = (__KDThreadStartArgs*)kdMalloc(sizeof(__KDThreadStartArgs));
    start_args->start_routine = start_routine;
    start_args->arg = arg;
    start_args->thread = thread;

	KDint error = 0;
#if defined(KD_THREAD_C11)
	error = thrd_create(&thread->nativethread, (thrd_start_t)__kdThreadStart, start_args);
#elif defined(KD_THREAD_POSIX)
	/* TODO: Fix thread attributes. */
	error = pthread_create(&thread->nativethread, KD_NULL, __kdThreadStart, start_args);
#else
	kdAssert(0);
#endif
	if (error != 0)
	{
		kdSetError(KD_EAGAIN);
        kdFree(thread);
        return KD_NULL;
	}

	if (attr != KD_NULL)
	{
		if (attr->detachstate == KD_THREAD_CREATE_DETACHED)
		{
			kdThreadDetach(thread);
			return KD_NULL;
		}
	}

    return thread;
}

/* __kdThreadEqual: Compare two threads. */
static KDboolean __kdThreadEqual(KDThread *first, KDThread *second)
{
#if defined(KD_THREAD_C11)
    return thrd_equal(first->nativethread, second->nativethread);
#elif defined(KD_THREAD_POSIX)
    return pthread_equal(first->nativethread, second->nativethread);
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadExit: Terminate this thread. */
KD_API KD_NORETURN void KD_APIENTRY kdThreadExit(void *retval)
{
#if defined(KD_THREAD_C11)
	KDint result = 0;
	if (retval != KD_NULL)
	{
		result = *(KDint*)retval;
	}
	thrd_exit(result);
#elif defined(KD_THREAD_POSIX)
	pthread_exit(retval);
#else
	kdAssert(0);
#endif
}

/* kdThreadJoin: Wait for termination of another thread. */
KD_API KDint KD_APIENTRY kdThreadJoin(KDThread *thread, void **retval)
{
#if defined(KD_THREAD_C11)
    KDint* result = KD_NULL;
    if(retval != KD_NULL)
    {
        result = *retval;
    }
    int error = thrd_join(thread->nativethread, result);
    if(error == thrd_error)
#elif defined(KD_THREAD_POSIX)
    int error = pthread_join(thread->nativethread, retval);
    if(error == EINVAL || error == ESRCH)
#else
    kdAssert(0);
#endif
	{
		kdSetError(KD_EINVAL);
		return -1;
	}
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
    error = pthread_detach(thread->nativethread);
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
#else
    kdAssert(0);
#endif
    return 0;
}
#endif /* ndef KD_NO_STATIC_DATA */

/* kdThreadMutexCreate: Create a mutex. */
typedef struct KDThreadMutex
{
#if defined(KD_THREAD_C11)
    mtx_t nativemutex;
#elif defined(KD_THREAD_POSIX)
    pthread_mutex_t nativemutex;
#endif
} KDThreadMutex;
KD_API KDThreadMutex *KD_APIENTRY kdThreadMutexCreate(const void *mutexattr)
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
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadCondCreate: Create a condition variable. */
typedef struct KDThreadCond
{
#if defined(KD_THREAD_C11)
    cnd_t nativecond;
#elif defined(KD_THREAD_POSIX)
    pthread_cond_t nativecond;
#endif
} KDThreadCond;
KD_API KDThreadCond *KD_APIENTRY kdThreadCondCreate(const void *attr)
{
    KDThreadCond *cond = (KDThreadCond*)kdMalloc(sizeof(KDThreadCond));
    KDint error = 0;
#if defined(KD_THREAD_C11)
    error =  cnd_init(&cond->nativecond);
    if(error ==  thrd_nomem)
    {
        kdSetError(KD_ENOMEM);
    }
    else if(error == thrd_error)
#elif defined(KD_THREAD_POSIX)
    error = pthread_cond_init(&cond->nativecond, KD_NULL);
    if (error != 0)
#else
    kdAssert(0);
#endif
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
#else
    kdAssert(0);
#endif
    return 0;
}

/* kdThreadSemCreate: Create a semaphore. */
typedef struct KDThreadSem
{
    KDuint          count;
    KDThreadMutex*   mutex;
    KDThreadCond*    condition;
} KDThreadSem;
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
static __KDQueue *__kd_eventqueue = KD_NULL;
typedef struct __KDEventWrapper
{
    KDEvent* event;
    KDThread* thread;
} __KDEventWrapper;

/* __KDEventGet: Get event meant for the thread from global eventqeue. */
KDEvent* __kdEventGet(KDThread* thread)
{
    KDEvent *event = KD_NULL;
    __KDEventWrapper* eventwrapper = (__KDEventWrapper*)__kdQueueGet(__kd_eventqueue);
    if (eventwrapper != KD_NULL)
    {
        if(__kdThreadEqual(eventwrapper->thread, thread))
        {
            event = eventwrapper->event;
            kdFree(eventwrapper);
        }
        else
        {
            __kdQueuePost(__kd_eventqueue, (void*)eventwrapper);
        }
    }
    return event;
}

/* __KDEventPost: Post event to global eventqeue meant for a specific thread. */
void __kdEventPost(KDEvent* event, KDThread* thread)
{
    __KDEventWrapper *eventwrapper = (__KDEventWrapper*)kdMalloc(sizeof(__KDEventWrapper));
    eventwrapper->event = event;
    eventwrapper->thread = thread;
    __kdQueuePost(__kd_eventqueue, (void*)eventwrapper);
}

/* __KDSleep: Sleep for nanoseconds. */
void __KDSleep(KDust timeout)
{
    struct timespec ts = {0};
    /* Determine seconds from the overall nanoseconds */
    if ((timeout % 1000000000) == 0)
    {
        ts.tv_sec = timeout / 1000000000;
    }
    else
    {
        ts.tv_sec = (timeout - (timeout % 1000000000)) / 1000000000;
    }
    /* Remaining nanoseconds */
    ts.tv_nsec = timeout - (ts.tv_sec * 1000000000);

#if defined(KD_THREAD_C11)
    thrd_sleep(&ts, NULL);
#elif defined(KD_THREAD_POSIX)
    nanosleep(&ts, NULL);
#else
    kdAssert(0);
#endif
}

/* kdWaitEvent: Get next event from thread's event queue. */
static _Thread_local KDEvent *__kd_lastevent = KD_NULL;
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    if(__kd_lastevent != KD_NULL)
    {
        kdFreeEvent(__kd_lastevent);
    }
    if(timeout != -1)
    {
        __KDSleep(timeout);
    }
    kdPumpEvents();
    __kd_lastevent = __kdEventGet(kdThreadSelf());
    return __kd_lastevent;
}
/* kdSetEventUserptr: Set the userptr for global events. */
KD_API void KD_APIENTRY kdSetEventUserptr(void *userptr)
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
typedef struct KDCallback 
{
    KDCallbackFunc *func;
    KDint eventtype;
    void *eventuserptr;
} KDCallback;
#ifdef KD_WINDOW_SUPPORTED
#ifdef __ANDROID__
typedef struct KDWindowAndroid
{
    struct ANativeWindow *window;
    void *display;
} KDWindowAndroid;
#elif __unix__
typedef struct KDWindowX11
{
    Window window;
    Display *display;
} KDWindowX11;
#endif
typedef struct KDWindow
{
    void *nativewindow;
    EGLint format;
} KDWindow;
static _Thread_local KDWindow windows[999]= {{0}};
#endif
static _Thread_local KDuint __kd_callbacks_index = 0;
static _Thread_local KDCallback __kd_callbacks[999] = {{0}};
static KDboolean __kdExecCallback(KDEvent* event)
{
    for (KDuint callback = 0; callback < __kd_callbacks_index; callback++)
    {
        if(__kd_callbacks[callback].func != KD_NULL)
        {
            KDboolean typematch = __kd_callbacks[callback].eventtype == event->type || __kd_callbacks[callback].eventtype == 0;
            KDboolean userptrmatch = __kd_callbacks[callback].eventuserptr == event->userptr;
            if (typematch && userptrmatch)
            {
                __kd_callbacks[callback].func(event);
                kdFreeEvent(event);
                return 1;
            }
        }
    }
    return 0;
}

#ifdef __ANDROID__
static AInputQueue *__kd_androidinputqueue = KD_NULL;
static KDThreadMutex *__kd_androidinputqueue_mutex = KD_NULL;
#endif
KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
#ifdef __EMSCRIPTEN__
    /* Give back control to the browser */
    emscripten_sleep(1);
#endif
    KDsize queuesize = __kdQueueSize(__kd_eventqueue);
    if(queuesize > 0)
    {
        for (KDuint i = 0; i < 10; i++)
        {
            KDEvent *callbackevent = __kdEventGet(kdThreadSelf());
            if (callbackevent != KD_NULL)
            {
                if (!__kdExecCallback(callbackevent))
                {
                    /* Not a callback */
                    kdPostEvent(callbackevent);
                }
            }
        }
    }
#ifdef KD_WINDOW_SUPPORTED
#ifdef __ANDROID__
    AInputEvent* aevent = NULL;
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    if(__kd_androidinputqueue != KD_NULL)
    {
        while (AInputQueue_getEvent(__kd_androidinputqueue, &aevent) >= 0)
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
#elif __unix__
    for(KDuint window = 0; window < sizeof(windows) / sizeof(windows[0]); window++)
    {
        if(windows[window].nativewindow)
        {
            KDWindowX11 *x11window = windows[window].nativewindow;
            XSelectInput(x11window->display, x11window->window, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
            while(XPending(x11window->display) > 0)
            {
                KDEvent *event = kdCreateEvent();
                XEvent xevent = {0};
                XNextEvent(x11window->display,&xevent);switch(xevent.type)
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
                    case KeyPress:
                    {
                        KeySym keysym;
                        XLookupString(&xevent.xkey, NULL, 25, &keysym, NULL);
                        event->type = KD_EVENT_INPUT_KEY_ATX;
                        KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&event->data);
                        switch(keysym)
                        {
                            case(XK_Up):
                            {

                                keyevent->keycode = KD_KEY_UP_ATX;
                                break;
                            }
                            case(XK_Down):
                            {

                                keyevent->keycode = KD_KEY_DOWN_ATX;
                                break;
                            }
                            case(XK_Left):
                            {

                                keyevent->keycode = KD_KEY_LEFT_ATX;
                                break;
                            }
                            case(XK_Right):
                            {

                                keyevent->keycode = KD_KEY_RIGHT_ATX;
                                break;
                            }
                            default:
                            {
                                event->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                                KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *) (&event->data);
                                keycharevent->character = (KDint32) keysym;
                                break;
                            }
                        }
                        if(!__kdExecCallback(event))
                        {
                            kdPostEvent(event);
                        }
                    }
                    case KeyRelease:
                    {
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
                        if((Atom)xevent.xclient.data.l[0] == XInternAtom(x11window->display, "WM_DELETE_WINDOW", False))
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
	/* Undefined behavior */
	if(thread == KD_NULL)
	{
	}
    if(event->timestamp == 0)
    {
        event->timestamp = kdGetTimeUST();
    }
    __kdEventPost(event, thread);
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
const char* __kdAppName(const char *argv0)
{
#ifdef __GLIBC__
    return __progname;
#endif
    /* TODO: argv[0] is not a reliable way to get the appname */
    if(argv0 == KD_NULL)
    {
        return "";
    }
    return argv0;
}

#ifdef __ANDROID__
/* All Android events are send to the mainthread */
static KDThread *__kd_androidmainthread = KD_NULL;
static ANativeActivity *__kd_androidactivity =  KD_NULL;
static KDThreadMutex *__kd_androidactivity_mutex =  KD_NULL;
static void __kd_AndroidOnDestroy(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_QUIT;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnStart(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnResume(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_androidmainthread);
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
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnStop(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_PAUSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnConfigurationChanged(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_ORIENTATION;
    kdPostThreadEvent(event, __kd_androidmainthread);
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
    kdPostThreadEvent(event, __kd_androidmainthread);
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
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = KD_NULL;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_CLOSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
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

typedef struct __KDMainArgs
{
    int argc;
    char **argv;
} __KDMainArgs;
static void* __kdMainInjector( void *arg)
{
    __KDMainArgs *mainargs = (__KDMainArgs *) arg;
    if (mainargs == KD_NULL)
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

#ifdef __ANDROID__
    __kd_androidmainthread = __kd_thread;
    kdMain(mainargs->argc, (const KDchar *const *)mainargs->argv);
    kdThreadMutexFree(__kd_androidactivity_mutex);
    kdThreadMutexFree(__kd_androidwindow_mutex);
    kdThreadMutexFree(__kd_androidinputqueue_mutex);
    __kdQueueFree(__kd_eventqueue);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    kdMain(mainargs->argc, (const KDchar *const *)mainargs->argv);
#else
    typedef KDint(*KDMAIN)(KDint argc, const KDchar *const *argv);
	KDMAIN kdMain = KD_NULL;
    void *app = dlopen(NULL, RTLD_NOW);
    /* ISO C forbids assignment between function pointer and ‘void *’ */
    void *rawptr = dlsym(app, "kdMain");
    kdMemcpy(&kdMain, &rawptr, sizeof(rawptr));
    if(dlerror() != NULL)
    {
        kdLogMessage("Cant dlopen self. Dont strip symbols from me.\n");
        kdAssert(0);
    }
    (*kdMain)(mainargs->argc, (const KDchar *const *)mainargs->argv);
#endif

#ifdef KD_VFS_SUPPORTED
    PHYSFS_deinit();
#endif
    return 0;
}

#ifdef __ANDROID__
void ANativeActivity_onCreate(ANativeActivity *activity, void* savedState, size_t savedStateSize)
{
    __kd_eventqueue = __kdQueueCreate();
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
    KDThread thread = kdThreadCreate(KD_NULL, __kdMainInjector, &mainargs);
    kdThreadDetach(thread);
}
#else
KD_API int main(int argc, char **argv)
{
    __kd_eventqueue = __kdQueueCreate();
    __KDMainArgs mainargs = {0};
    mainargs.argc = argc;
    mainargs.argv = argv;
    KDThread * mainthread = kdThreadCreate(KD_NULL, __kdMainInjector, &mainargs);
    kdThreadJoin(mainthread, KD_NULL);
    __kdQueueFree(__kd_eventqueue);
    return 0;
}
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
    return abs(i);
}

/* kdStrtof: Convert a string to a floating point number. */
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
KD_API KDint KD_APIENTRY kdCryptoRandom(KDuint8 *buf, KDsize buflen)
{
#ifdef __OpenBSD__
    return getentropy(buf, buflen);
#endif

#ifdef __EMSCRIPTEN__
    for(KDsize i = 0; i < buflen; i++)
    {
        buf[i] = EM_ASM_INT_V
        (
            var array = new Uint8Array(1);
            window.crypto.getRandomValues(array);
            return array;
        );
    }
    return 0;
#endif

#ifdef __unix__
    FILE *urandom = fopen("/dev/urandom", "r");
    KDsize result = fread((void *)buf, sizeof(KDuint8), buflen, urandom);
    fclose(urandom);
    if (result != buflen)
    {
        kdSetError(KD_ENOMEM);
        return -1;
    }
    return 0;
#endif

    /* TODO: Implement kdCryptoRandom on Windows*/
    kdAssert(0);
    return 0;
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
#ifdef GC_DEBUG
    return GC_MALLOC(size);
#else
    return malloc(size);
#endif
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

static _Thread_local void* tlsptr = KD_NULL;
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
 ******************************************************************************/

 /* kdMemchr: Scan memory for a byte value. */
KD_API void *KD_APIENTRY kdMemchr(const void *src, KDint byte, KDsize len)
{
    void *retval = memchr(src , byte, len);
    if(retval == NULL)
    {
        kdSetError(__kdTranslateError(errno));
        return KD_NULL;
    }
    return retval;
}

/* kdMemcmp: Compare two memory regions. */
KD_API KDint KD_APIENTRY kdMemcmp(const void *src1, const void *src2, KDsize len)
{
    return memcmp(src1, src2, len);
}

/* kdMemcpy: Copy a memory region, no overlapping. */
KD_API void *KD_APIENTRY kdMemcpy(void *buf, const void *src, KDsize len)
{
    return memcpy(buf, src, len);
}

/* kdMemmove: Copy a memory region, overlapping allowed. */
KD_API void *KD_APIENTRY kdMemmove(void *buf, const void *src, KDsize len)
{
    return memmove(buf, src, len);
}

/* kdMemset: Set bytes in memory to a value. */
KD_API void *KD_APIENTRY kdMemset(void *buf, KDint byte, KDsize len)
{
    return memset(buf, byte, len);
}

/* kdStrchr: Scan string for a byte value. */
KD_API KDchar *KD_APIENTRY kdStrchr(const KDchar *str, KDint ch)
{
    void *retval = strchr(str, ch);
    if(retval == NULL)
    {
        kdSetError(__kdTranslateError(errno));
        return KD_NULL;
    }
    return retval;
}

/* kdStrcmp: Compares two strings. */
KD_API KDint KD_APIENTRY kdStrcmp(const KDchar *str1, const KDchar *str2)
{
    return strcmp(str1, str2);
}

/* kdStrlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrlen(const KDchar *str)
{
    return strlen(str);
}

/* kdStrnlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrnlen(const KDchar *str, KDsize maxlen)
{
    size_t i = 0;
    for(; (i < maxlen) && str[i]; ++i);
    return i;
}

/* kdStrncat_s: Concatenate two strings. */
KD_API KDint KD_APIENTRY kdStrncat_s(KDchar *buf, KDsize buflen, const KDchar *src, KDsize srcmaxlen)
{
    return (KDint)strncat_s(buf, buflen, src, srcmaxlen);
}

/* kdStrncmp: Compares two strings with length limit. */
KD_API KDint KD_APIENTRY kdStrncmp(const KDchar *str1, const KDchar *str2, KDsize maxlen)
{
    return strncmp(str1, str2, maxlen);
}

/* kdStrcpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrcpy_s(KDchar *buf, KDsize buflen, const KDchar *src)
{
    return (KDint)strcpy_s(buf, buflen, src);
}

/* kdStrncpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrncpy_s(KDchar *buf, KDsize buflen, const KDchar *src, KDsize srclen)
{
    return (KDint)strncpy_s(buf, buflen, src, srclen);
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
typedef struct __KDTimerPayload
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
typedef struct KDTimer
{
    KDThread    *thread;
    __KDTimerPayload* payload;
} KDTimer;
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
typedef struct KDFile
{
#ifdef KD_VFS_SUPPORTED
    PHYSFS_File *file;
#else
    FILE *file;
#endif
} KDFile;
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
    KDuint  seekorigin_kd;
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
        if (seekorigins_c[i].seekorigin_kd == origin)
        {
			retval = fseek(file->file, offset, seekorigins_c[i].seekorigin);
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
KD_API KDint KD_APIENTRY kdTruncate(const KDchar *pathname, KDoff length)
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
        if (accessmode[i].accessmode_kd == amode)
        {
            retval = access(pathname, accessmode[i].accessmode);
            break;
        }
    }
#endif
    return retval;
}

/* kdOpenDir: Open a directory ready for listing. */
typedef struct KDDir
{
#ifdef KD_VFS_SUPPORTED
    char **dir;
#else
    DIR *dir;
#endif
} KDDir;
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
static _Thread_local KDDirent *__kd_lastdirent = KD_NULL;
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
KD_API KDint KD_APIENTRY kdNameLookup(KDint af, const KDchar *hostname, void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdNameLookupCancel: Selectively cancels ongoing kdNameLookup operations. */
KD_API void KD_APIENTRY kdNameLookupCancel(void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
}

/* kdSocketCreate: Creates a socket. */
typedef struct KDSocket
{
    int placebo;
} KDSocket;
KD_API KDSocket *KD_APIENTRY kdSocketCreate(KDint type, void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/* kdSocketClose: Closes a socket. */
KD_API KDint KD_APIENTRY kdSocketClose(KDSocket *socket)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketBind: Bind a socket. */
KD_API KDint KD_APIENTRY kdSocketBind(KDSocket *socket, const struct KDSockaddr *addr, KDboolean reuse)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketGetName: Get the local address of a socket. */
KD_API KDint KD_APIENTRY kdSocketGetName(KDSocket *socket, struct KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketConnect: Connects a socket. */
KD_API KDint KD_APIENTRY kdSocketConnect(KDSocket *socket, const KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketListen: Listen on a socket. */
KD_API KDint KD_APIENTRY kdSocketListen(KDSocket *socket, KDint backlog)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketAccept: Accept an incoming connection. */
KD_API KDSocket *KD_APIENTRY kdSocketAccept(KDSocket *socket, KDSockaddr *addr, void *eventuserptr)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/* kdSocketSend, kdSocketSendTo: Send data to a socket. */
KD_API KDint KD_APIENTRY kdSocketSend(KDSocket *socket, const void *buf, KDint len)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdSocketSendTo(KDSocket *socket, const void *buf, KDint len, const KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdSocketRecv, kdSocketRecvFrom: Receive data from a socket. */
KD_API KDint KD_APIENTRY kdSocketRecv(KDSocket *socket, void *buf, KDint len)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdSocketRecvFrom(KDSocket *socket, void *buf, KDint len, KDSockaddr *addr)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdHtonl: Convert a 32-bit integer from host to network byte order. */
KD_API KDuint32 KD_APIENTRY kdHtonl(KDuint32 hostlong)
{
    kdSetError(KD_EOPNOTSUPP);
    return 0;
}

/* kdHtons: Convert a 16-bit integer from host to network byte order. */
KD_API KDuint16 KD_APIENTRY kdHtons(KDuint16 hostshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return (KDuint16)0;
}

/* kdNtohl: Convert a 32-bit integer from network to host byte order. */
KD_API KDuint32 KD_APIENTRY kdNtohl(KDuint32 netlong)
{
    kdSetError(KD_EOPNOTSUPP);
    return 0;
}

/* kdNtohs: Convert a 16-bit integer from network to host byte order. */
KD_API KDuint16 KD_APIENTRY kdNtohs(KDuint16 netshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return (KDuint16)0;
}

/* kdInetAton: Convert a &#8220;dotted quad&#8221; format address to an integer. */
KD_API KDint KD_APIENTRY kdInetAton(const KDchar *cp, KDuint32 *inp)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdInetNtop: Convert a network address to textual form. */
KD_API const KDchar *KD_APIENTRY kdInetNtop(KDuint af, const void *src, KDchar *dst, KDsize cnt)
{
    kdSetError(KD_EOPNOTSUPP);
    return KD_NULL; 
}

/******************************************************************************
 * Input/output
 ******************************************************************************/
/* kdStateGeti, kdStateGetl, kdStateGetf: get state value(s) */
KD_API KDint KD_APIENTRY kdStateGeti(KDint startidx, KDuint numidxs, KDint32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;  
}

KD_API KDint KD_APIENTRY kdStateGetl(KDint startidx, KDuint numidxs, KDint64 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdStateGetf(KDint startidx, KDuint numidxs, KDfloat32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}


/* kdOutputSeti, kdOutputSetf: set outputs */
KD_API KDint KD_APIENTRY kdOutputSeti(KDint startidx, KDuint numidxs, const KDint32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

KD_API KDint KD_APIENTRY kdOutputSetf(KDint startidx, KDuint numidxs, const KDfloat32 *buffer)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/******************************************************************************
 * Windowing
 ******************************************************************************/
#ifdef KD_WINDOW_SUPPORTED

/* kdCreateWindow: Create a window. */
KD_API KDWindow *KD_APIENTRY kdCreateWindow(EGLDisplay display, EGLConfig config, void *eventuserptr)
{
    KDWindow *window = (KDWindow *) kdMalloc(sizeof(KDWindow));
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->format);
#ifdef __ANDROID__
    window->nativewindow = kdMalloc(sizeof(KDWindowAndroid));
#elif __unix__
    window->nativewindow = kdMalloc(sizeof(KDWindowX11));
    KDWindowX11 *x11window = window->nativewindow;
    XInitThreads();
    x11window->display = XOpenDisplay(NULL);
    x11window->window = XCreateSimpleWindow(x11window->display,
            XRootWindow(x11window->display, XDefaultScreen(x11window->display)), 0, 0,
            (KDuint)XWidthOfScreen(XDefaultScreenOfDisplay(x11window->display)),
            (KDuint)XHeightOfScreen(XDefaultScreenOfDisplay(x11window->display)), 0,
            XBlackPixel(x11window->display, XDefaultScreen(x11window->display)),
            XWhitePixel(x11window->display, XDefaultScreen(x11window->display)));
    XStoreName(x11window->display, x11window->window, "OpenKODE");
    Atom wm_del_win_msg = XInternAtom(x11window->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x11window->display, x11window->window, &wm_del_win_msg, 1);
    Atom mwm_prop_hints = XInternAtom(x11window->display, "_MOTIF_WM_HINTS", True);
    long mwm_hints[5] = {2, 0, 0, 0, 0};
    XChangeProperty(x11window->display, x11window->window, mwm_prop_hints, mwm_prop_hints, 32, 0, (const unsigned char *) &mwm_hints, 5);
    Atom netwm_prop_hints = XInternAtom(x11window->display, "_NET_WM_STATE", False);
    Atom netwm_hints[3];
    netwm_hints[0] = XInternAtom(x11window->display, "_NET_WM_STATE_FULLSCREEN", False);
    netwm_hints[1] = XInternAtom(x11window->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    netwm_hints[2] = XInternAtom(x11window->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    netwm_hints[2] = XInternAtom(x11window->display, "_NET_WM_STATE_FOCUSED", False);
    XChangeProperty(x11window->display, x11window->window, netwm_prop_hints, 4, 32, 0, (const unsigned char *) &netwm_hints, 3);
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_REDRAW;
    kdPostThreadEvent(event, kdAtomicPtrLoad(__kd_threads[0]));
#endif
    for (KDuint i = 0; i < sizeof(windows) / sizeof(windows[0]); i++)
    {
        if (!windows[i].nativewindow)
        {
            windows[i].nativewindow = window->nativewindow;
            windows[i].format = window->format;
            break;
        }
    }
    return window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
#ifdef __ANDROID__
    KDWindowAndroid *androidwindow = window->nativewindow;
    kdFree(androidwindow);
#elif __unix__
    KDWindowX11 *x11window = window->nativewindow;
    XCloseDisplay(x11window->display);
    kdFree(x11window);
#endif
    kdFree(window);
    return 0;
}

/* kdSetWindowPropertybv, kdSetWindowPropertyiv, kdSetWindowPropertycv: Set a window property to request a change in the on-screen representation of the window. */
KD_API KDint KD_APIENTRY kdSetWindowPropertybv(KDWindow *window, KDint pname, const KDboolean *param)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdSetWindowPropertyiv(KDWindow *window, KDint pname, const KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#ifdef __ANDROID__
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif __unix__
        KDWindowX11 *x11window = window->nativewindow;
        XMoveResizeWindow(x11window->display, x11window->window, 0, 0, (KDuint)param[0], (KDuint)param[1]);
        XFlush(x11window->display);
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        kdPostThreadEvent(event, kdThreadSelf());
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdSetWindowPropertycv(KDWindow *window, KDint pname, const KDchar *param)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#ifdef __ANDROID__
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif __unix__
        KDWindowX11 *x11window = window->nativewindow;
        XStoreName(x11window->display, x11window->window, param);
        XFlush(x11window->display);
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
KD_API KDint KD_APIENTRY kdGetWindowPropertybv(KDWindow *window, KDint pname, KDboolean *param)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertyiv(KDWindow *window, KDint pname, KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#ifdef __ANDROID__
        KDWindowAndroid *androidwindow = window->nativewindow;
        param[0] = ANativeWindow_getWidth(androidwindow->window);
        param[1] = ANativeWindow_getHeight(androidwindow->window);
#elif __unix__
        KDWindowX11 *x11window = window->nativewindow;
        param[0] = XWidthOfScreen(XDefaultScreenOfDisplay(x11window->display));
        param[1] = XHeightOfScreen(XDefaultScreenOfDisplay(x11window->display));
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertycv(KDWindow *window, KDint pname, KDchar *param, KDsize *size)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#ifdef __ANDROID__
        kdSetError(KD_EOPNOTSUPP);
        return -1;
#elif __unix__
        KDWindowX11 *x11window = window->nativewindow;
        XFetchName(x11window->display, x11window->window, &param);
#endif
        return 0;
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdRealizeWindow: Realize the window as a displayable entity and get the native window handle for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizeWindow(KDWindow *window, EGLNativeWindowType *nativewindow)
{
#ifdef __ANDROID__
    KDWindowAndroid *androidwindow = window->nativewindow;
    for(;;)
    {
        kdThreadMutexLock(__kd_androidwindow_mutex);
        if(__kd_androidwindow != KD_NULL)
        {
            androidwindow->window = __kd_androidwindow;
            kdThreadMutexUnlock(__kd_androidwindow_mutex);
            break;
        }
        kdThreadMutexUnlock(__kd_androidwindow_mutex);
    }
    ANativeWindow_setBuffersGeometry(androidwindow->window, 0, 0, window->format);
    *nativewindow = androidwindow->window;
#elif __unix__
    KDWindowX11 *x11window = window->nativewindow;
    XMapWindow(x11window->display, x11window->window);
    XFlush(x11window->display);
    *nativewindow = x11window->window;
#endif
    return 0;
}
#endif

/******************************************************************************
 * Assertions and logging
 ******************************************************************************/

/* kdHandleAssertion: Handle assertion failure. */
KD_API void KD_APIENTRY kdHandleAssertion(const KDchar *condition, const KDchar *filename, KDint linenumber)
{

}

/* kdLogMessage: Output a log message. */
#ifndef KD_NDEBUG
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, __kdAppName(KD_NULL), string);
#else
    printf(string);
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
typedef struct KDAtomicInt { _Atomic KDint value; } KDAtomicInt;
typedef struct KDAtomicPtr { _Atomic void *value; } KDAtomicPtr;
#elif defined(KD_ATOMIC_WIN32)
typedef struct KDAtomicInt { KDint value; } KDAtomicInt;
typedef struct KDAtomicPtr { void *value; } KDAtomicPtr;
void _ReadWriteBarrier();
#pragma intrinsic(_ReadWriteBarrier)
#endif

inline KD_API void KD_APIENTRY kdAtomicFenceAcquire(void)
{
#if defined(KD_ATOMIC_C11)
    atomic_thread_fence(memory_order_acquire);
#elif defined(KD_ATOMIC_WIN32)
    _ReadWriteBarrier();
#endif
}

inline KD_API void KD_APIENTRY kdAtomicFenceRelease(void)
{
#if defined(KD_ATOMIC_C11)
    atomic_thread_fence(memory_order_release);
#elif defined(KD_ATOMIC_WIN32)
    _ReadWriteBarrier();
#endif
}

KD_API KDAtomicInt* KD_APIENTRY kdAtomicIntCreate(KDint value)
{
    KDAtomicInt* object = (KDAtomicInt*)kdMalloc(sizeof(KDAtomicInt));
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    object->value = value;
#endif
    return object;
}

KD_API KDAtomicPtr* KD_APIENTRY kdAtomicPtrCreate(void* value)
{
    KDAtomicPtr* object = (KDAtomicPtr*)kdMalloc(sizeof(KDAtomicPtr));
#if defined(KD_ATOMIC_C11)
    atomic_init(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    object->value = value;
#endif
    return object;
}

KD_API void KD_APIENTRY kdAtomicIntFree(KDAtomicInt *object)
{
    kdFree(object);
}

KD_API void kdAtomicPtrFree(KDAtomicPtr *object)
{
    kdFree(object);
}

KD_API KDint KD_APIENTRY kdAtomicIntLoad(KDAtomicInt *object)
{
#if defined(KD_ATOMIC_C11)
    return atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32)
    int value = 0;
    do {
        value = object->value;
    } while (!kdAtomicIntCompareExchange(object, value, value));
    return value;
#endif
}

KD_API void* KD_APIENTRY kdAtomicPtrLoad(KDAtomicPtr *object)
{
#if defined(KD_ATOMIC_C11)
    return atomic_load(&object->value);
#elif defined(KD_ATOMIC_WIN32)
    void* value = 0;
    do {
        value = object->value;
    } while (!kdAtomicPtrCompareExchange(object, value, value));
    return value;
#endif
}

KD_API void KD_APIENTRY kdAtomicIntStore(KDAtomicInt *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    atomic_store(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    _InterlockedExchange((long *)&object->value, (long)value);
#endif
}

KD_API void KD_APIENTRY kdAtomicPtrStore(KDAtomicPtr *object, void* value)
{
#if defined(KD_ATOMIC_C11)
    atomic_store(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
	_InterlockedExchangePointer(&object->value, value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchAdd(KDAtomicInt *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_add(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long*)&object->value, value);
#endif
}

KD_API KDint KD_APIENTRY kdAtomicIntFetchSub(KDAtomicInt *object, KDint value)
{
#if defined(KD_ATOMIC_C11)
    return atomic_fetch_sub(&object->value, value);
#elif defined(KD_ATOMIC_WIN32)
    return _InterlockedExchangeAdd((long*)&object->value, -value);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchange(KDAtomicInt *object, KDint expected, KDint desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, &expected, desired);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchange((long*)&object->value, (long)desired, (long)expected) == (long)expected);
#endif
}

KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchange(KDAtomicPtr *object, void* expected, void* desired)
{
#if defined(KD_ATOMIC_C11)
    return atomic_compare_exchange_weak(&object->value, &expected, desired);
#elif defined(KD_ATOMIC_WIN32)
    return (_InterlockedCompareExchangePointer(&object->value, desired, expected) == expected);
#endif
}

/******************************************************************************
 * Guid
 ******************************************************************************/

typedef struct KDGuid { KDuint8 guid[16]; } KDGuid;

KD_API KDGuid* KDGuidCreate(void)
{
    KDGuid* guid = (KDGuid*)kdMalloc(sizeof(KDGuid));
#if defined(_MSC_VER) || defined(__MINGW32__)
    GUID nativeguid;
    CoCreateGuid(&nativeguid);
    guid->guid[0] = (nativeguid.Data1 >> 24) & 0xFF;
    guid->guid[1] = (nativeguid.Data1 >> 16) & 0xFF;
    guid->guid[2] = (nativeguid.Data1 >> 8) & 0xFF;
    guid->guid[3] = (nativeguid.Data1) & 0xff;
    guid->guid[4] = (nativeguid.Data2 >> 8) & 0xFF;
    guid->guid[5] = (nativeguid.Data2) & 0xff;
    guid->guid[6] = (nativeguid.Data3 >> 8) & 0xFF;
    guid->guid[7] = (nativeguid.Data3) & 0xFF;
    guid->guid[8] = nativeguid.Data4[0];
    guid->guid[9] = nativeguid.Data4[1];
    guid->guid[10] = nativeguid.Data4[2];
    guid->guid[11] = nativeguid.Data4[3];
    guid->guid[12] = nativeguid.Data4[4];
    guid->guid[13] = nativeguid.Data4[5];
    guid->guid[14] = nativeguid.Data4[6];
    guid->guid[15] = nativeguid.Data4[7];
#elif __unix__
    FILE *uuid = fopen("/proc/sys/kernel/random/uuid", "r");
	KDsize result = fread(guid->guid, sizeof(KDuint8), sizeof(guid->guid), uuid);
	fclose(uuid);
	if (result != sizeof(guid->guid))
	{
		kdAssert(0);
	}
#else
    kdAssert(0);
#endif
    return guid;
}

KD_API void KDGuidFree(KDGuid* guid)
{
    kdFree(guid);
}

KD_API KDboolean KDGuidEqual(KDGuid* guid1, KDGuid* guid2)
{
    return (guid1->guid == guid2->guid);
}

/******************************************************************************
 * Thirdparty
 ******************************************************************************/
