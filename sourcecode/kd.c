/******************************************************************************
 * Copyright (c) 2014 Kevin Schmidt
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

#define __STDC_WANT_LIB_EXT1__ 1
#define _POSIX_C_SOURCE 200809L

#ifdef __EMSCRIPTEN__
#define _GNU_SOURCE
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/KHR_float64.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

/* This needs to be included early */
#ifdef KD_GC
#ifndef KD_NDEBUG
#define GC_DEBUG
#endif
#define GC_THREADS
#include <gc.h>
#endif

/******************************************************************************
 * C11 includes
 ******************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/******************************************************************************
 * POSIX includes
 ******************************************************************************/

#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/select.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#ifdef __unix__
#ifdef __linux__
#include <bsd/string.h>
#include <linux/random.h>
#include <linux/version.h>
#endif
#include <sys/param.h>
#ifdef BSD
    /* __DragonFly__ __FreeBSD__ __NetBSD__ __OpenBSD__ */
#endif
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef KD_WINDOW_SUPPORTED
#ifdef KD_WINDOW_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#ifdef KD_WINDOW_WAYLAND
#include <wayland-client.h>
#include <wayland-egl.h>
#endif
#ifdef KD_WINDOW_DISPMANX
#include <bcm_host.h>
#endif
#endif

/******************************************************************************
 * Workarrounds
 ******************************************************************************/

/* POSIX reserved but OpenKODE uses this */
#undef st_mtime

/* C11 Annex K is optional */
#if !defined(__STDC_LIB_EXT1__)
#define strncat_s(buf, buflen, src, srcmaxlen) strlcat(buf, src, buflen)
#define strncpy_s(buf, buflen, src, srcmaxlen) strlcpy(buf, src, buflen)
#define strcpy_s(buf, buflen, src) strlcpy(buf, src, buflen)
#endif

/******************************************************************************
 * Programming environment
 ******************************************************************************/
/* Header only */

/******************************************************************************
 * Errors
 ******************************************************************************/
typedef struct
{
    KDint       errorcode_kd;
    int         errorcode;
    const char *errorcode_text;
} errorcodes;

errorcodes errorcodes_posix[] =
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

KDint errorcode_posix(int errorcode)
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

static thread_local KDint lasterror = 0;
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
        return "libKD open source implementation";
    }
    else if(attribute == KD_ATTRIB_VERSION)
    {
        return "0.1";
    }
    else if(attribute == KD_ATTRIB_PLATFORM)
    {
        return "POSIX";
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
} KDThreadAttr;
KD_API KDThreadAttr *KD_APIENTRY kdThreadAttrCreate(void)
{
    KDThreadAttr *attr = (KDThreadAttr*)kdMalloc(sizeof(KDThreadAttr));
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
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdThreadCreate: Create a new thread. */
typedef struct KDThread
{
    thrd_t thread;
} KDThread;
static KDThreadMutex* __kd_threads_mutex = KD_NULL;
static struct KDThread __kd_threads[999] = {{0}};
static void *(*__kd_start_routine_original)(void *) = KD_NULL;
static void __kd_start_routine_injector(void *arg)
{
#ifdef KD_GC
    struct GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
#endif
    __kd_start_routine_original(arg);
#ifdef KD_GC
    GC_unregister_my_thread();
#endif
}
KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
    kdThreadMutexLock(__kd_threads_mutex);
    __kd_start_routine_original = start_routine;
    thrd_t thrd = 0;
    int error = thrd_create(&thrd, (thrd_start_t)__kd_start_routine_injector, arg);
    if(error == thrd_nomem)
    {
        kdSetError(KD_ENOMEM);
    }
    else if(error == thrd_success)
    {
        for (KDuint thread = 0; thread < sizeof(__kd_threads) / sizeof(__kd_threads[0]); thread++)
        {
            if(!__kd_threads[thread].thread)
            {
                __kd_threads[thread].thread = thrd;
                KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
                KDchar thread_id[KD_ULTOSTR_MAXLEN];
                kdUltostr(thread_id, KD_ULTOSTR_MAXLEN, __kd_threads[thread].thread, 0);
                kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
                mqd_t queue = mq_open(queue_path, O_CREAT, S_IRUSR | S_IWUSR, NULL);
                if(queue == -1)
                {
                    kdAssert(0);
                }
                mq_close(queue);
                if(attr != KD_NULL)
                {
                    if (attr->detachstate == KD_THREAD_CREATE_DETACHED)
                    {
                        kdThreadDetach(&__kd_threads[thread]);
                    }
                }
                kdThreadMutexUnlock(__kd_threads_mutex);
                return &__kd_threads[thread];
            }
        }
    }
    kdThreadMutexUnlock(__kd_threads_mutex);
    return KD_NULL;
}

