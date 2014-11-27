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

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/KHR_float64.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

/******************************************************************************
 * C11 includes
 ******************************************************************************/

#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/******************************************************************************
 * POSIX includes
 ******************************************************************************/

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/select.h> 
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#ifdef KD_WINDOW_SUPPORTED
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

/******************************************************************************
 * Workarrounds
 ******************************************************************************/

/* POSIX reserved but OpenKODE uses this */
#undef st_mtime

/* C11 Annex K is optional */
#ifndef __STDC_LIB_EXT1__
#if __BSD_VISIBLE != 1
size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif
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
    KDThreadAttr* attr = (KDThreadAttr*)kdMalloc(sizeof(KDThreadAttr));
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
    thrd_t thrd;
} KDThread;
static thread_local KDThread threadself = {0};
KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
    KDThread* thread = &threadself;
    int error = thrd_create(&thread->thrd, (thrd_start_t)start_routine, arg);
    if(error == thrd_success)
    {
        KDchar queueid[KD_ULTOSTR_MAXLEN];
        kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)thread->thrd, 0);
        mq_open(queueid,O_CREAT);
    }
    else if(error == thrd_nomem)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    if(attr->detachstate == KD_THREAD_CREATE_DETACHED)
    {
        kdThreadDetach(thread);
    }
    return thread;
}

/* kdThreadExit: Terminate this thread. */
KD_API KD_NORETURN void KD_APIENTRY kdThreadExit(void *retval)
{
    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)kdThreadSelf()->thrd, 0);
    mq_unlink(queueid);
    thrd_exit(*(int*)retval);
}

/* kdThreadJoin: Wait for termination of another thread. */
KD_API KDint KD_APIENTRY kdThreadJoin(KDThread *thread, void **retval)
{
    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)thread->thrd, 0);
    int error = thrd_join(thread->thrd, *retval);
    if(error == thrd_success)
    {
        mq_unlink(queueid);
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
    int error = thrd_detach(thread->thrd);
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
    KDThread* thread = &threadself;
    thread->thrd = thrd_current();
    return thread;
}

/* kdThreadOnce: Wrap initialization code so it is executed only once. */
#ifndef KD_NO_STATIC_DATA
KD_API KDint KD_APIENTRY kdThreadOnce(KDThreadOnce *once_control, void (*init_routine)(void))
{
    once_control->impl = ONCE_FLAG_INIT;
    call_once( once_control->impl, init_routine);
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
    KDThreadMutex* mutex = (KDThreadMutex*)kdMalloc(sizeof(KDThreadMutex));
    int error = mtx_init(&mutex->mtx, mtx_plain);
    if(error == thrd_success)
    {
        return mutex;
    }
    else if(error == thrd_error)
    {
        kdSetError(KD_EAGAIN);
    }
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
    KDThreadCond* cond = (KDThreadCond*)kdMalloc(sizeof(KDThreadCond));
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
    sem_t sem;
} KDThreadSem;
KD_API KDThreadSem *KD_APIENTRY kdThreadSemCreate(KDuint value)
{
    KDThreadSem* sem = (KDThreadSem*)kdMalloc(sizeof(KDThreadSem));
    sem_init(&sem->sem, 0, value);
    return sem;
}

/* kdThreadSemFree: Free a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemFree(KDThreadSem *sem)
{
    sem_destroy(&sem->sem);
    kdFree(sem);
    return 0;
}

/* kdThreadSemWait: Lock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemWait(KDThreadSem *sem)
{
    return sem_wait(&sem->sem);
}

/* kdThreadSemPost: Unlock a semaphore. */
KD_API KDint KD_APIENTRY kdThreadSemPost(KDThreadSem *sem)
{
    return sem_post(&sem->sem);
}

/******************************************************************************
 * Events
 ******************************************************************************/