/* kdThreadExit: Terminate this thread. */
KD_API KD_NORETURN void KD_APIENTRY kdThreadExit(void *retval)
{
    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN, kdThreadSelf()->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    KDint sucess = mq_unlink(queue_path);
    if(sucess == -1)
    {
        kdAssert(0);
    }

    KDint result = 0;
    if(retval != KD_NULL)
    {
        result = *(KDint*)retval;
    }
    thrd_exit(result);
}

/* kdThreadJoin: Wait for termination of another thread. */
KD_API KDint KD_APIENTRY kdThreadJoin(KDThread *thread, void **retval)
{
    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN, thread->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    if(thrd_equal(thread->thread, kdThreadSelf()->thread) != 0)
    {
        kdSetError(KD_EDEADLK);
        return -1;
    }
    void* result = KD_NULL;
    if(retval != KD_NULL)
    {
        result = *retval;
    }
    int error = thrd_join(thread->thread, result);
    if(error == thrd_success)
    {
        KDint sucess = mq_unlink(queue_path);
        if(sucess == -1)
        {
            kdAssert(0);
        }
        return 0;
    }
    else if(error == thrd_error)
    {
        kdSetError(KD_EINVAL);
    }
    return -1;
}

/* kdThreadDetach: Allow resources to be freed as soon as a thread terminates. */
KD_API KDint KD_APIENTRY kdThreadDetach(KDThread *thread)
{
    int error = thrd_detach(thread->thread);
    if(error == thrd_success)
    {
        return 0;
    }
    else if(error == thrd_error)
    {
        kdSetError(KD_EINVAL);
    }
    return -1;
}

/* kdThreadSelf: Return calling thread's ID. */
KD_API KDThread *KD_APIENTRY kdThreadSelf(void)
{
    for (KDuint thread = 0; thread < sizeof(__kd_threads) / sizeof(__kd_threads[0]); thread++)
    {
        if (__kd_threads[thread].thread == thrd_current())
        {
            return &__kd_threads[thread];
        }
    }
    return &__kd_threads[0];
}

/* kdThreadOnce: Wrap initialization code so it is executed only once. */
#ifndef KD_NO_STATIC_DATA
static KDThreadMutex* __kd_once_mutex = KD_NULL;
KD_API KDint KD_APIENTRY kdThreadOnce(KDThreadOnce *once_control, void (*init_routine)(void))
{
    /* Safe double-checked locking */
    KDThreadOnce temp_control = atomic_load_explicit(once_control, memory_order_relaxed);
    atomic_thread_fence(memory_order_acquire);
    if(temp_control.impl == 0)
    {
        kdThreadMutexLock(__kd_once_mutex);
        temp_control = atomic_load_explicit(once_control, memory_order_relaxed);
        if(temp_control.impl == 0)
        {
            temp_control.impl = (void*)1;
            init_routine();
            atomic_thread_fence(memory_order_release);
            atomic_store_explicit(once_control, temp_control, memory_order_relaxed);
        };
        kdThreadMutexUnlock(__kd_once_mutex);
    }
    return 0;
}
#endif /* ndef KD_NO_STATIC_DATA */

/* kdThreadMutexCreate: Create a mutex. */
typedef struct KDThreadMutex
{
    mtx_t mtx;
} KDThreadMutex;
KD_API KDThreadMutex *KD_APIENTRY kdThreadMutexCreate(const void *mutexattr)
{
    KDThreadMutex *mutex = (KDThreadMutex*)kdMalloc(sizeof(KDThreadMutex));
    int error = mtx_init(&mutex->mtx, mtx_plain);
    if(error == thrd_success)
    {
        return mutex;
    }
    else if(error == thrd_error)
    {
        kdSetError(KD_EAGAIN);
    }
    kdFree(mutex);
    return KD_NULL;
}

/* kdThreadMutexFree: Free a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexFree(KDThreadMutex *mutex)
{
    mtx_destroy(&mutex->mtx);
    kdFree(mutex);
    return 0;
}

/* kdThreadMutexLock: Lock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexLock(KDThreadMutex *mutex)
{
    mtx_lock(&mutex->mtx);
    return 0;
}

/* kdThreadMutexUnlock: Unlock a mutex. */
KD_API KDint KD_APIENTRY kdThreadMutexUnlock(KDThreadMutex *mutex)
{
    mtx_unlock(&mutex->mtx);
    return 0;
}

/* kdThreadCondCreate: Create a condition variable. */
typedef struct KDThreadCond
{
    cnd_t cnd;
} KDThreadCond;
KD_API KDThreadCond *KD_APIENTRY kdThreadCondCreate(const void *attr)
{
    KDThreadCond *cond = (KDThreadCond*)kdMalloc(sizeof(KDThreadCond));
    int error =  cnd_init(&cond->cnd);
    if(error == thrd_success)
    {
        return cond;
    }
    else if(error ==  thrd_nomem)
    {
        kdSetError(KD_ENOMEM);
    }
    else if(error == thrd_error)
    {
        kdSetError(KD_EAGAIN);
    }
    kdFree(cond);
    return KD_NULL;
}

/* kdThreadCondFree: Free a condition variable. */
KD_API KDint KD_APIENTRY kdThreadCondFree(KDThreadCond *cond)
{
    cnd_destroy(&cond->cnd);
    kdFree(cond);
    return 0;
}

/* kdThreadCondSignal, kdThreadCondBroadcast: Signal a condition variable. */
KD_API KDint KD_APIENTRY kdThreadCondSignal(KDThreadCond *cond)
{
    cnd_signal(&cond->cnd);
    return 0;
}

KD_API KDint KD_APIENTRY kdThreadCondBroadcast(KDThreadCond *cond)
{
    cnd_broadcast(&cond->cnd);
    return 0;
}

/* kdThreadCondWait: Wait for a condition variable to be signalled. */
KD_API KDint KD_APIENTRY kdThreadCondWait(KDThreadCond *cond, KDThreadMutex *mutex)
{
    cnd_wait(&cond->cnd, &mutex->mtx );
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
    sem->count = 0;
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

/* kdWaitEvent: Get next event from thread's event queue. */
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    kdPumpEvents();
    KDEvent *event = kdCreateEvent();

    struct timespec tm = {0};
    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_nsec += timeout;

    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN,  kdThreadSelf()->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    mqd_t queue = mq_open(queue_path, O_RDONLY);
    KDssize retval = mq_timedreceive( queue, (char*)event, 8192, NULL, &tm );
    mq_close(queue);
    if(retval == -1)
    {
        kdSetError(errorcode_posix(errno));
        kdFreeEvent(event);
        return KD_NULL;
    }
    return event;
}
/* kdSetEventUserptr: Set the userptr for global events. */
KD_API void KD_APIENTRY kdSetEventUserptr(void *userptr)
{
    kdSetError(KD_EOPNOTSUPP);
}