/* kdWaitEvent: Get next event from thread's event queue. */
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    kdPumpEvents();
    KDEvent* event = kdCreateEvent();

    struct timespec tm = {0};
    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_nsec += timeout;

    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)kdThreadSelf()->thrd, 0);
    mqd_t queue = mq_open(queueid, O_RDONLY);
    if(mq_timedreceive( queue, (char*)event, sizeof(struct KDEvent), NULL, &tm ) == -1) 
    {
        kdFreeEvent(event);
        event = KD_NULL;
    }
    mq_close(queue);
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
    KDCallbackFunc* func;
    KDint eventtype;
    void* eventuserptr;
} KDCallback;
static thread_local struct KDCallback callbacks[999] = {{0}};
KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
    EGLenum platform = EGL_PLATFORM_X11_KHR;
    if(platform == EGL_PLATFORM_X11_KHR)
    {
        __block Display* display = XOpenDisplay(NULL);
        while(^{
            /*
            Simple DirectMedia Layer
            Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>
            zlib license
            */
            XFlush(display);
            if (XEventsQueued(display, QueuedAlready)) 
            {
                return (1);
            }

            static struct timeval zero_time;
            int x11_fd;
            fd_set fdset;

            x11_fd = ConnectionNumber(display);
            FD_ZERO(&fdset);
            FD_SET(x11_fd, &fdset);
            if (select(x11_fd + 1, &fdset, NULL, NULL, &zero_time) == 1) 
            {
                return (XPending(display));
            }
            return (0);
        })
        {
            KDEvent* event = kdCreateEvent();
            XEvent xevent = {0};
            XNextEvent(display,&xevent);
            switch(xevent.type)
            {
                case KeyPress:
                {
                    if(XLookupKeysym(&xevent.xkey, 0) == XK_Escape)
                    {
                        event->type = KD_EVENT_WINDOW_CLOSE;
                        event->timestamp = kdGetTimeUST();
                        kdPostEvent(event);
                        break;
                    }
                }
                case ButtonPress:
                {
                    event->type = KD_EVENT_INPUT_POINTER;
                    event->timestamp = kdGetTimeUST();
                    event->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                    event->data.inputpointer.x = xevent.xbutton.x;
                    event->data.inputpointer.y = xevent.xbutton.y;
                    event->data.inputpointer.select = 1;

                    KDboolean has_callback = 0;
                    for (KDuint i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); i++)
                    {
                        if(callbacks[i].eventtype == KD_EVENT_INPUT)
                        {
                            has_callback = 1;
                            callbacks[i].func(event);
                        }
                    }

                    if(has_callback)
                    {
                        kdFreeEvent(event);
                    }
                    else
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                case ConfigureNotify:
                {
                    event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
                    event->timestamp = kdGetTimeUST();
                    kdPostEvent(event);
                    break;
                }
                case ClientMessage:
                {
                    if((Atom)xevent.xclient.data.l[0] == XInternAtom(display, "WM_DELETE_WINDOW", False))
                    {
                        event->type = KD_EVENT_WINDOW_CLOSE;
                        event->timestamp = kdGetTimeUST();
                        kdPostEvent(event);
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
        XCloseDisplay(display);
    }
    else if(platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        kdSetError(KD_EOPNOTSUPP);
        return -1;
    }
    else
    {
        kdSetError(KD_EOPNOTSUPP);
        return -1;
    }
    return 0;
}

/* kdInstallCallback: Install or remove a callback function for event processing. */
KD_API KDint KD_APIENTRY kdInstallCallback(KDCallbackFunc *func, KDint eventtype, void *eventuserptr)
{
    for (KDuint i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); i++)
    {
        if(!callbacks[i].func)
        {
           callbacks[i].func = func;
           callbacks[i].eventtype = eventtype;
           callbacks[i].eventuserptr = eventuserptr;
           return 0;
        }
    }
    return -1;
}

/* kdCreateEvent: Create an event for posting. */
KD_API KDEvent *KD_APIENTRY kdCreateEvent(void)
{
    KDEvent* event = (KDEvent*)kdMalloc(sizeof(KDEvent));
    return event;
}

/* kdPostEvent, kdPostThreadEvent: Post an event into a queue. */
KD_API KDint KD_APIENTRY kdPostEvent(KDEvent *event)
{
    return kdPostThreadEvent(event, kdThreadSelf());
}
KD_API KDint KD_APIENTRY kdPostThreadEvent(KDEvent *event, KDThread *thread)
{
    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)thread->thrd, 0);
    mqd_t queue = mq_open(queueid, O_WRONLY);
    mq_send(queue, (const char*)event, sizeof(struct KDEvent), 0);
    return 0;
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
 int main(int argc, char **argv)
{
    void* app = dlopen(NULL, RTLD_NOW);
    KDint (*kdMain)(KDint argc, const KDchar *const *argv) = KD_NULL;
    /* ISO C forbids assignment between function pointer and ‘void *’ */
    void* rawptr = dlsym(app, "kdMain");
    memcpy(&kdMain, &rawptr, sizeof(rawptr));
    if(dlerror() != NULL)
    {
        kdLogMessage("Cant dlopen self. Dont strip symbols from me.");
        kdAssert(0);
    }

    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)kdThreadSelf()->thrd, 0);
    mq_open(queueid,O_CREAT, 0666, NULL);
    int retval = (*kdMain)(argc, (const KDchar *const *)argv);
    mq_unlink(queueid);
    return retval;
}