/* kdDefaultEvent: Perform default processing on an unrecognized event. */
KD_API void KD_APIENTRY kdDefaultEvent(const KDEvent *event)
{
    if(event != KD_NULL)
    {
        kdFreeEvent((KDEvent*)event);
    }
}

/* kdPumpEvents: Pump the thread's event queue, performing callbacks. */
struct KDCallback 
{
    KDCallbackFunc *func;
    KDint eventtype;
    void *eventuserptr;
} KDCallback;
typedef struct KDWindowX11
{
    Window window;
    Display *display;
} KDWindowX11;
typedef struct KDWindowWayland
{
} KDWindowWayland;
typedef struct KDWindow
{
    void *nativewindow;
    EGLint format;
    EGLenum platform;
} KDWindow;
static thread_local struct KDCallback callbacks[999] = {{0}};
static thread_local struct KDWindow windows[999]= {{0}};
KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
#ifdef __EMSCRIPTEN__
    /* Give back control to the browser */
    emscripten_sleep(1);
#endif
    for (KDuint window = 0; window < sizeof(windows) / sizeof(windows[0]); window++)
    {
        if(windows[window].nativewindow)
        {
            if(windows[window].platform == EGL_PLATFORM_X11_KHR)
            {
                KDWindowX11 *x11window = windows[window].nativewindow;
                XSelectInput(x11window->display, x11window->window, KeyPressMask|ButtonPressMask|SubstructureRedirectMask);
                while(XPending(x11window->display) > 0)
                {
                    KDEvent *event = kdCreateEvent();
                    XEvent xevent = {0};
                    XNextEvent(x11window->display,&xevent);
                    switch(xevent.type)
                    {
                        case KeyPress:
                        {
                            /* Alt-F4 fires a close window event */
                            if(XLookupKeysym(&xevent.xkey, 0) == XK_F4 && (xevent.xkey.state & Mod1Mask))
                            {
                                event->type      = KD_EVENT_WINDOW_CLOSE;

                                KDboolean has_callback = 0;
                                for (KDuint callback = 0; callback < sizeof(callbacks) / sizeof(callbacks[0]); callback++)
                                {
                                    if(callbacks[callback].eventtype == KD_EVENT_WINDOW_CLOSE)
                                    {
                                        has_callback = 1;
                                        callbacks[callback].func(event);
                                    }
                                }

                                if(!has_callback)
                                {
                                    kdPostEvent(event);
                                }
                                break;
                            }
                        }
                        case ButtonPress:
                        {
                            event->type                     = KD_EVENT_INPUT_POINTER;
                            event->data.inputpointer.index  = KD_INPUT_POINTER_SELECT;
                            event->data.inputpointer.x      = xevent.xbutton.x;
                            event->data.inputpointer.y      = xevent.xbutton.y;
                            event->data.inputpointer.select = 1;

                            KDboolean has_callback = 0;
                            for (KDuint callback = 0; callback < sizeof(callbacks) / sizeof(callbacks[0]); callback++)
                            {
                                if(callbacks[callback].eventtype == KD_EVENT_INPUT_POINTER)
                                {
                                    has_callback = 1;
                                    callbacks[callback].func(event);
                                }
                            }

                            if(!has_callback)
                            {
                                kdPostEvent(event);
                            }
                            break;
                        }
                        case ConfigureNotify:
                        {
                            event->type      = KD_EVENT_WINDOWPROPERTY_CHANGE;

                            kdPostEvent(event);
                            break;
                        }
                        case ClientMessage:
                        {
                            if((Atom)xevent.xclient.data.l[0] == XInternAtom(x11window->display, "WM_DELETE_WINDOW", False))
                            {
                                event->type      = KD_EVENT_WINDOW_CLOSE;
                                KDboolean has_callback = 0;
                                for (KDuint callback = 0; callback < sizeof(callbacks) / sizeof(callbacks[0]); callback++)
                                {
                                    if(callbacks[callback].eventtype == KD_EVENT_WINDOW_CLOSE)
                                    {
                                        has_callback = 1;
                                        callbacks[callback].func(event);
                                    }
                                }

                                if(!has_callback)
                                {
                                    kdPostEvent(event);
                                }
                                break;
                            }
                        }
                        default:
                        {
                            kdFreeEvent(event);
                            break;
                        }
                    }
                }
            }
            else if(windows[window].platform == EGL_PLATFORM_WAYLAND_KHR)
            {
                kdSetError(KD_EOPNOTSUPP);
            }
        }
    }
    return 0;
}

/* kdInstallCallback: Install or remove a callback function for event processing. */
KD_API KDint KD_APIENTRY kdInstallCallback(KDCallbackFunc *func, KDint eventtype, void *eventuserptr)
{
    for (KDuint callback = 0; callback < sizeof(callbacks) / sizeof(callbacks[0]); callback++)
    {
        if(!callbacks[callback].func)
        {
           callbacks[callback].func         = func;
           callbacks[callback].eventtype    = eventtype;
           callbacks[callback].eventuserptr = eventuserptr;
           
           return 0;
        }
    }
    return -1;
}

/* kdCreateEvent: Create an event for posting. */
KD_API KDEvent *KD_APIENTRY kdCreateEvent(void)
{
    KDEvent *event = (KDEvent*)kdMalloc(sizeof(KDEvent));
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

    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN,  thread->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    mqd_t queue = mq_open(queue_path, O_WRONLY);
    if(queue == -1)
    {
        kdAssert(0);
    }

    KDint retval = mq_send(queue, (const char*)event, sizeof(struct KDEvent), 0);
    mq_close(queue);
    kdFreeEvent(event);
    return retval;
}

/* kdFreeEvent: Abandon an event instead of posting it. */
KD_API void KD_APIENTRY kdFreeEvent(KDEvent *event)
{
    kdFree(event);
}

/******************************************************************************
 * System events
 ******************************************************************************/
/* Header only */

/******************************************************************************
 * Application startup and exit.
 ******************************************************************************/
typedef struct KDFile
{
    FILE *file;
} KDFile;

int main(int argc, char **argv)
{
#ifdef KD_GC
    GC_INIT();
    GC_allow_register_threads();
#endif
#ifdef KD_WINDOW_DISPMANX
    bcm_host_init();
#endif
    __kd_threads_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_once_mutex = kdThreadMutexCreate(KD_NULL);

    /* Register mainthread, no need for lock at this stage*/
    __kd_threads[0].thread = thrd_current();

    /* Raise number of mqeues to this users hardlimit*/
    if (!geteuid())
    {
        struct rlimit limit = {RLIM_INFINITY, RLIM_INFINITY};
        setrlimit(RLIMIT_MSGQUEUE, &limit);
    }

    void *app = dlopen(NULL, RTLD_NOW);
    KDint (*kdMain)(KDint argc, const KDchar *const *argv) = KD_NULL;
    /* ISO C forbids assignment between function pointer and ‘void *’ */
    void *rawptr = dlsym(app, "kdMain");
    kdMemcpy(&kdMain, &rawptr, sizeof(rawptr));
    if(dlerror() != NULL)
    {
        kdLogMessage("Cant dlopen self. Dont strip symbols from me.");
        kdAssert(0);
    }

    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN, kdThreadSelf()->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    mqd_t queue = mq_open(queue_path, O_CREAT, S_IRUSR | S_IWUSR, NULL);
    if(queue == -1)
    {
        kdAssert(0);
    }

    int retval = (*kdMain)(argc, (const KDchar *const *)argv);

    mq_close(queue);

#ifdef KD_WINDOW_DISPMANX
    bcm_host_deinit();
#endif
    kdThreadMutexFree(__kd_once_mutex);
    kdThreadMutexFree(__kd_threads_mutex);

    KDint sucess_unlink = mq_unlink(queue_path);
    if(sucess_unlink == -1)
    {
        kdAssert(0);
    }
    return retval;
}