/* kdExit: Exit the application. */
KD_API KD_NORETURN void KD_APIENTRY kdExit(KDint status)
{
    KDchar queueid[KD_ULTOSTR_MAXLEN];
    kdUltostr(queueid, KD_ULTOSTR_MAXLEN,  (KDuintptr)kdThreadSelf()->thrd, 0);
    mq_unlink(queueid);
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
    for (KDuint i = 0; i < buflen; i++)
    {
        srand(time(0));
        buf[i] = rand() ;
    }
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
    void* retval = memchr(src , byte, len);
    if(retval == NULL)
    {
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
    void* retval = strchr(str, ch);
    if(retval == NULL)
    {
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
    return strncat_s(buf, buflen, src, srcmaxlen);
}

/* kdStrncmp: Compares two strings with length limit. */
KD_API KDint KD_APIENTRY kdStrncmp(const KDchar *str1, const KDchar *str2, KDsize maxlen)
{
    return strncmp(str1, str2, maxlen);
}

/* kdStrcpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrcpy_s(KDchar *buf, KDsize buflen, const KDchar *src)
{
    return strcpy_s(buf, buflen, src);
}

/* kdStrncpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrncpy_s(KDchar *buf, KDsize buflen, const KDchar *src, KDsize srclen)
{
    return strncpy_s(buf, buflen, src, srclen);
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
    return time(timep);
}

/* kdGmtime_r, kdLocaltime_r: Convert a seconds-since-epoch time into broken-down time. */
KD_API KDTm *KD_APIENTRY kdGmtime_r(const KDtime *timep, KDTm *result)
{
    KDTm * retval = (KDTm*)gmtime(timep);
    *result = *retval;
    return retval;
}
KD_API KDTm *KD_APIENTRY kdLocaltime_r(const KDtime *timep, KDTm *result)
{
    KDTm * retval = (KDTm*)localtime(timep);
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
typedef struct KDTimer
{
    timer_t timer;
    KDThread* thread;
    void *userptr;
} KDTimer;
static void timerhandler(int sig, siginfo_t *si, void *uc)
{
    KDTimer* timer = (KDTimer*)si->si_value.sival_ptr;
    KDEvent* event = kdCreateEvent();
    event->type = KD_EVENT_TIMER;
    event->timestamp = kdGetTimeUST();
    event->userptr = timer->userptr;
    kdPostThreadEvent(event, timer->thread);
}
KD_API KDTimer *KD_APIENTRY kdSetTimer(KDint64 interval, KDint periodic, void *eventuserptr)
{
    KDTimer* timer = (KDTimer*)kdMalloc(sizeof(KDTimer));
    timer->thread = kdThreadSelf();
    timer->userptr = eventuserptr;

    struct sigaction sa = {{0}};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerhandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    struct sigevent se = {{0}};
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGRTMIN;
    se.sigev_value.sival_ptr = &timer;

    timer_create(CLOCK_REALTIME, &se, &timer->timer);

    struct itimerspec ts = {{0}};
    ts.it_value.tv_nsec = interval;
    if(periodic != KD_TIMER_ONESHOT)
    {
        ts.it_interval.tv_nsec = interval;
    }
    timer_settime(timer->timer, 0, &ts, NULL);

    return timer;
}

/* kdCancelTimer: Cancel and free a timer. */
KD_API KDint KD_APIENTRY kdCancelTimer(KDTimer *timer)
{
    timer_delete(timer->timer);
    kdFree(timer);
    return 0;
}

/******************************************************************************
 * File system
 ******************************************************************************/

/* kdFopen: Open a file from the file system. */
typedef struct KDFile
{
    FILE* file;
} KDFile;
KD_API KDFile *KD_APIENTRY kdFopen(const KDchar *pathname, const KDchar *mode)
{
    KDFile* file = (KDFile*)kdMalloc(sizeof(KDFile));
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
    return getc(file->file);
}

/* kdPutc: Write a byte to an open file. */
KD_API KDint KD_APIENTRY kdPutc(KDint c, KDFile *file)
{
    return putc(c, file->file);
}

/* kdFgets: Read a line of text from an open file. */
KD_API KDchar *KD_APIENTRY kdFgets(KDchar *buffer, KDsize buflen, KDFile *file)
{
    return fgets(buffer, buflen, file->file);
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
    struct stat* posixstat = {0};
    int retval = stat(pathname, posixstat);
    buf->st_mode = posixstat->st_mode;
    buf->st_size = posixstat->st_size;
    buf->st_mtime = posixstat->st_mtim.tv_sec;
    return retval;
}

KD_API KDint KD_APIENTRY kdFstat(KDFile *file, struct KDStat *buf)
{
    struct stat* posixstat = {0};
    int retval = fstat(fileno(file->file), posixstat);
    buf->st_mode = posixstat->st_mode;
    buf->st_size = posixstat->st_size;
    buf->st_mtime = posixstat->st_mtim.tv_sec;
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
    DIR* dir;
} KDDir;
KD_API KDDir *KD_APIENTRY kdOpenDir(const KDchar *pathname)
{
    KDDir* dir = (KDDir*)kdMalloc(sizeof(KDDir));
    dir->dir = opendir(pathname);
    return dir;
}

/* kdReadDir: Return the next file in a directory. */
KD_API KDDirent *KD_APIENTRY kdReadDir(KDDir *dir)
{
    KDDirent*  kddirent = {0};
    struct dirent* posixdirent = readdir(dir->dir);
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
    return -1; 
}

/* kdHtons: Convert a 16-bit integer from host to network byte order. */
KD_API KDuint16 KD_APIENTRY kdHtons(KDuint16 hostshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdNtohl: Convert a 32-bit integer from network to host byte order. */
KD_API KDuint32 KD_APIENTRY kdNtohl(KDuint32 netlong)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
}

/* kdNtohs: Convert a 16-bit integer from network to host byte order. */
KD_API KDuint16 KD_APIENTRY kdNtohs(KDuint16 netshort)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1; 
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
 typedef struct KDWindow
{
    EGLNativeWindowType window;
    EGLNativeDisplayType display;
    EGLint format;
    EGLenum platform;
} KDWindow;
KD_API KDWindow *KD_APIENTRY kdCreateWindow(EGLDisplay display, EGLConfig config, void *eventuserptr)
{
    KDWindow* window = (KDWindow*)kdMalloc(sizeof(KDWindow));
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->format);
    window->platform = EGL_PLATFORM_X11_KHR;
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        window->display = XOpenDisplay(NULL);
        window->window = XCreateSimpleWindow(window->display, 
            XRootWindow(window->display, XDefaultScreen(window->display)), 0, 0, 
            XWidthOfScreen(XDefaultScreenOfDisplay(window->display)), 
            XHeightOfScreen(XDefaultScreenOfDisplay(window->display)), 0, 
            XBlackPixel(window->display, XDefaultScreen(window->display)), 
            XWhitePixel(window->display, XDefaultScreen(window->display)));
        XStoreName(window->display, window->window, "OpenKODE");
        Atom wm_del_win_msg = XInternAtom(window->display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(window->display, window->window, &wm_del_win_msg, 1);
        Atom mwm_prop_hints = XInternAtom(window->display, "_MOTIF_WM_HINTS", True);
        long mwm_hints[5] = { 2, 0, 0, 0, 0 };
        XChangeProperty(window->display, window->window, mwm_prop_hints, mwm_prop_hints, 32, 0, (const unsigned char *)&mwm_hints, 5);
        Atom netwm_prop_hints = XInternAtom(window->display, "_NET_WM_STATE", False);
        Atom netwm_hints[3];
        netwm_hints[0] = XInternAtom(window->display, "_NET_WM_STATE_FULLSCREEN", False);
        netwm_hints[1] = XInternAtom(window->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        netwm_hints[2] = XInternAtom(window->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        XChangeProperty(window->display, window->window, netwm_prop_hints, 4, 32, 0, (const unsigned char *)&netwm_hints, 3); 
        XMapWindow(window->display, window->window);
        XFlush(window->display);
        XSelectInput(window->display, window->window, KeyPressMask|ButtonPressMask|SubstructureRedirectMask);
    }
    else if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        kdSetError(KD_EOPNOTSUPP);
        window = KD_NULL;
    }
    else
    {
        kdSetError(KD_EOPNOTSUPP);
        window = KD_NULL;
    }
    return window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
    KDint retval = -1;
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        XCloseDisplay(window->display);
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
            XMoveResizeWindow(window->display, (Window)window->window, 0, 0, param[0], param[1]);
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
            XStoreName(window->display, (Window)window->window, param);
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
            param[0] = XWidthOfScreen(XDefaultScreenOfDisplay(window->display));
            param[1] = XHeightOfScreen(XDefaultScreenOfDisplay(window->display));
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
            XFetchName(window->display, (Window)window->window, &param);
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
    *nativewindow = window->window;
    return 0;
}
#endif

/******************************************************************************
 * Assertions and logging
 ******************************************************************************/

/* kdHandleAssertion: Handle assertion failure. */
KD_API void KD_APIENTRY kdHandleAssertion(const KDchar *condition, const KDchar *filename, KDint linenumber)
{
    char message[KDINT_MAX-1];
    char format[] = "Assert \"%s\" in file \"%s\" on line %d";
    snprintf(message, KDINT_MAX-1, format, condition, filename, linenumber);
    kdLogMessage(message);
    kdExit(-1);
}

/* kdLogMessage: Output a log message. */
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
    printf(string);
}

/******************************************************************************
 * Extensions
 ******************************************************************************/

/******************************************************************************
 * Thirdparty
 ******************************************************************************/

 #if __BSD_VISIBLE != 1
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return(dlen + (s - src));   /* count does not include NUL */
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';      /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
}
#endif