/* kdExit: Exit the application. */
KD_API KD_NORETURN void KD_APIENTRY kdExit(KDint status)
{
    kdThreadMutexFree(__kd_once_mutex);
    kdThreadMutexFree(__kd_threads_mutex);
    
    KDchar queue_path[KD_ULTOSTR_MAXLEN] = "/";
    KDchar thread_id[KD_ULTOSTR_MAXLEN];
    kdUltostr(thread_id, KD_ULTOSTR_MAXLEN, kdThreadSelf()->thread, 0);
    kdStrncat_s(queue_path, sizeof(queue_path), thread_id, sizeof(thread_id));
    KDint sucess_unlink = mq_unlink(queue_path);
    if(sucess_unlink == -1)
    {
                kdAssert(0);
    }
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
#ifdef __linux__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)
    syscall(__NR_getrandom, buf, buflen, 0);
    return 0;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd >= 0)
    {
        kdAssert(ioctl(fd, RNDGETENTCNT, NULL) == 0);
        close(fd);
    }

    KDFile *urandom = kdFopen("/dev/urandom", "r");
    KDsize result = kdFread((void *)buf, sizeof(KDuint8), buflen, urandom);
    kdFclose(urandom);
    if (result != buflen)
    {
        kdSetError(KD_ENOMEM);
        return -1;
    }
    return 0;
#endif
#endif

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
#ifdef KD_GC
    GC_FREE(ptr);
#else
    free(ptr);
#endif
}

/* kdRealloc: Resize memory block. */
KD_API void *KD_APIENTRY kdRealloc(void *ptr, KDsize size)
{
#ifdef KD_GC
    return GC_REALLOC(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

/******************************************************************************
 * Thread-local storage.
 ******************************************************************************/

static thread_local void* tlsptr = KD_NULL;
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
        kdSetError(errorcode_posix(errno));
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
        kdSetError(errorcode_posix(errno));
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

    struct timespec ts = {0};
    /* Determine seconds from the overall nanoseconds */
    if ((payload->interval % 1000000000) == 0)
    {
        ts.tv_sec = payload->interval / 1000000000;
    }
    else
    {
        ts.tv_sec = (payload->interval - (payload->interval % 1000000000)) / 1000000000;
    }

    /* Remaining nanoseconds */
    ts.tv_nsec = payload->interval - (ts.tv_sec*1000000000);

    for(;;)
    {
        thrd_sleep(&ts, NULL);

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
        const KDEvent *event = kdWaitEvent(0);
        if(event)
        {
            KDboolean quit = 0;
            if(event->type == KD_EVENT_QUIT)
            {
                quit = 1;
            }

            kdDefaultEvent(event);
            if(quit)
            {
                break;
            }
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
KD_API KDFile *KD_APIENTRY kdFopen(const KDchar *pathname, const KDchar *mode)
{
    KDFile *file = (KDFile*)kdMalloc(sizeof(KDFile));
    file->file = fopen(pathname, mode);
    return file;
}

/* kdFclose: Close an open file. */
KD_API KDint KD_APIENTRY kdFclose(KDFile *file)
{
    KDint retval = fclose(file->file);
    kdFree(file);
    return retval;
}

/* kdFflush: Flush an open file. */
KD_API KDint KD_APIENTRY kdFflush(KDFile *file)
{
    return fflush(file->file);
}

/* kdFread: Read from a file. */
KD_API KDsize KD_APIENTRY kdFread(void *buffer, KDsize size, KDsize count, KDFile *file)
{
    return fread(buffer, size, count, file->file);
}

/* kdFwrite: Write to a file. */
KD_API KDsize KD_APIENTRY kdFwrite(const void *buffer, KDsize size, KDsize count, KDFile *file)
{
    return fwrite(buffer, size, count, file->file);
}

/* kdGetc: Read next byte from an open file. */
KD_API KDint KD_APIENTRY kdGetc(KDFile *file)
{
    return fgetc(file->file);
}

/* kdPutc: Write a byte to an open file. */
KD_API KDint KD_APIENTRY kdPutc(KDint c, KDFile *file)
{
    return fputc(c, file->file);
}

/* kdFgets: Read a line of text from an open file. */
KD_API KDchar *KD_APIENTRY kdFgets(KDchar *buffer, KDsize buflen, KDFile *file)
{
    return fgets(buffer, (KDint)buflen, file->file);
}

/* kdFEOF: Check for end of file. */
KD_API KDint KD_APIENTRY kdFEOF(KDFile *file)
{
    return feof(file->file);
}

/* kdFerror: Check for an error condition on an open file. */
KD_API KDint KD_APIENTRY kdFerror(KDFile *file)
{
    return ferror(file->file);
}

/* kdClearerr: Clear a file's error and end-of-file indicators. */
KD_API void KD_APIENTRY kdClearerr(KDFile *file)
{
    clearerr(file->file);
}

/* kdFseek: Reposition the file position indicator in a file. */
KD_API KDint KD_APIENTRY kdFseek(KDFile *file, KDoff offset, KDfileSeekOrigin origin)
{
    return fseek( file->file, offset, origin);
}

/* kdFtell: Get the file position of an open file. */
KD_API KDoff KD_APIENTRY kdFtell(KDFile *file)
{
    return ftell( file->file);
}

/* kdMkdir: Create new directory. */
KD_API KDint KD_APIENTRY kdMkdir(const KDchar *pathname)
{
    return mkdir (pathname, S_IRWXU);
}

/* kdRmdir: Delete a directory. */
KD_API KDint KD_APIENTRY kdRmdir(const KDchar *pathname)
{
    return rmdir(pathname);
}

/* kdRename: Rename a file. */
KD_API KDint KD_APIENTRY kdRename(const KDchar *src, const KDchar *dest)
{
    return rename(src, dest);
}

/* kdRemove: Delete a file. */
KD_API KDint KD_APIENTRY kdRemove(const KDchar *pathname)
{
    return remove(pathname);
}

/* kdTruncate: Truncate or extend a file. */
KD_API KDint KD_APIENTRY kdTruncate(const KDchar *pathname, KDoff length)
{
    return truncate(pathname, length); 
}

/* kdStat, kdFstat: Return information about a file. */
KD_API KDint KD_APIENTRY kdStat(const KDchar *pathname, struct KDStat *buf)
{
    struct stat posixstat = {0};
    int retval = stat(pathname, &posixstat);

    buf->st_mode  = posixstat.st_mode;
    buf->st_size  = posixstat.st_size;
    buf->st_mtime = posixstat.st_mtim.tv_sec;

    return retval;
}

KD_API KDint KD_APIENTRY kdFstat(KDFile *file, struct KDStat *buf)
{
    struct stat posixstat = {0};
    int retval = fstat(fileno(file->file), &posixstat);

    buf->st_mode  = posixstat.st_mode;
    buf->st_size  = posixstat.st_size;
    buf->st_mtime = posixstat.st_mtim.tv_sec;

    return retval;
}

/* kdAccess: Determine whether the application can access a file or directory. */
KD_API KDint KD_APIENTRY kdAccess(const KDchar *pathname, KDint amode)
{
    return access(pathname, amode);
}

/* kdOpenDir: Open a directory ready for listing. */
typedef struct KDDir
{
    DIR *dir;
} KDDir;
KD_API KDDir *KD_APIENTRY kdOpenDir(const KDchar *pathname)
{
    KDDir *dir = (KDDir*)kdMalloc(sizeof(KDDir));
    dir->dir = opendir(pathname);
    return dir;
}

/* kdReadDir: Return the next file in a directory. */
KD_API KDDirent *KD_APIENTRY kdReadDir(KDDir *dir)
{
    KDDirent *kddirent = (KDDirent*)kdMalloc(sizeof(KDDirent));
    struct dirent *posixdirent = readdir(dir->dir);
    kddirent->d_name = posixdirent->d_name;
    return kddirent;
}

/* kdCloseDir: Close a directory. */
KD_API KDint KD_APIENTRY kdCloseDir(KDDir *dir)
{
    KDint retval = closedir(dir->dir);
    kdFree(dir);
    return retval;
}

/* kdGetFree: Get free space on a drive. */
KD_API KDoff KD_APIENTRY kdGetFree(const KDchar *pathname)
{
     struct statfs buf = {0};
     statfs(pathname, &buf);
     return (buf.f_bsize/1024L) * buf.f_bavail;
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
    window->platform = EGL_PLATFORM_X11_KHR;
    if (window->platform == EGL_PLATFORM_X11_KHR)
    {
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
        XChangeProperty(x11window->display, x11window->window, netwm_prop_hints, 4, 32, 0, (const unsigned char *) &netwm_hints, 3);
        XMapWindow(x11window->display, x11window->window);
        XFlush(x11window->display);
    }
    else if (window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        window->nativewindow = kdMalloc(sizeof(KDWindowWayland));
        KDWindowWayland *waylandwindow = window->nativewindow;
    }
    else
    {
        kdSetError(KD_EOPNOTSUPP);
        kdFree(window);
        window = KD_NULL;
    }
    if (window != KD_NULL)
    {
        for (KDuint i = 0; i < sizeof(windows) / sizeof(windows[0]); i++)
        {
            if (!windows[i].nativewindow)
            {
                windows[i].nativewindow = window->nativewindow;
                windows[i].format = window->format;
                windows[i].platform = window->platform;

                break;
            }
        }
    }
    return window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
    KDint retval = -1;
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        KDWindowX11 *x11window = window->nativewindow;
        XCloseDisplay(x11window->display);
        kdFree(x11window);
        retval = 0;
    }
    else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        kdSetError(KD_EOPNOTSUPP);
    }
    kdFree(window);
    return retval;
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
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            KDWindowX11 *x11window = window->nativewindow;
            XMoveResizeWindow(x11window->display, x11window->window, 0, 0, (KDuint)param[0], (KDuint)param[1]);
            return 0;
        }
        else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            kdSetError(KD_EOPNOTSUPP);
            return -1;
        }
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdSetWindowPropertycv(KDWindow *window, KDint pname, const KDchar *param)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            KDWindowX11 *x11window = window->nativewindow;
            XStoreName(x11window->display, x11window->window, param);
            return 0;
        }
        else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            kdSetError(KD_EOPNOTSUPP);
            return -1;
        }
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
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            KDWindowX11 *x11window = window->nativewindow;
            param[0] = XWidthOfScreen(XDefaultScreenOfDisplay(x11window->display));
            param[1] = XHeightOfScreen(XDefaultScreenOfDisplay(x11window->display));
            return 0;
        }
        else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            kdSetError(KD_EOPNOTSUPP);
            return -1;
        }
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertycv(KDWindow *window, KDint pname, KDchar *param, KDsize *size)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            KDWindowX11 *x11window = window->nativewindow;
            XFetchName(x11window->display, x11window->window, &param);
            return 0;
        } 
        else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            kdSetError(KD_EOPNOTSUPP);
            return -1;
        }
    }
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdRealizeWindow: Realize the window as a displayable entity and get the native window handle for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizeWindow(KDWindow *window, EGLNativeWindowType *nativewindow)
{
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        KDWindowX11 *x11window = window->nativewindow;
        *nativewindow = x11window->window;
        return 0;
    } 
    return -1;
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
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
    printf(string);
    fflush(stdout);
}

/******************************************************************************
 * Extensions
 ******************************************************************************/

/******************************************************************************
 * Thirdparty
 ******************************************************************************/
