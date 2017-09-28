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
 * Implementation notes
 *
 * - Only one window is supported
 * - Networking is not supported
 * - To receive orientation changes AndroidManifest.xml should include
 *   android:configChanges="orientation|keyboardHidden|screenSize"
 *
 ******************************************************************************/

/******************************************************************************
 * Header workarounds
 ******************************************************************************/

/* clang-format off */
#if defined(__unix__) || defined(__EMSCRIPTEN__)
#   if defined(__linux__) || defined(__EMSCRIPTEN__)
#       define _GNU_SOURCE
#   endif
#   include <sys/param.h>
#   if defined(BSD)
#       define _BSD_SOURCE
#   endif
#endif

#if defined(_MSC_VER)
#   define _CRT_SECURE_NO_WARNINGS 1
#   define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/kdext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "kd_internal.h"

#ifndef EGL_PLATFORM_X11_KHR
#define EGL_PLATFORM_X11_KHR 0x31D5
#endif 

#ifndef EGL_PLATFORM_WAYLAND_KHR
#define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

/* Freestanding safe */
#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(KD_FREESTANDING)
#   include <errno.h>
#   include <locale.h>
#   include <stdlib.h>
#   include <stdio.h>
#   include <time.h>
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <direct.h> /* R_OK/W_OK/X_OK */
#   include <intrin.h> /* _mm_* */
#   include <winsock2.h> /* WSA */
#   include <ws2tcpip.h> 
#   ifndef inline
#       define inline __inline /* MSVC redefinition fix */
#   endif
#   undef s_addr /* OpenKODE uses this */
#endif

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#   include <unistd.h>
#   include <fcntl.h>
#   include <dirent.h>
#   include <dlfcn.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <sys/mman.h> /* mmap */
#   include <sys/socket.h>
#   include <sys/stat.h>
#   include <sys/syscall.h>
#   if defined(__APPLE__) || defined(BSD)
#       include <sys/mount.h>
#   else
#       include <sys/vfs.h>
#   endif
#   if defined(__linux__)
#       include <sys/prctl.h>
#   endif
#   if !defined(__TINYC__)  && !defined(__PGIC__)
#       if defined(__x86_64__) || defined(__i386__)
#           include <x86intrin.h>        
#       elif defined(__ARM_NEON__)       
#           include <arm_neon.h>     
#       endif
#   endif
#   if defined(__ANDROID__)
#       include <android/api-level.h>
#       include <android/keycodes.h>
#       include <android/native_activity.h>
#       include <android/native_window.h>
#       include <android/window.h>
#   endif
#   if defined(__APPLE__)
#       include <TargetConditionals.h>
#   endif
#   if defined(__EMSCRIPTEN__)
#       include <emscripten/emscripten.h>
#       include <emscripten/html5.h>
#   endif
#   if defined(KD_WINDOW_X11) || defined(KD_WINDOW_WAYLAND)
#       include <xcb/xcb.h>
#       include <xcb/randr.h>
#       include <xkbcommon/xkbcommon.h>
#   endif
#   if defined(KD_WINDOW_WAYLAND)
#       include <wayland-client.h>
#       include <wayland-egl.h>
#   endif
#   if defined(KD_WINDOW_X11) 
#       include <xcb/xcb_ewmh.h>
#       include <xcb/xcb_icccm.h>
#       include <xcb/xcb_util.h>
#       include <xcb/xkb.h>
#       include <xkbcommon/xkbcommon-x11.h>
#   endif
#   undef st_mtime /* OpenKODE uses this */
#endif

#if defined(KD_THREAD_POSIX) || defined(KD_WINDOW_X11) || defined(KD_WINDOW_WAYLAND)
#   include <pthread.h>
#endif
#if defined(KD_THREAD_C11)
#   include <threads.h>
#endif

/******************************************************************************
 * Thirdparty includes
 ******************************************************************************/

#if defined(__INTEL_COMPILER)
#    pragma warning(push)
#    pragma warning(disable: 3656)
#elif defined(__GNUC__) || (__clang__)
#   pragma GCC diagnostic push
#   if __GNUC__ >= 6
#       pragma GCC diagnostic ignored "-Wmisleading-indentation"
#       pragma GCC diagnostic ignored "-Wshift-negative-value"
#   endif
#   if !defined(__clang__)
#       pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#   endif
#   pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4244)
#   pragma warning(disable : 4701)
#   pragma warning(disable : 4703)
#   pragma warning(disable : 6001)
#   pragma warning(disable : 6011)
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
/* clang-format on */

/******************************************************************************
 * Errors
 ******************************************************************************/

typedef struct __KDCallback __KDCallback;
struct KDThread {
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
    _KDQueue *eventqueue;
    KDEvent *lastevent;
    KDint lasterror;
    KDint callbackindex;
    __KDCallback **callbacks;
    void *tlsptr;
};

/* kdGetError: Get last error indication. */
KD_API KDint KD_APIENTRY kdGetError(void)
{
    return kdThreadSelf()->lasterror;
}

/* kdSetError: Set last error indication. */
KD_API void KD_APIENTRY kdSetError(KDint error)
{
    kdThreadSelf()->lasterror = error;
}

KD_API void KD_APIENTRY kdSetErrorPlatformVEN(KDint error, KDint allowed)
{
    KDint kderror = 0;
#if defined(_WIN32)
    switch(error)
    {
        case(ERROR_ACCESS_DENIED):
        case(ERROR_LOCK_VIOLATION):
        case(ERROR_SHARING_VIOLATION):
        case(ERROR_WRITE_PROTECT):
        {
            kderror = KD_EACCES;
            break;
        }
        case(ERROR_INVALID_HANDLE):
        {
            kderror = KD_EBADF;
            break;
        }
        case(ERROR_BUSY):
        case(ERROR_CHILD_NOT_COMPLETE):
        case(ERROR_PIPE_BUSY):
        case(ERROR_PIPE_CONNECTED):
        case(ERROR_SIGNAL_PENDING):
        {
            kderror = KD_EBUSY;
            break;
        }
        case(ERROR_ALREADY_EXISTS):
        case(ERROR_DIR_NOT_EMPTY):
        case(ERROR_FILE_EXISTS):
        {
            kderror = KD_EEXIST;
            break;
        }
        case(ERROR_BAD_USERNAME):
        case(ERROR_BAD_PIPE):
        case(ERROR_INVALID_DATA):
        case(ERROR_INVALID_PARAMETER):
        case(ERROR_INVALID_SIGNAL_NUMBER):
        case(ERROR_META_EXPANSION_TOO_LONG):
        case(ERROR_NEGATIVE_SEEK):
        case(ERROR_NO_TOKEN):
        case(ERROR_THREAD_1_INACTIVE):
        {
            kderror = KD_EINVAL;
            break;
        }
        case(ERROR_CRC):
        case(ERROR_IO_DEVICE):
        case(ERROR_NO_SIGNAL_SENT):
        case(ERROR_SIGNAL_REFUSED):
        case(ERROR_OPEN_FAILED):
        {
            kderror = KD_EIO;
            break;
        }
        case(ERROR_NO_MORE_SEARCH_HANDLES):
        case(ERROR_TOO_MANY_OPEN_FILES):
        {
            kderror = KD_EMFILE;
            break;
        }
        case(ERROR_FILENAME_EXCED_RANGE):
        {
            kderror = KD_ENAMETOOLONG;
            break;
        }
        case(ERROR_BAD_PATHNAME):
        case(ERROR_FILE_NOT_FOUND):
        case(ERROR_INVALID_NAME):
        case(ERROR_PATH_NOT_FOUND):
        {
            kderror = KD_ENOENT;
            break;
        }
        case(ERROR_NOT_ENOUGH_MEMORY):
        case(ERROR_OUTOFMEMORY):
        {
            kderror = KD_ENOMEM;
            break;
        }
        case(ERROR_END_OF_MEDIA):
        case(ERROR_DISK_FULL):
        case(ERROR_HANDLE_DISK_FULL):
        case(ERROR_NO_DATA_DETECTED):
        case(ERROR_EOM_OVERFLOW):
        {
            kderror = KD_ENOSPC;
            break;
        }
        default:
        {
            /* TODO: Handle other errorcodes */
            kdLogMessagefKHR("kdSetErrorPlatformVEN() encountered unknown errorcode: %d\n", error);
            kdAssert(0);
        }
    }
#else
    switch(error)
    {
        case(EACCES):
        case(EROFS):
        case(EISDIR):
        {
            kderror = KD_EACCES;
            break;
        }
        case(EAGAIN):
        {
            kderror = KD_ETRY_AGAIN;
            break;
        }
        case(EBADF):
        {
            kderror = KD_EBADF;
            break;
        }
        case(EBUSY):
        {
            kderror = KD_EBUSY;
            break;
        }
        case(EEXIST):
        case(ENOTEMPTY):
        {
            kderror = KD_EEXIST;
            break;
        }
        case(EFBIG):
        {
            kderror = KD_EFBIG;
            break;
        }
        case(EINVAL):
        {
            kderror = KD_EINVAL;
            break;
        }
        case(EIO):
        {
            kderror = KD_EIO;
            break;
        }
        case(EMFILE):
        case(ENFILE):
        {
            kderror = KD_EMFILE;
            break;
        }
        case(ENAMETOOLONG):
        {
            kderror = KD_ENAMETOOLONG;
            break;
        }
        case(ENOENT):
        case(ENOTDIR):
        {
            kderror = KD_ENOENT;
            break;
        }
        case(ENOMEM):
        {
            kderror = KD_ENOMEM;
            break;
        }
        case(ENOSPC):
        {
            kderror = KD_ENOSPC;
            break;
        }
        case(EOVERFLOW):
        {
            kderror = KD_EOVERFLOW;
            break;
        }
        default:
        {
            /* TODO: Handle other errorcodes */
            kdLogMessagefKHR("kdSetErrorPlatformVEN() encountered unknown errorcode: %d\n", error);
            kdAssert(0);
        }
    }
#endif

    /* KD errors are 1 to 37*/
    for(KDint i = KD_EACCES; i <= KD_ETRY_AGAIN; i++)
    {
        if(kderror == (allowed & i))
        {
            kdSetError(kderror);
            return;
        }
    }
    /* Error is not in allowed list */
    kdLogMessagefKHR("kdSetErrorPlatformVEN() encountered unexpected errorcode: %d\n", kderror);
    kdAssert(0);
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
#if defined(__EMSCRIPTEN__)
        return "Web (Emscripten)";
#elif defined(__MINGW32__)
        return "Windows (MinGW)";
#elif defined(_WIN32)
        return "Windows";
#elif defined(__ANDROID__)
        return "Android";
#elif defined(__linux__) && defined(KD_WINDOW_SUPPORTED)
        if(kdStrstrVEN(kdGetEnvVEN("XDG_SESSION_TYPE"), "wayland"))
        {
            return "Linux (Wayland)";
        }
        else
        {
            return "Linux (X11)";
        }
#elif defined(__linux__)
        return "Linux";
#elif defined(__APPLE__) && TARGET_OS_IPHONE
        return "iOS";
#elif defined(__APPLE__) && TARGET_OS_MAC
        return "macOS";
#elif defined(__FreeBSD__)
        return "FreeBSD";
#elif defined(__NetBSD__)
        return "NetBSD";
#elif defined(__OpenBSD__)
        return "OpenBSD";
#elif defined(__DragonFly__)
        return "DragonFly BSD";
#else
        return "Unknown";
#endif
    }
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/* kdQueryIndexedAttribcv: Obtain the value of an indexed string OpenKODE Core attribute. */
KD_API const KDchar *KD_APIENTRY kdQueryIndexedAttribcv(KD_UNUSED KDint attribute, KD_UNUSED KDint index)
{
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/******************************************************************************
 * Threads and synchronization
 ******************************************************************************/

/* kdThreadAttrCreate: Create a thread attribute object. */
struct KDThreadAttr {
#if defined(KD_THREAD_POSIX)
    pthread_attr_t nativeattr;
#endif
    KDint detachstate;
    KDsize stacksize;
    KDchar debugname[256];
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
static KDThread *__kdThreadInit(void)
{
    KDThread *thread = (KDThread *)kdMalloc(sizeof(KDThread));
    if(thread == KD_NULL)
    {
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
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
    thread->callbacks = (__KDCallback **)kdMalloc(sizeof(__KDCallback *));
    if(thread->callbacks == KD_NULL)
    {
        __kdQueueFree(thread->eventqueue);
        kdFree(thread);
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
    return thread;
}

static void __kdThreadFree(KDThread *thread)
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
    kdFree(thread);
}

static KDThreadStorageKeyKHR __kd_threadlocal = 0;
static void __kdThreadInitOnce(void)
{
    __kd_threadlocal = kdMapThreadStorageKHR(&__kd_threadlocal);
}

static KDThreadOnce __kd_threadlocal_once = KD_THREAD_ONCE_INIT;
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
static void *__kdThreadRun(void *init)
{
    KDThread *thread = (KDThread *)init;
    kdThreadOnce(&__kd_threadlocal_once, __kdThreadInitOnce);
    /* Set the thread name */
    KD_UNUSED const char *threadname = thread->attr ? thread->attr->debugname : "KDThread";
#if defined(_MSC_VER)
    typedef HRESULT(WINAPI * SETTHREADDESCRIPTION)(HANDLE hThread, PCWSTR lpThreadDescription);
    SETTHREADDESCRIPTION __SetThreadDescription = KD_NULL;
    HMODULE kernel32 = GetModuleHandle("Kernel32.dll");
    if(kernel32)
    {
        __SetThreadDescription = (SETTHREADDESCRIPTION)GetProcAddress(kernel32, "SetThreadDescription");
    }
    if(__SetThreadDescription)
    {
        WCHAR wthreadname[256] = L"KDThread";
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
            /* clang-format off */
            __except(EXCEPTION_CONTINUE_EXECUTION)
            /* clang-format on */
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
    void *result = thread->start_routine(thread->arg);
    if(thread->attr && thread->attr->detachstate == KD_THREAD_CREATE_DETACHED)
    {
        __kdThreadFree(thread);
    }
    return result;
}
#endif

KD_API KDThread *KD_APIENTRY kdThreadCreate(const KDThreadAttr *attr, void *(*start_routine)(void *), void *arg)
{
#if !defined(KD_THREAD_C11) && !defined(KD_THREAD_POSIX) && !defined(KD_THREAD_WIN32)
    kdSetError(KD_ENOSYS);
    return KD_NULL;
#endif
#if defined(__EMSCRIPTEN__)
    if(!emscripten_has_threading_support())
    {
        kdSetError(KD_ENOSYS);
        return KD_NULL;
    }
#endif

    KDThread *thread = __kdThreadInit();
    if(thread == KD_NULL)
    {
        kdSetError(KD_EAGAIN);
        return KD_NULL;
    }
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->attr = attr;

    KDint error = 0;
#if defined(KD_THREAD_C11)
    error = thrd_create(&thread->nativethread, (thrd_start_t)__kdThreadRun, thread);
#elif defined(KD_THREAD_POSIX)
    error = pthread_create(&thread->nativethread, attr ? &attr->nativeattr : KD_NULL, __kdThreadRun, thread);
#elif defined(KD_THREAD_WIN32)
    thread->nativethread = CreateThread(KD_NULL, attr ? attr->stacksize : 0, (LPTHREAD_START_ROUTINE)__kdThreadRun, (LPVOID)thread, 0, KD_NULL);
    error = thread->nativethread ? 0 : 1;
#endif

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
    KDint ipretvalinit = 0;
    KD_UNUSED KDint *ipretval = &ipretvalinit;
    if(retval)
    {
        ipretval = *retval;
    }

    KD_UNUSED KDint error = 0;
    KDint result = 0;
#if defined(KD_THREAD_C11)
    error = thrd_join(thread->nativethread, ipretval);
    if(error == thrd_error)
#elif defined(KD_THREAD_POSIX)
    error = pthread_join(thread->nativethread, retval);
    if(error == EINVAL || error == ESRCH)
#elif defined(KD_THREAD_WIN32)
    error = WaitForSingleObject(thread->nativethread, INFINITE);
    GetExitCodeThread(thread->nativethread, (LPDWORD)ipretval);
    CloseHandle(thread->nativethread);
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
    error = thrd_detach(thread->nativethread);
#elif defined(KD_THREAD_POSIX)
    error = pthread_detach(thread->nativethread);
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
};
KD_API KDThreadMutex *KD_APIENTRY kdThreadMutexCreate(KD_UNUSED const void *mutexattr)
{
    /* TODO: Write KDThreadMutexAttr extension */
    KDThreadMutex *mutex = (KDThreadMutex *)kdMalloc(sizeof(KDThreadMutex));
    if(mutex == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
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
    mutex->nativemutex = KD_FALSE;
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
    if(mutex)
    {
/* No need to free anything on WIN32*/
#if defined(KD_THREAD_C11)
        mtx_destroy(&mutex->nativemutex);
#elif defined(KD_THREAD_POSIX)
        pthread_mutex_destroy(&mutex->nativemutex);
#endif
        kdFree(mutex);
    }
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
    mutex->nativemutex = KD_TRUE;
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
    kdSetError(KD_ENOSYS);
    return KD_NULL;
#endif
#if defined(__EMSCRIPTEN__)
    if(!emscripten_has_threading_support())
    {
        kdSetError(KD_ENOSYS);
        return KD_NULL;
    }
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
struct KDThreadSem {
    KDuint count;
    KDThreadMutex *mutex;
#if defined(KD_THREAD_C11) || defined(KD_THREAD_POSIX) || defined(KD_THREAD_WIN32)
    KDThreadCond *condition;
#endif
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
    struct timespec ts = {0};
    /* Determine seconds from the overall nanoseconds */
    if((timeout % 1000000000) == 0)
    {
        ts.tv_sec = (timeout / 1000000000);
    }
    else
    {
        ts.tv_sec = (timeout - (timeout % 1000000000)) / 1000000000;
    }

    /* Remaining nanoseconds */
    ts.tv_nsec = (KDint32)timeout - ((KDint32)ts.tv_sec * 1000000000);
#endif

#if defined(__EMSCRIPTEN__)
    emscripten_sleep(timeout / 1000000);
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

/******************************************************************************
 * Events
 ******************************************************************************/

/* kdWaitEvent: Get next event from thread's event queue. */
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    _KDQueue *eventqueue = kdThreadSelf()->eventqueue;
    KDEvent *lastevent = kdThreadSelf()->lastevent;
    if(lastevent)
    {
        kdFreeEvent(lastevent);
    }
    if(timeout != -1)
    {
        kdThreadSleepVEN(timeout);
    }
    kdPumpEvents();
    if(__kdQueueSize(eventqueue) > 0)
    {
        lastevent = (KDEvent *)__kdQueuePull(eventqueue);
    }
    else
    {
        lastevent = KD_NULL;
        kdSetError(KD_EAGAIN);
    }
    return lastevent;
}

/* kdSetEventUserptr: Set the userptr for global events. */
static void *__kd_userptr = KD_NULL;
static KDThreadMutex *__kd_userptrmtx = KD_NULL;
KD_API void KD_APIENTRY kdSetEventUserptr(KD_UNUSED void *userptr)
{
    kdThreadMutexLock(__kd_userptrmtx);
    __kd_userptr = userptr;
    kdThreadMutexUnlock(__kd_userptrmtx);
}

/* kdDefaultEvent: Perform default processing on an unrecognized event. */
KD_API void KD_APIENTRY kdDefaultEvent(KD_UNUSED const KDEvent *event)
{
}

/* kdPumpEvents: Pump the thread's event queue, performing callbacks. */
struct __KDCallback {
    KDCallbackFunc *func;
    KDint eventtype;
    void *eventuserptr;
};
static KDboolean __kdExecCallback(KDEvent *event)
{   
    KDint callbackindex = kdThreadSelf()->callbackindex;
    __KDCallback **callbacks = kdThreadSelf()->callbacks;
    for(KDint i = 0; i < callbackindex; i++)
    {
        if(callbacks[i]->func)
        {
            KDboolean typematch = (callbacks[i]->eventtype == event->type) || (callbacks[i]->eventtype == 0);
            KDboolean userptrmatch = (callbacks[i]->eventuserptr == event->userptr);
            if(typematch && userptrmatch)
            {
                callbacks[i]->func(event);
                kdFreeEvent(event);
                return KD_TRUE;
            }
        }
    }
    return KD_FALSE;
}

#ifdef KD_WINDOW_SUPPORTED
struct KDWindow {
    void *nativewindow;
    void *nativedisplay;
    EGLenum platform;
    EGLint format;
    void *eventuserptr;
    KDThread *originthr;
    struct {
        KDint32 width;
        KDint32 height; 
    } screen;
    struct
    {
        KDchar caption[256];
        KDboolean focused;
        KDboolean visible;
        KDint32 width;
        KDint32 height;
    } properties;
    struct
    {
        struct 
        {
            KDint32 availability;
            KDint32 select;
            KDint32 x;
            KDint32 y;
        } pointer;
        struct 
        {
            KDint32 availability;
            KDint32 flags;
            KDint32 character;
            KDint32 keycode;
            KDint32 charflags;
        } keyboard;
        struct 
        {
            KDint32 availability;
            KDint32 up;
            KDint32 left;
            KDint32 right;
            KDint32 down;
            KDint32 select;
        } dpad;
        struct 
        {
            KDint32 availability;
            KDint32 up;
            KDint32 left;
            KDint32 right;
            KDint32 down;
            KDint32 fire;
        } gamekeys;
    } states;
#if defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    struct
    {
        struct xkb_context *context;
        struct xkb_state *state;
        struct xkb_keymap *keymap;
        KDuint8 firstevent;
    } xkb;
#endif
#if defined(KD_WINDOW_WAYLAND)
    struct
    {
        struct wl_surface *surface;
        struct wl_shell_surface *shell_surface;
        struct wl_registry *registry;
        struct wl_compositor *compositor;
        struct wl_shell *shell;
        struct wl_seat *seat;
        struct wl_keyboard *keyboard;
        struct wl_pointer *pointer;
    } wayland;
#endif
#if defined(KD_WINDOW_WAYLAND)
    struct
    {
        xcb_ewmh_connection_t ewmh;
    } xcb;
#endif
};

static KDWindow *__kd_window = KD_NULL;

#ifdef __ANDROID__
static AInputQueue *__kd_androidinputqueue = KD_NULL;
static KDThreadMutex *__kd_androidinputqueue_mutex = KD_NULL;
#endif

#if defined(KD_WINDOW_WAYLAND)
static struct wl_display *__kd_wl_display;
#endif

static void __kdHandleSpecialKeys(KDWindow *window, KDEventInputKeyATX *keyevent)
{
    KDEvent *dpadevent = kdCreateEvent();
    dpadevent->type = KD_EVENT_INPUT;
    KDEvent *gamekeysevent = kdCreateEvent();
    gamekeysevent->type = KD_EVENT_INPUT;

    switch(keyevent->keycode) 
    {
        case(KD_KEY_UP_ATX): 
        {
            window->states.dpad.up = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_UP; 
            dpadevent->data.input.value.i = window->states.dpad.up;

            window->states.gamekeys.up = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_UP; 
            gamekeysevent->data.input.value.i = window->states.gamekeys.up;
            break;
        }
        case(KD_KEY_LEFT_ATX): 
        {
            window->states.dpad.left = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_LEFT; 
            dpadevent->data.input.value.i = window->states.dpad.left;

            window->states.gamekeys.left = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_LEFT; 
            gamekeysevent->data.input.value.i = window->states.gamekeys.left;
            break;
        }
        case(KD_KEY_RIGHT_ATX): 
        {
            window->states.dpad.right = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_RIGHT; 
            dpadevent->data.input.value.i = window->states.dpad.right;

            window->states.gamekeys.right = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_RIGHT; 
            gamekeysevent->data.input.value.i = window->states.gamekeys.right;
            break;
        }
        case(KD_KEY_DOWN_ATX): 
        {
            window->states.dpad.down = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_DOWN; 
            dpadevent->data.input.value.i = window->states.dpad.down;

            window->states.gamekeys.down = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_DOWN; 
            gamekeysevent->data.input.value.i = window->states.gamekeys.down;
            break;
        }
        case(KD_KEY_ENTER_ATX):
        {
            window->states.dpad.select = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_SELECT; 
            dpadevent->data.input.value.i = window->states.dpad.select;

            window->states.gamekeys.fire = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_FIRE; 
            gamekeysevent->data.input.value.i = window->states.gamekeys.fire;
            break;
        }
        default: 
        {
            kdFreeEvent(dpadevent);
            kdFreeEvent(gamekeysevent);
            return;
        }
    }

    if(!__kdExecCallback(dpadevent)) 
    { 
        kdPostEvent(dpadevent); 
    } 
    if(!__kdExecCallback(gamekeysevent)) 
    { 
        kdPostEvent(gamekeysevent); 
    }
}

static KDint32 __KDKeycodeLookup(KDint32 keycode)
{
    switch(keycode)
    {
#if defined(KD_WINDOW_ANDROID)
        /* KD_KEY_ACCEPT_ATX */
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        case(AKEYCODE_EISU):
        {
            return KD_KEY_ALPHANUMERIC_ATX;
        }
        case(AKEYCODE_ALT_LEFT):
        case(AKEYCODE_ALT_RIGHT):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX
           KD_KEY_APPS_ATX
           KD_KEY_ATTN_ATX */
        case(AKEYCODE_BACK):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        /* KD_KEY_BROWSERFAVORITES_ATX
           KD_KEY_BROWSERFORWARD_ATX
           KD_KEY_BROWSERHOME_ATX
           KD_KEY_BROWSERREFRESH_ATX
           KD_KEY_BROWSERSEARCH_ATX
           KD_KEY_BROWSERSTOP_ATX */
        case(AKEYCODE_CAPS_LOCK):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(AKEYCODE_CLEAR):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX */
        /* KD_KEY_COMPOSE_ATX */
        case(AKEYCODE_CTRL_LEFT):
        case(AKEYCODE_CTRL_RIGHT):
        {
            return KD_KEY_CONTROL_ATX;
        }
        /* KD_KEY_CRSEL_ATX
           KD_KEY_CONVERT_ATX */
        case(AKEYCODE_COPY):
        {
            return KD_KEY_COPY_ATX;
        }
        case(AKEYCODE_CUT):
        {
            return KD_KEY_CUT_ATX;
        }
        case(AKEYCODE_DPAD_DOWN):
        {
            return KD_KEY_DOWN_ATX;
        }
        /* KD_KEY_END_ATX */
        case(AKEYCODE_ENTER):
        {
            return KD_KEY_ENTER_ATX;
        }
        /* KD_KEY_ERASEEOF_ATX
           KD_KEY_EXECUTE_ATX
           KD_KEY_EXSEL_ATX */
        case(AKEYCODE_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(AKEYCODE_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(AKEYCODE_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(AKEYCODE_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(AKEYCODE_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(AKEYCODE_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(AKEYCODE_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(AKEYCODE_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(AKEYCODE_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(AKEYCODE_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(AKEYCODE_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(AKEYCODE_F12):
        {
            return KD_KEY_F12_ATX;
        }
        /* KD_KEY_F13_ATX
           KD_KEY_F14_ATX
           KD_KEY_F15_ATX
           KD_KEY_F16_ATX
           KD_KEY_F17_ATX
           KD_KEY_F18_ATX
           KD_KEY_F19_ATX
           KD_KEY_F20_ATX
           KD_KEY_F21_ATX
           KD_KEY_F22_ATX
           KD_KEY_F23_ATX
           KD_KEY_F24_ATX
           KD_KEY_FINALMODE_ATX
           KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX
           KD_KEY_HANGULMODE_ATX
           KD_KEY_HANJAMODE_ATX */
        case(AKEYCODE_HELP):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(AKEYCODE_HOME):
        {
            return KD_KEY_HOME_ATX;
        }
        case(AKEYCODE_DEL):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        /* KD_KEY_LAUNCHAPPLICATION1_ATX
           KD_KEY_LAUNCHAPPLICATION2_ATX */
        case(AKEYCODE_ENVELOPE):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(AKEYCODE_SOFT_LEFT):
        case(AKEYCODE_DPAD_LEFT):
        {
            return KD_KEY_LEFT_ATX;
        }
        /* KD_KEY_META_ATX */
        case(AKEYCODE_MEDIA_NEXT):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(AKEYCODE_MEDIA_PLAY_PAUSE):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(AKEYCODE_MEDIA_PREVIOUS):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(AKEYCODE_MEDIA_STOP):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        /* KD_KEY_MODECHANGE_ATX
           KD_KEY_NONCONVERT_ATX */
        case(AKEYCODE_NUM_LOCK):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(AKEYCODE_PAGE_DOWN):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(AKEYCODE_PAGE_UP):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(AKEYCODE_MEDIA_PAUSE):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(AKEYCODE_MEDIA_PLAY):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(AKEYCODE_SYSRQ):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX
           KD_KEY_PROPS_ATX */
        case(AKEYCODE_SOFT_RIGHT):
        case(AKEYCODE_DPAD_RIGHT):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(AKEYCODE_MOVE_END):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(AKEYCODE_BUTTON_SELECT):
        {
            return KD_KEY_SELECT_ATX;
        }
        case(AKEYCODE_MEDIA_TOP_MENU):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(AKEYCODE_SHIFT_LEFT):
        case(AKEYCODE_SHIFT_RIGHT):
        {
            return KD_KEY_SHIFT_ATX;
        }
        /* KD_KEY_STOP_ATX */
        case(AKEYCODE_DPAD_UP):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(AKEYCODE_VOLUME_DOWN):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(AKEYCODE_VOLUME_MUTE):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(AKEYCODE_VOLUME_UP):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        /* KD_KEY_WIN_ATX */
        case(AKEYCODE_ZOOM_IN):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_WIN32)
        case(VK_ACCEPT):
        {
            return KD_KEY_ACCEPT_ATX;
        }
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(VK_MENU):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        case(VK_APPS):
        {
            return KD_KEY_APPS_ATX;
        }
        case(VK_ATTN):
        {
            return KD_KEY_ATTN_ATX;
        }
        case(VK_BACK):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        case(VK_BROWSER_FAVORITES):
        {
            return KD_KEY_BROWSERFAVORITES_ATX;
        }
        case(VK_BROWSER_FORWARD):
        {
            return KD_KEY_BROWSERFORWARD_ATX;
        }
        case(VK_BROWSER_HOME):
        {
            return KD_KEY_BROWSERHOME_ATX;
        }
        case(VK_BROWSER_REFRESH):
        {
            return KD_KEY_BROWSERREFRESH_ATX;
        }
        case(VK_BROWSER_SEARCH):
        {
            return KD_KEY_BROWSERSEARCH_ATX;
        }
        case(VK_BROWSER_STOP):
        {
            return KD_KEY_BROWSERSTOP_ATX;
        }
        case(VK_CAPITAL):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(VK_CLEAR):
        case(VK_OEM_CLEAR):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX */
        /* KD_KEY_COMPOSE_ATX */
        case(VK_CONTROL):
        {
            return KD_KEY_CONTROL_ATX;
        }
        case(VK_CRSEL):
        {
            return KD_KEY_CRSEL_ATX;
        }
        case(VK_CONVERT):
        {
            return KD_KEY_CONVERT_ATX;
        }
        /* KD_KEY_COPY_ATX */
        /* KD_KEY_CUT_ATX */
        case(VK_DOWN):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(VK_END):
        {
            return KD_KEY_END_ATX;
        }
        case(VK_RETURN):
        {
            return KD_KEY_ENTER_ATX;
        }
        case(VK_EREOF):
        {
            return KD_KEY_ERASEEOF_ATX;
        }
        case(VK_EXECUTE):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        case(VK_EXSEL):
        {
            return KD_KEY_EXSEL_ATX;
        }
        case(VK_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(VK_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(VK_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(VK_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(VK_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(VK_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(VK_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(VK_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(VK_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(VK_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(VK_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(VK_F12):
        {
            return KD_KEY_F12_ATX;
        }
        case(VK_F13):
        {
            return KD_KEY_F13_ATX;
        }
        case(VK_F14):
        {
            return KD_KEY_F14_ATX;
        }
        case(VK_F15):
        {
            return KD_KEY_F15_ATX;
        }
        case(VK_F16):
        {
            return KD_KEY_F16_ATX;
        }
        case(VK_F17):
        {
            return KD_KEY_F17_ATX;
        }
        case(VK_F18):
        {
            return KD_KEY_F18_ATX;
        }
        case(VK_F19):
        {
            return KD_KEY_F19_ATX;
        }
        case(VK_F20):
        {
            return KD_KEY_F20_ATX;
        }
        case(VK_F21):
        {
            return KD_KEY_F21_ATX;
        }
        case(VK_F22):
        {
            return KD_KEY_F22_ATX;
        }
        case(VK_F23):
        {
            return KD_KEY_F23_ATX;
        }
        case(VK_F24):
        {
            return KD_KEY_F24_ATX;
        }
        case(VK_FINAL):
        {
            return KD_KEY_FINALMODE_ATX;
        }
        /* KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX */
        case(VK_HANGUL):
        {
            return KD_KEY_HANGULMODE_ATX;
        }
        case(VK_HANJA):
        {
            return KD_KEY_HANJAMODE_ATX;
        }
        case(VK_HELP):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(VK_HOME):
        {
            return KD_KEY_HOME_ATX;
        }
        case(VK_INSERT):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        case(VK_LAUNCH_APP1):
        {
            return KD_KEY_LAUNCHAPPLICATION1_ATX;
        }
        case(VK_LAUNCH_APP2):
        {
            return KD_KEY_LAUNCHAPPLICATION2_ATX;
        }
        case(VK_LAUNCH_MAIL):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(VK_LEFT):
        {
            return KD_KEY_LEFT_ATX;
        }
        /* KD_KEY_META_ATX */
        case(VK_MEDIA_NEXT_TRACK):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(VK_MEDIA_PLAY_PAUSE):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(VK_MEDIA_PREV_TRACK):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(VK_MEDIA_STOP):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        case(VK_MODECHANGE):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        case(VK_NONCONVERT):
        {
            return KD_KEY_NONCONVERT_ATX;
        }
        case(VK_NUMLOCK):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(VK_NEXT):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(VK_PRIOR):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(VK_PAUSE):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(VK_PLAY):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(VK_PRINT):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        case(VK_PROCESSKEY):
        {
            return KD_KEY_PROCESS_ATX;
        }
        /* KD_KEY_PROPS_ATX */
        case(VK_RIGHT):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(VK_SCROLL):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(VK_SELECT):
        {
            return KD_KEY_SELECT_ATX;
        }
        case(VK_LAUNCH_MEDIA_SELECT):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(VK_SHIFT):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(VK_CANCEL):
        {
            return KD_KEY_STOP_ATX;
        }
        case(VK_UP):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(VK_VOLUME_DOWN):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(VK_VOLUME_MUTE):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(VK_VOLUME_UP):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(VK_LWIN):
        case(VK_RWIN):
        {
            return KD_KEY_WIN_ATX;
        }
        case(VK_ZOOM):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_EMSCRIPTEN)
        case(30):
        {
            return KD_KEY_ACCEPT_ATX;
        }
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(18):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        /* KD_KEY_APPS_ATX */
        case(240):
        case(246):
        {
            return KD_KEY_ATTN_ATX;
        }
        /* KD_KEY_BROWSERBACK_ATX
           KD_KEY_BROWSERFAVORITES_ATX
           KD_KEY_BROWSERFORWARD_ATX
           KD_KEY_BROWSERHOME_ATX
           KD_KEY_BROWSERREFRESH_ATX
           KD_KEY_BROWSERSEARCH_ATX
           KD_KEY_BROWSERSTOP_ATX */
        case(20):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(12):
        case(230):
        case(254):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX
           KD_KEY_COMPOSE_ATX */
        case(17):
        {
            return KD_KEY_CONTROL_ATX;
        }
        case(247):
        {
            return KD_KEY_CRSEL_ATX;
        }
        case(28):
        {
            return KD_KEY_CONVERT_ATX;
        }
        case(242):
        {
            return KD_KEY_COPY_ATX;
        }
        /* KD_KEY_CUT_ATX */
        case(40):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(35):
        {
            return KD_KEY_END_ATX;
        }
        case(13):
        {
            return KD_KEY_ENTER_ATX;
        }
        case(249):
        {
            return KD_KEY_ERASEEOF_ATX;
        }
        case(43):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        case(248):
        {
            return KD_KEY_EXSEL_ATX;
        }
        case(112):
        {
            return KD_KEY_F1_ATX;
        }
        case(113):
        {
            return KD_KEY_F2_ATX;
        }
        case(114):
        {
            return KD_KEY_F3_ATX;
        }
        case(115):
        {
            return KD_KEY_F4_ATX;
        }
        case(116):
        {
            return KD_KEY_F5_ATX;
        }
        case(117):
        {
            return KD_KEY_F6_ATX;
        }
        case(118):
        {
            return KD_KEY_F7_ATX;
        }
        case(119):
        {
            return KD_KEY_F8_ATX;
        }
        case(120):
        {
            return KD_KEY_F9_ATX;
        }
        case(121):
        {
            return KD_KEY_F10_ATX;
        }
        case(122):
        {
            return KD_KEY_F11_ATX;
        }
        case(123):
        {
            return KD_KEY_F12_ATX;
        }
        case(124):
        {
            return KD_KEY_F13_ATX;
        }
        case(125):
        {
            return KD_KEY_F14_ATX;
        }
        case(126):
        {
            return KD_KEY_F15_ATX;
        }
        case(127):
        {
            return KD_KEY_F16_ATX;
        }
        case(128):
        {
            return KD_KEY_F17_ATX;
        }
        case(129):
        {
            return KD_KEY_F18_ATX;
        }
        case(130):
        {
            return KD_KEY_F19_ATX;
        }
        case(131):
        {
            return KD_KEY_F20_ATX;
        }
        case(132):
        {
            return KD_KEY_F21_ATX;
        }
        case(133):
        {
            return KD_KEY_F22_ATX;
        }
        case(134):
        {
            return KD_KEY_F23_ATX;
        }
        case(135):
        {
            return KD_KEY_F24_ATX;
        }
        case(24):
        {
            return KD_KEY_FINALMODE_ATX;
        }
        /* KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX */
        case(21):
        {
            return KD_KEY_HANGULMODE_ATX;
        }
        case(25):
        {
            return KD_KEY_HANJAMODE_ATX;
        }
        case(6):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(36):
        {
            return KD_KEY_HOME_ATX;
        }
        case(45):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        /* KD_KEY_LAUNCHAPPLICATION1_ATX
           KD_KEY_LAUNCHAPPLICATION2_ATX
           KD_KEY_LAUNCHMAIL_ATX*/
        case(37):
        {
            return KD_KEY_LEFT_ATX;
        }
        case(224):
        {
            return KD_KEY_META_ATX;
        }
        /* KD_KEY_MEDIANEXTTRACK_ATX;
           KD_KEY_MEDIAPLAYPAUSE_ATX
           KD_KEY_MEDIAPREVIOUSTRACK_ATX
           KD_KEY_MEDIASTOP_ATX */
        case(31):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        case(29):
        {
            return KD_KEY_NONCONVERT_ATX;
        }
        case(144):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(34):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(33):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(19):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(250):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(42):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX */
        /* KD_KEY_PROPS_ATX */
        case(39):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(145):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(41):
        {
            return KD_KEY_SELECT_ATX;
        }
        /* KD_KEY_SELECTMEDIA_ATX */
        case(16):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(3):
        {
            return KD_KEY_STOP_ATX;
        }
        case(38):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(182):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(181):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(183):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(91):
        {
            return KD_KEY_WIN_ATX;
        }
        case(251):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
        /* KD_KEY_ACCEPT_ATX */
        case(XKB_KEY_Redo):
        {
            return KD_KEY_AGAIN_ATX;
        }
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(XKB_KEY_Alt_L):
        case(XKB_KEY_Alt_R):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        /* KD_KEY_APPS_ATX */
        /* KD_KEY_ATTN_ATX */
        case(XKB_KEY_XF86Back):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        case(XKB_KEY_XF86Favorites):
        {
            return KD_KEY_BROWSERFAVORITES_ATX;
        }
        case(XKB_KEY_XF86Forward):
        {
            return KD_KEY_BROWSERFORWARD_ATX;
        }
        case(XKB_KEY_XF86HomePage):
        {
            return KD_KEY_BROWSERHOME_ATX;
        }
        case(XKB_KEY_XF86Refresh):
        {
            return KD_KEY_BROWSERREFRESH_ATX;
        }
        case(XKB_KEY_XF86Search):
        {
            return KD_KEY_BROWSERSEARCH_ATX;
        }
        case(XKB_KEY_XF86Stop):
        {
            return KD_KEY_BROWSERSTOP_ATX;
        }
        case(XKB_KEY_Caps_Lock):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(XKB_KEY_Clear):
        {
            return KD_KEY_CLEAR_ATX;
        }
        case(XKB_KEY_Codeinput):
        {
            return KD_KEY_CODEINPUT_ATX;
        }
        case(XKB_KEY_Multi_key):
        {
            return KD_KEY_COMPOSE_ATX;
        }
        case(XKB_KEY_Control_L):
        case(XKB_KEY_Control_R):
        {
            return KD_KEY_CONTROL_ATX;
        }
        /* KD_KEY_CRSEL_ATX */
        /* KD_KEY_CONVERT_ATX */
        case(XKB_KEY_XF86Copy):
        {
            return KD_KEY_COPY_ATX;
        }
        case(XKB_KEY_XF86Cut):
        {
            return KD_KEY_CUT_ATX;
        }
        case(XKB_KEY_Down):
        case(XKB_KEY_KP_Down):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(XKB_KEY_End):
        case(XKB_KEY_KP_End):
        {
            return KD_KEY_END_ATX;
        }
        case(XKB_KEY_Return):
        case(XKB_KEY_KP_Enter):
        case(XKB_KEY_ISO_Enter):
        {
            return KD_KEY_ENTER_ATX;
        }
        /* KD_KEY_ERASEEOF_ATX */
        case(XKB_KEY_Execute):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        /* KD_KEY_EXSEL_ATX */
        case(XKB_KEY_F1):
        case(XKB_KEY_KP_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(XKB_KEY_F2):
        case(XKB_KEY_KP_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(XKB_KEY_F3):
        case(XKB_KEY_KP_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(XKB_KEY_F4):
        case(XKB_KEY_KP_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(XKB_KEY_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(XKB_KEY_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(XKB_KEY_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(XKB_KEY_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(XKB_KEY_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(XKB_KEY_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(XKB_KEY_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(XKB_KEY_F12):
        {
            return KD_KEY_F12_ATX;
        }
        case(XKB_KEY_F13):
        {
            return KD_KEY_F13_ATX;
        }
        case(XKB_KEY_F14):
        {
            return KD_KEY_F14_ATX;
        }
        case(XKB_KEY_F15):
        {
            return KD_KEY_F15_ATX;
        }
        case(XKB_KEY_F16):
        {
            return KD_KEY_F16_ATX;
        }
        case(XKB_KEY_F17):
        {
            return KD_KEY_F17_ATX;
        }
        case(XKB_KEY_F18):
        {
            return KD_KEY_F18_ATX;
        }
        case(XKB_KEY_F19):
        {
            return KD_KEY_F19_ATX;
        }
        case(XKB_KEY_F20):
        {
            return KD_KEY_F20_ATX;
        }
        case(XKB_KEY_F21):
        {
            return KD_KEY_F21_ATX;
        }
        case(XKB_KEY_F22):
        {
            return KD_KEY_F22_ATX;
        }
        case(XKB_KEY_F23):
        {
            return KD_KEY_F23_ATX;
        }
        case(XKB_KEY_F24):
        {
            return KD_KEY_F24_ATX;
        }
        /* KD_KEY_FINALMODE_ATX */
        case(XKB_KEY_Find):
        {
            return KD_KEY_FIND_ATX;
        }
        /* KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX
           KD_KEY_HANGULMODE_ATX
           KD_KEY_HANJAMODE_ATX */
        case(XKB_KEY_Help):
        {
            return KD_KEY_HELP_ATX;
        }
        case(XKB_KEY_Hiragana):
        {
            return KD_KEY_HIRAGANA_ATX;
        }
        case(XKB_KEY_Home):
        {
            return KD_KEY_HOME_ATX;
        }
        case(XKB_KEY_Insert):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        case(XKB_KEY_XF86Launch0):
        {
            return KD_KEY_LAUNCHAPPLICATION1_ATX;
        }
        case(XKB_KEY_XF86Launch1):
        {
            return KD_KEY_LAUNCHAPPLICATION2_ATX;
        }
        case(XKB_KEY_XF86Mail):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(XKB_KEY_Left):
        {
            return KD_KEY_LEFT_ATX;
        }
        case(XKB_KEY_Meta_L):
        case(XKB_KEY_Meta_R):
        {
            return KD_KEY_META_ATX;
        }
        case(XKB_KEY_XF86AudioForward):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(XKB_KEY_XF86AudioPlay):
        case(XKB_KEY_XF86AudioPause):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(XKB_KEY_XF86AudioPrev):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(XKB_KEY_XF86AudioStop):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        case(XKB_KEY_Mode_switch):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        /* KD_KEY_NONCONVERT_ATX */
        case(XKB_KEY_Num_Lock):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(XKB_KEY_Page_Down):
        case(XKB_KEY_KP_Page_Down):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(XKB_KEY_KP_Up):
        case(XKB_KEY_KP_Page_Up):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(XKB_KEY_Pause):
        {
            return KD_KEY_PAUSE_ATX;
        }
        /* KD_KEY_PLAY_ATX */
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(XKB_KEY_Print):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX */
        /* KD_KEY_PROPS_ATX */
        case(XKB_KEY_Right):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        /* KD_KEY_SCROLL_ATX */
        case(XKB_KEY_Select):
        {
            return KD_KEY_SELECT_ATX;
            break;
        }
        case(XKB_KEY_XF86AudioMedia):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(XKB_KEY_Shift_L):
        case(XKB_KEY_Shift_R):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(XKB_KEY_Cancel):
        {
            return KD_KEY_STOP_ATX;
        }
        case(XKB_KEY_Up):
        {
            return KD_KEY_UP_ATX;
        }
        case(XKB_KEY_Undo):
        {
            return KD_KEY_UNDO_ATX;
        }
        case(XKB_KEY_XF86AudioLowerVolume):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(XKB_KEY_XF86AudioMute):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(XKB_KEY_XF86AudioRaiseVolume):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(XKB_KEY_Super_L):
        case(XKB_KEY_Super_R):
        {
            return KD_KEY_WIN_ATX;
        }
/* KD_KEY_ZOOM_ATX */
#endif
        default:
        {
            return 0;
        }
    }
}
#endif

KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
    KDsize queuesize = __kdQueueSize(kdThreadSelf()->eventqueue);
    for(KDuint i = 0; i < queuesize; i++)
    {
        KDEvent *callbackevent = __kdQueuePull(kdThreadSelf()->eventqueue);
        if(callbackevent)
        {
            if(!__kdExecCallback(callbackevent))
            {
                /* Not a callback */
                kdPostEvent(callbackevent);
            }
        }
    }

#ifdef KD_WINDOW_SUPPORTED
    KD_UNUSED KDWindow *window = __kd_window;
#if defined(KD_WINDOW_ANDROID)
    AInputEvent *aevent = KD_NULL;
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    if(__kd_androidinputqueue)
    {
        while(AInputQueue_getEvent(__kd_androidinputqueue, &aevent) >= 0)
        {
            AInputQueue_preDispatchEvent(__kd_androidinputqueue, aevent);
            KDEvent *event = kdCreateEvent();
            switch(AInputEvent_getType(aevent))
            {
                case(AINPUT_EVENT_TYPE_KEY):
                {
                    event->type = KD_EVENT_INPUT_KEY_ATX;
                    KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&event->data);

                    if(AKeyEvent_getAction(aevent) == AKEY_EVENT_ACTION_DOWN)
                    {
                        keyevent->flags = KD_KEY_PRESS_ATX;
                    }
                    else
                    {
                        keyevent->flags = 0;
                    }

                    KDint32 keycode = AKeyEvent_getKeyCode(aevent);
                    keyevent->keycode = __KDKeycodeLookup(keycode);
                    if(!keyevent->keycode)
                    {
                        event->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&event->data);
                        keycharevent->character = keycode;
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
                    break;
                }
            }
            AInputQueue_finishEvent(__kd_androidinputqueue, aevent, 1);
        }
    }
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
#elif defined(KD_WINDOW_WIN32)
    if(window)
    {
        MSG msg = {0};
        while(PeekMessage(&msg, window->nativewindow, 0, 0, PM_REMOVE) != 0)
        {
            KDEvent *kdevent = kdCreateEvent();
            kdevent->userptr = window->eventuserptr;
            switch(msg.message)
            {
                case WM_CLOSE:
                case WM_DESTROY:
                case WM_QUIT:
                {
                    ShowWindow(window->nativewindow, SW_HIDE);
                    kdevent->type = KD_EVENT_WINDOW_CLOSE;
                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case WM_INPUT:
                {
                    KDchar buffer[sizeof(RAWINPUT)] = {0};
                    KDsize size = sizeof(RAWINPUT);
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, buffer, (PUINT)&size, sizeof(RAWINPUTHEADER));
                    RAWINPUT *raw = (RAWINPUT *)buffer;
                    if(raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                            kdevent->data.inputpointer.select = 1;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.select = kdevent->data.inputpointer.select;
                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                        else if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                            kdevent->data.inputpointer.select = 0;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.select = kdevent->data.inputpointer.select;
                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                        else if(raw->data.keyboard.Flags & MOUSE_MOVE_ABSOLUTE)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                    }
                    else if(raw->header.dwType == RIM_TYPEKEYBOARD)
                    {
                        kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                        KDint32 character = 0;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6313)
#endif
                        if(raw->data.keyboard.Flags & RI_KEY_MAKE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                        {
                            GetKeyNameText((KDint64)MapVirtualKey(raw->data.keyboard.VKey, MAPVK_VK_TO_VSC) << 16, (KDchar *)&character, sizeof(KDint32));
                        }

                        if(character)
                        {
                            kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                            KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
                            keycharevent->flags = 0;
                            keycharevent->character = character;

                            window->states.keyboard.charflags = keycharevent->flags;
                            window->states.keyboard.character = keycharevent->character;
                        }
                        else
                        {
                            KDint32 keycode = __KDKeycodeLookup(raw->data.keyboard.VKey);
                            if(keycode)
                            {
                                kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                                KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

                                keyevent->flags = 0;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6313)
#endif
                                if(raw->data.keyboard.Flags & RI_KEY_MAKE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                                {
                                    keyevent->flags |= KD_KEY_PRESS_ATX;
                                }
                                keyevent->keycode = keycode;

                                window->states.keyboard.flags = keyevent->flags;
                                window->states.keyboard.keycode = keyevent->keycode;

                                __kdHandleSpecialKeys(window, keyevent);
                            }
                            else
                            {
                                kdFreeEvent(kdevent);
                                break;
                            }
                        }
                    }
                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                default:
                {
                    kdFreeEvent(kdevent);
                    DispatchMessage(&msg);
                    break;
                }
            }
        }
    }
#elif defined(KD_WINDOW_EMSCRIPTEN)
    emscripten_sleep(1);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_X11)
    if(window && window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_generic_event_t *event;
        while((event = xcb_poll_for_event(window->nativedisplay)))
        {
            KDEvent *kdevent = kdCreateEvent();
            kdevent->userptr = window->eventuserptr;
            KDuint type = event->response_type & ~0x80;
            switch(type)
            {
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE:
                {
                    xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                    kdevent->type = KD_EVENT_INPUT_POINTER;
                    kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                    kdevent->data.inputpointer.select = (type == XCB_BUTTON_PRESS) ? 1 : 0;
                    kdevent->data.inputpointer.x = press->event_x;
                    kdevent->data.inputpointer.y = press->event_y;

                    window->states.pointer.select = kdevent->data.inputpointer.select;
                    window->states.pointer.x = kdevent->data.inputpointer.x;
                    window->states.pointer.y = kdevent->data.inputpointer.y;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t *press = (xcb_key_press_event_t *)event;
                    xkb_keysym_t keysym = xkb_state_key_get_one_sym(window->xkb.state, press->detail);

                    KDuint32 flags = 0;
                    KDint32 character = 0;
                    static xcb_key_press_event_t *lastpress = KD_NULL;
                    if(type == XCB_KEY_PRESS)
                    {
                        if(lastpress && ((lastpress->response_type & ~0x80) == XCB_KEY_RELEASE) && (lastpress->detail == press->detail) && (lastpress->time == press->time))
                        {
                            flags = KD_KEY_AUTOREPEAT_ATX;
                        }

                        /* Printable ASCII range. */
                        if((keysym >= 20) && (keysym <= 126))
                        {
                            character = keysym;
                        }
                    }
                    lastpress = press;

                    if(character)
                    {
                        kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
                        keycharevent->flags = flags;
                        keycharevent->character = character;

                        window->states.keyboard.charflags = keycharevent->flags;
                        window->states.keyboard.character = keycharevent->character;
                    }
                    else
                    {
                        KDint32 keycode = __KDKeycodeLookup(keysym);
                        if(keycode)
                        {
                            kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                            KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);
                            keyevent->flags = 0;
                            if(type == XCB_KEY_PRESS)
                            {
                                keyevent->flags |= KD_KEY_PRESS_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
                            }
                            keyevent->keycode = keycode;

                            window->states.keyboard.flags = keyevent->flags;
                            window->states.keyboard.keycode = keyevent->keycode;
                            
                            __kdHandleSpecialKeys(window, keyevent);
                        }
                        else
                        {
                            kdFreeEvent(kdevent);
                            break;
                        }
                    }

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_MOTION_NOTIFY:
                {
                    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
                    kdevent->type = KD_EVENT_INPUT_POINTER;
                    kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
                    kdevent->data.inputpointer.select = 0;
                    kdevent->data.inputpointer.x = motion->event_x;
                    kdevent->data.inputpointer.y = motion->event_y;

                    window->states.pointer.select = kdevent->data.inputpointer.select;
                    window->states.pointer.x = kdevent->data.inputpointer.x;
                    window->states.pointer.y = kdevent->data.inputpointer.y;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_ENTER_NOTIFY:
                case XCB_LEAVE_NOTIFY:
                {
                    kdevent->type = KD_EVENT_WINDOW_FOCUS;
                    kdevent->data.windowfocus.focusstate = (type == XCB_ENTER_NOTIFY) ? 1 : 0;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_CLIENT_MESSAGE:
                {
                    xcb_intern_atom_cookie_t delcookie = xcb_intern_atom(window->nativedisplay, 0, 16, "WM_DELETE_WINDOW");
                    xcb_intern_atom_reply_t *delreply = xcb_intern_atom_reply(window->nativedisplay, delcookie, 0);
                    if((*(xcb_client_message_event_t *)event).data.data32[0] == (*delreply).atom)
                    {
                        kdevent->type = KD_EVENT_WINDOW_CLOSE;
                        if(!__kdExecCallback(kdevent))
                        {
                            kdPostEvent(kdevent);
                        }
                        break;
                    }
                    kdFreeEvent(kdevent);
                    break;
                }
                default:
                {
                    if(type == window->xkb.firstevent)
                    {
                        switch(event->pad0)
                        {
                            case XCB_XKB_NEW_KEYBOARD_NOTIFY:
                            case XCB_XKB_MAP_NOTIFY:
                            {
                                KDint32 device = xkb_x11_get_core_keyboard_device_id(window->nativedisplay);
                                xkb_keymap_unref(window->xkb.keymap);
                                window->xkb.keymap = xkb_x11_keymap_new_from_device(window->xkb.context, window->nativedisplay, device, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                xkb_state_unref(window->xkb.state);
                                window->xkb.state = xkb_x11_state_new_from_device(window->xkb.keymap, window->nativedisplay, device);
                                break;
                            }
                            case XCB_XKB_STATE_NOTIFY:
                            {
                                xcb_xkb_state_notify_event_t *statenotify = (xcb_xkb_state_notify_event_t *)event;
                                xkb_state_update_mask(window->xkb.state,
                                    statenotify->baseMods, statenotify->latchedMods, statenotify->lockedMods,
                                    statenotify->baseGroup, statenotify->latchedGroup, statenotify->lockedGroup);
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                    kdFreeEvent(kdevent);
                    break;
                }
            }
        }
    }
#endif
#if defined(KD_WINDOW_WAYLAND)
    if(window && window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        wl_display_dispatch_pending(window->nativedisplay);
    }
#endif
#endif
#endif
    return 0;
}

/* kdInstallCallback: Install or remove a callback function for event processing. */
KD_API KDint KD_APIENTRY kdInstallCallback(KDCallbackFunc *func, KDint eventtype, void *eventuserptr)
{
    KDint callbackindex = kdThreadSelf()->callbackindex;
    __KDCallback **callbacks = kdThreadSelf()->callbacks;
    for(KDint i = 0; i < callbackindex; i++)
    {
        KDboolean typematch = callbacks[i]->eventtype == eventtype || callbacks[i]->eventtype == 0;
        KDboolean userptrmatch = callbacks[i]->eventuserptr == eventuserptr;
        if(typematch && userptrmatch)
        {
            callbacks[i]->func = func;
            return 0;
        }
    }
    callbacks[callbackindex] = (__KDCallback *)kdMalloc(sizeof(__KDCallback));
    if(callbacks[callbackindex] == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return -1;
    }
    callbacks[callbackindex]->func = func;
    callbacks[callbackindex]->eventtype = eventtype;
    callbacks[callbackindex]->eventuserptr = eventuserptr;
    kdThreadSelf()->callbackindex++;
    return 0;
}

/* kdCreateEvent: Create an event for posting. */
KD_API KDEvent *KD_APIENTRY kdCreateEvent(void)
{
    KDEvent *event = (KDEvent *)kdMalloc(sizeof(KDEvent));
    if(event == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
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
    KDint error = __kdQueuePush(thread->eventqueue, (void *)event);
    if(error == -1)
    {
        kdFreeEvent(event);
        kdSetError(KD_ENOMEM);
        return -1;
    }
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
extern const char *__progname;
const char *__kdAppName(KD_UNUSED const char *argv0)
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

#if defined(__ANDROID__)
/* All Android events are send to the mainthread */
static KDThread *__kd_androidmainthread = KD_NULL;
static KDThreadMutex *__kd_androidmainthread_mutex = KD_NULL;
static ANativeActivity *__kd_androidactivity = KD_NULL;
static KDThreadMutex *__kd_androidactivity_mutex = KD_NULL;
static void __kd_AndroidOnDestroy(ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_CLOSE;
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

static void *__kd_AndroidOnSaveInstanceState(ANativeActivity *activity, size_t *len)
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

static ANativeWindow *__kd_androidwindow = KD_NULL;
static KDThreadMutex *__kd_androidwindow_mutex = KD_NULL;
static void __kd_AndroidOnNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = window;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
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

static KDThreadMutex *__kd_tls_mutex = KD_NULL;
static void __kdCleanupThreadStorageKHR(void);
static int __kdPreMain(int argc, char **argv)
{
#if defined(_WIN32)
    if(WSAStartup(0x202, (WSADATA[]){{0}}) != 0)
    {
        kdLogMessage("Winsock2 error.\n");
        kdExit(-1);
    }
#endif

    __kd_userptrmtx = kdThreadMutexCreate(KD_NULL);
    __kd_tls_mutex = kdThreadMutexCreate(KD_NULL);

    KDThread *thread = KD_NULL;
#if defined(__ANDROID__)
    kdThreadMutexLock(__kd_androidmainthread_mutex);
    thread = __kd_androidmainthread;
    kdThreadMutexUnlock(__kd_androidmainthread_mutex);
#else
    thread = __kdThreadInit();
#endif
    kdThreadOnce(&__kd_threadlocal_once, __kdThreadInitOnce);
    kdSetThreadStorageKHR(__kd_threadlocal, thread);

    KDint result = 0;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && !defined(__MINGW64__))
    result = kdMain(argc, (const KDchar *const *)argv);
#if defined(__ANDROID__)
    kdThreadMutexFree(__kd_androidactivity_mutex);
    kdThreadMutexFree(__kd_androidinputqueue_mutex);
    kdThreadMutexFree(__kd_androidwindow_mutex);
    kdThreadMutexFree(__kd_androidmainthread_mutex);
#endif
#else
    typedef KDint(KD_APIENTRY * KDMAIN)(KDint, const KDchar *const *);
    KDMAIN kdmain = KD_NULL;
#if defined(_WIN32)
    HMODULE handle = GetModuleHandle(0);
    kdmain = (KDMAIN)GetProcAddress(handle, "kdMain");
    if(kdmain == KD_NULL)
    {
        kdLogMessage("Unable to locate kdMain.\n");
        kdExit(-1);
    }
    result = kdmain(argc, (const KDchar *const *)argv);
#else
    void *app = dlopen(NULL, RTLD_NOW);
    void *rawptr = dlsym(app, "kdMain");
    if(dlerror())
    {
        kdLogMessage("Unable to locate kdMain.\n");
        kdExit(-1);
    }
    /* ISO C forbids assignment between function pointer and ‘void *’ */
    kdMemcpy(&kdmain, &rawptr, sizeof(rawptr));
    result = kdmain(argc, (const KDchar *const *)argv);
    dlclose(app);
#endif
#endif

    __kdCleanupThreadStorageKHR();
#if !defined(__ANDROID__)
    __kdThreadFree(thread);
#endif
    kdThreadMutexFree(__kd_tls_mutex);
    kdThreadMutexFree(__kd_userptrmtx);

#if defined(_WIN32)
    WSACleanup();
#endif
    return result;
}

#ifdef __ANDROID__
static void *__kdAndroidPreMain(void *arg)
{
    return (void *)__kdPreMain(0, KD_NULL);
}
void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
    __kd_androidmainthread_mutex = kdThreadMutexCreate(KD_NULL);
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

    kdThreadMutexLock(__kd_androidmainthread_mutex);
    __kd_androidmainthread = kdThreadCreate(KD_NULL, __kdAndroidPreMain, KD_NULL);
    kdThreadDetach(__kd_androidmainthread);
    kdThreadMutexUnlock(__kd_androidmainthread_mutex);
}
#endif


#if defined(_WIN32)
int WINAPI WinMain(KD_UNUSED HINSTANCE hInstance, KD_UNUSED HINSTANCE hPrevInstance, KD_UNUSED LPSTR lpCmdLine, KD_UNUSED int nShowCmd)
{
    return __kdPreMain(__argc, __argv);
}
#ifdef KD_FREESTANDING
int WINAPI WinMainCRTStartup(void)
{
    return __kdPreMain(__argc, __argv);
}
int WINAPI mainCRTStartup(void)
{
    return __kdPreMain(__argc, __argv);
}
#endif
#endif
KD_API int main(int argc, char **argv)
{
    KDint result = __kdPreMain(argc, argv);
    return result;
}

/* kdExit: Exit the application. */
KD_API KD_NORETURN void KD_APIENTRY kdExit(KDint status)
{
    if(status == 0)
    {
        status = EXIT_SUCCESS;
    }
    else
    {
        status = EXIT_FAILURE;
    }

#if defined(__EMSCRIPTEN__)
    emscripten_force_exit(status);
#endif

#if defined(_WIN32)
    ExitProcess(status);
    while(1)
    {
        ;
    }
#else
    exit(status);
#endif
}

/******************************************************************************
 * Locale specific functions
 ******************************************************************************/

/* kdGetLocale: Determine the current language and locale. */
KD_API const KDchar *KD_APIENTRY kdGetLocale(void)
{
    /* TODO: Add ISO 3166-1 part.*/
    static KDchar localestore[5] = "";
    kdMemset(&localestore, 0, sizeof(localestore));
#if defined(_WIN32)
    KDint localesize = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, KD_NULL, 0);
    KDchar *locale = (KDchar *)kdMalloc(localesize);
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, locale, localesize);
    kdMemcpy(localestore, locale, 2);
    kdFree(locale);
#else
    setlocale(LC_ALL, "");
    KDchar *locale = setlocale(LC_CTYPE, KD_NULL);
    if(locale == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
    }
    else if(kdStrcmp(locale, "C") == 0)
    {
        /* No locale support (musl, emscripten) */
        locale = "en";
    }
    kdMemcpy(localestore, locale, 2 /* 5 */);
#endif
    return (const KDchar *)localestore;
}

/******************************************************************************
 * Memory allocation
 ******************************************************************************/

/* kdMalloc: Allocate memory. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdMalloc(KDsize size)
{
    void *result = KD_NULL;
#if defined(_WIN32)
    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#else
    result = malloc(size);
#endif
    if(result == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    return result;
}

/* kdFree: Free allocated memory block. */
KD_API void KD_APIENTRY kdFree(void *ptr)
{
    if(ptr)
    {
#if defined(_WIN32)
        HeapFree(GetProcessHeap(), 0, ptr);
#else
        free(ptr);
#endif
    }
}

/* kdRealloc: Resize memory block. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdRealloc(void *ptr, KDsize size)
{
    void *result = KD_NULL;
#if defined(_WIN32)
    result = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, size);
#else
    result = realloc(ptr, size);
#endif
    if(result == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    return result;
}

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
typedef struct __KDThreadStorage __KDThreadStorage;
struct __KDThreadStorage {
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

static __KDThreadStorage __kd_tls[999];
static KDuint __kd_tls_index = 0;
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
    __kd_tls[__kd_tls_index].id = (void *)id;
#if defined(KD_THREAD_C11)
    if(tss_create(&__kd_tls[__kd_tls_index].nativekey, KD_NULL) != 0)
#elif defined(KD_THREAD_POSIX)
    if(pthread_key_create(&__kd_tls[__kd_tls_index].nativekey, KD_NULL) != 0)
#elif defined(KD_THREAD_WIN32)
    __kd_tls[__kd_tls_index].nativekey = FlsAlloc(NULL);
    if(__kd_tls[__kd_tls_index].nativekey == TLS_OUT_OF_INDEXES)
#else
    if(0)
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

static void __kdCleanupThreadStorageKHR(void)
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

/******************************************************************************
 * String and memory functions
 *
 * Notes:
 * - Based on the BSD libc developed at the University of California, Berkeley
 * - kdStrcpy_s/kdStrncat_s based on work by Todd C. Miller
 * - kdMemcmp/kdStrchr/kdStrcmp/kdStrlen (SSE4) based on work by Wojciech Muła
 * - kdMemchr/kdStrlen (SSE2) based on work by Mitsunari Shigeo
 * - kdMemchr/kdStrlen (NEON) based on work by Masaki Ota
 ******************************************************************************/
/******************************************************************************
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ******************************************************************************/
/******************************************************************************
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
 ******************************************************************************/
/******************************************************************************
 * Copyright (c) 2006-2015, Wojciech Muła
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
/******************************************************************************
 * Copyright (C) 2008 MITSUNARI Shigeo at Cybozu Labs, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organization nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
/******************************************************************************
 * Copyright (C) 2016 Masaki Ota
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organization nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

static KDuint32 __kdBitScanForward(KDuint32 x)
{
#if defined(__BMI__)
    return _tzcnt_u32(x);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    return _BitScanForward((unsigned long *)&x, (unsigned long)x);
#else
    static const KDchar ctz_lookup[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
    KDsize s = 0;
    do
    {
        KDuint32 r = (x >> s) & 0xff;
        if(r != 0)
        {
            return ctz_lookup[r] + (KDchar)s;
        }
    } while((s += 8) < (sizeof(KDuint32) * 8));
    return sizeof(KDuint32) - 1;
#endif
}

/* kdMemchr: Scan memory for a byte value. */
KD_API void *KD_APIENTRY kdMemchr(const void *src, KDint byte, KDsize len)
{
#if defined(__SSE2__) || defined(__ARM_NEON__)
    const KDchar *p = (const KDchar *)src;
    if(len >= 16)
    {
#if defined(__SSE2__)
        __m128i c16 = _mm_set1_epi8((KDchar)byte);
#elif defined(__ARM_NEON__)
        uint8x16_t c16 = vdupq_n_u8((KDchar)byte);
        static const uint8x16_t compaction_mask = {
            1, 2, 4, 8, 16, 32, 64, 128,
            1, 2, 4, 8, 16, 32, 64, 128};
#endif
        /* 16 byte alignment */
        KDuintptr ip = (KDuintptr)p;
        KDuintptr n = ip & 15;
        if(n > 0)
        {
            ip &= ~15;
            KDuint32 mask = 0;
#if defined(__SSE2__)
            __m128i x = *(const __m128i *)ip;
            __m128i a = _mm_cmpeq_epi8(x, c16);
            mask = _mm_movemask_epi8(a);
            mask &= 0xffffffffUL << n;
#elif defined(__ARM_NEON__)
            uint8x16_t x = *(const uint8x16_t *)ip;
            uint8x16_t a = vceqq_u8(x, c16);
            uint8x16_t am = vandq_u8(a, compaction_mask);
            uint8x8_t a_sum = vpadd_u8(vget_low_u8(am), vget_high_u8(am));
            a_sum = vpadd_u8(a_sum, a_sum);
            a_sum = vpadd_u8(a_sum, a_sum);
            mask = vget_lane_u16(vreinterpret_u16_u8(a_sum), 0);
            mask = mask >> n;
#endif
            if(mask)
            {
                return (void *)(ip + __kdBitScanForward(mask));
            }
            n = 16 - n;
            len -= n;
            p += n;
        }
        while(len >= 32)
        {
            KDuint32 mask = 0;
#if defined(__SSE2__)
            __m128i x = *(const __m128i *)&p[0];
            __m128i y = *(const __m128i *)&p[16];
            __m128i a = _mm_cmpeq_epi8(x, c16);
            __m128i b = _mm_cmpeq_epi8(y, c16);
            mask = (_mm_movemask_epi8(b) << 16) | _mm_movemask_epi8(a);
#elif defined(__ARM_NEON__)
            uint8x16_t x = *(const uint8x16_t *)&p[0];
            uint8x16_t y = *(const uint8x16_t *)&p[16];
            uint8x16_t a = vceqq_u8(x, c16);
            uint8x16_t b = vceqq_u8(y, c16);
            uint8x8_t xx = vorr_u8(vget_low_u8(vorrq_u8(a, b)), vget_high_u8(vorrq_u8(a, b)));
            if(vget_lane_u64(vreinterpret_u64_u8(xx), 0))
            {
                uint8x16_t am = vandq_u8(a, compaction_mask);
                uint8x16_t bm = vandq_u8(b, compaction_mask);
                uint8x8_t a_sum = vpadd_u8(vget_low_u8(am), vget_high_u8(am));
                uint8x8_t b_sum = vpadd_u8(vget_low_u8(bm), vget_high_u8(bm));
                a_sum = vpadd_u8(a_sum, b_sum);
                a_sum = vpadd_u8(a_sum, a_sum);
                mask = vget_lane_u32(vreinterpret_u32_u8(a_sum), 0);
            }
#endif
            if(mask)
            {
                return (void *)(p + __kdBitScanForward(mask));
            }
            len -= 32;
            p += 32;
        }
    }
    while(len > 0)
    {
        if(*p == byte)
        {
            return (void *)p;
        }
        p++;
        len--;
    }
#else
    if(len != 0)
    {
        const KDuint8 *p = src;
        do
        {
            if(*p++ == (KDuint8)byte)
            {
                return ((void *)(p - 1));
            }
        } while(--len != 0);
    }
#endif
    return KD_NULL;
}

/* kdMemcmp: Compare two memory regions. */
KD_API KDint KD_APIENTRY kdMemcmp(const void *src1, const void *src2, KDsize len)
{
    if(len == 0 || (src1 == src2))
    {
        return 0;
    }
#if defined(__SSE4_2__)
    __m128i *ptr1 = (__m128i *)src1;
    __m128i *ptr2 = (__m128i *)src2;
    enum { mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_LEAST_SIGNIFICANT };

    for(; len != 0; ptr1++, ptr2++)
    {
        const __m128i a = _mm_loadu_si128(ptr1);
        const __m128i b = _mm_loadu_si128(ptr2);

        if(_mm_cmpestrc(a, len, b, len, mode))
        {
            const KDint idx = _mm_cmpestri(a, len, b, len, mode);
            const KDuint8 b1 = ((KDchar *)ptr1)[idx];
            const KDuint8 b2 = ((KDchar *)ptr2)[idx];

            if(b1 < b2)
            {
                return -1;
            }
            else if(b1 > b2)
            {
                return +1;
            }
            else
            {
                return 0;
            }
        }

        if(len > 16)
        {
            len -= 16;
        }
        else
        {
            len = 0;
        }
    }
#else
    const KDuint8 *p1 = src1, *p2 = src2;
    do
    {
        if(*p1++ != *p2++)
        {
            return (*--p1 - *--p2);
        }
    } while(--len != 0);
#endif
    return 0;
}

/* kdMemcpy: Copy a memory region, no overlapping. */
KD_API void *KD_APIENTRY kdMemcpy(void *buf, const void *src, KDsize len)
{
    KDint8 *_buf = buf;
    const KDint8 *_src = src;
    while(len--)
    {
        *_buf++ = *_src++;
    }
    return buf;
}

/* kdMemmove: Copy a memory region, overlapping allowed. */
KD_API void *KD_APIENTRY kdMemmove(void *buf, const void *src, KDsize len)
{
    KDint8 *_buf = buf;
    const KDint8 *_src = src;

    if(!len)
    {
        return buf;
    }

    if(buf <= src)
    {
        return kdMemcpy(buf, src, len);
    }

    _src += len;
    _buf += len;

    while(len--)
    {
        *--_buf = *--_src;
    }

    return buf;
}

/* kdMemset: Set bytes in memory to a value. */
KD_API void *KD_APIENTRY kdMemset(void *buf, KDint byte, KDsize len)
{
    KDuint8 *p = (KDuint8 *)buf;
    while(len--)
    {
        *p++ = (KDuint8)byte;
    }
    return buf;
}

/* kdStrchr: Scan string for a byte value. */
KD_API KDchar *KD_APIENTRY kdStrchr(const KDchar *str, KDint ch)
{
#if defined(__SSE4_2__)
    kdAssert(ch >= 0);
    kdAssert(ch < 256);

    __m128i *mem = (__m128i *)(KDchar *)str;
    const __m128i set = _mm_setr_epi8(ch, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    enum { mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT };

    for(;; mem++)
    {
        const __m128i chunk = _mm_loadu_si128(mem);

        if(_mm_cmpistrc(set, chunk, mode))
        {
            /* there is character ch in a chunk */
            const KDint idx = _mm_cmpistri(set, chunk, mode);
            return (KDchar *)mem + idx;
        }
        else if(_mm_cmpistrz(set, chunk, mode))
        {
            /* there is zero byte in a chunk */
            break;
        }
    }
    return KD_NULL;
#else
    for(;; ++str)
    {
        if(*str == (KDchar)ch)
        {
            return ((KDchar *)str);
        }
        if(*str == '\0')
        {
            return KD_NULL;
        }
    }
#endif
}

/* kdStrcmp: Compares two strings. */
KD_API KDint KD_APIENTRY kdStrcmp(const KDchar *str1, const KDchar *str2)
{
    if(str1 == str2)
    {
        return 0;
    }
#if defined(__SSE4_2__) && !defined(KD_ASAN)
    __m128i *ptr1 = (__m128i *)(KDchar *)str1;
    __m128i *ptr2 = (__m128i *)(KDchar *)str2;

    for(;; ptr1++, ptr2++)
    {
        const __m128i a = _mm_loadu_si128(ptr1);
        const __m128i b = _mm_loadu_si128(ptr2);

        enum { mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_LEAST_SIGNIFICANT };

        if(_mm_cmpistrc(a, b, mode))
        {
            /* a & b are different (not counting past-zero bytes) */
            const KDint idx = _mm_cmpistri(a, b, mode);

            const KDuint8 b1 = ((KDchar *)ptr1)[idx];
            const KDuint8 b2 = ((KDchar *)ptr2)[idx];

            if(b1 < b2)
            {
                return -1;
            }
            else if(b1 > b2)
            {
                return +1;
            }
            else
            {
                return 0;
            }
        }
        else if(_mm_cmpistrz(a, b, mode))
        {
            /* a & b are same, but b contains a zero byte */
            break;
        }
    }

    return 0;
#else
    while(*str1 == *str2++)
    {
        if(*str1++ == '\0')
        {
            return 0;
        }
    }
    return *str1 - *(str2 - 1);
#endif
}

/* kdStrlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrlen(const KDchar *str)
{
    const KDchar *s = str;
#if defined(__SSE4_2__) && !defined(KD_ASAN)
    KDsize result = 0;
    const __m128i zeros = _mm_setzero_si128();
    __m128i *mem = (__m128i *)(KDchar *)s;

    for(;; mem++, result += 16)
    {
        const __m128i data = _mm_loadu_si128(mem);
        enum { mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_LEAST_SIGNIFICANT };

        /* Note: pcmpstri return mask/index and set ALU flags. Intrinsics
         *       functions can return just single value (mask, particular
         *       flag), so multiple intrinsics functions have to be called.
         *
         *       The good news: GCC and MSVC merges multiple _mm_cmpXstrX
         *       invocations with the same arguments to the single pcmpstri
         *       instruction. 
         */
        if(_mm_cmpistrc(data, zeros, mode))
        {
            const KDint idx = _mm_cmpistri(data, zeros, mode);
            return result + idx;
        }
    }
#elif defined(__SSE4_1__) && !defined(KD_ASAN)
    KDsize result = 0;
    const __m128i zeros = _mm_setzero_si128();
    __m128i *mem = (__m128i *)(KDchar *)s;

    for(;; mem++, result += 16)
    {
        const __m128i data = _mm_loadu_si128(mem);
        const __m128i cmp = _mm_cmpeq_epi8(data, zeros);

        if(!_mm_testc_si128(zeros, cmp))
        {
            const KDint mask = _mm_movemask_epi8(cmp);
            return result + __kdBitScanForward(mask);
        }
    }

    kdAssert(0);
    return 0;
#elif (defined(__SSE2__) && !defined(KD_ASAN)) || defined(__ARM_NEON__)
#if defined(__SSE2__)
    __m128i c16 = _mm_set1_epi8(0);
#elif defined(__ARM_NEON__)
    uint8x16_t c16 = vdupq_n_u8(0);
    static const uint8x16_t compaction_mask = {
        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8, 16, 32, 64, 128};
#endif
    /* 16 byte alignment */
    KDuintptr ip = (KDuintptr)s;
    KDuintptr n = ip & 15;
    if(n > 0)
    {
        ip &= ~15;
        KDuint32 mask = 0;
#if defined(__SSE2__)
        __m128i x = *(const __m128i *)ip;
        __m128i a = _mm_cmpeq_epi8(x, c16);
        mask = _mm_movemask_epi8(a);
        mask &= 0xffffffffUL << n;
#elif defined(__ARM_NEON__)
        uint8x16_t x = *(const uint8x16_t *)ip;
        uint8x16_t a = vceqq_u8(x, c16);
        uint8x16_t am = vandq_u8(a, compaction_mask);
        uint8x8_t a_sum = vpadd_u8(vget_low_u8(am), vget_high_u8(am));
        a_sum = vpadd_u8(a_sum, a_sum);
        a_sum = vpadd_u8(a_sum, a_sum);
        mask = vget_lane_u16(vreinterpret_u16_u8(a_sum), 0);
        mask = mask >> n;
#endif
        if(mask)
        {
            return __kdBitScanForward(mask) - n;
        }
        s += 16 - n;
    }
    kdAssert(((KDuintptr)s & 15) == 0);
    if((KDuintptr)s & 31)
    {
        KDuint32 mask = 0;
#if defined(__SSE2__)
        __m128i x = *(const __m128i *)&s[0];
        __m128i a = _mm_cmpeq_epi8(x, c16);
        mask = _mm_movemask_epi8(a);
#elif defined(__ARM_NEON__)
        uint8x16_t x = *(const uint8x16_t *)&s[0];
        uint8x16_t a = vceqq_u8(x, c16);
        uint8x8_t xx = vorr_u8(vget_low_u8(x), vget_high_u8(x));
        if(vget_lane_u64(vreinterpret_u64_u8(xx), 0))
        {
            uint8x16_t am = vandq_u8(a, compaction_mask);
            uint8x8_t a_sum = vpadd_u8(vget_low_u8(am), vget_high_u8(am));
            a_sum = vpadd_u8(a_sum, a_sum);
            a_sum = vpadd_u8(a_sum, a_sum);
            mask = vget_lane_u16(vreinterpret_u16_u8(a_sum), 0);
        }
#endif
        if(mask)
        {
            return s + __kdBitScanForward(mask) - str;
        }
        s += 16;
    }
    kdAssert(((KDuintptr)s & 31) == 0);
    for(;;)
    {
        KDuint32 mask = 0;
#if defined(__SSE2__)
        __m128i x = *(const __m128i *)&s[0];
        __m128i y = *(const __m128i *)&s[16];
        __m128i a = _mm_cmpeq_epi8(x, c16);
        __m128i b = _mm_cmpeq_epi8(y, c16);
        mask = (_mm_movemask_epi8(b) << 16) | _mm_movemask_epi8(a);
#elif defined(__ARM_NEON__)
        uint8x16_t x = *(const uint8x16_t *)&s[0];
        uint8x16_t y = *(const uint8x16_t *)&s[16];
        uint8x16_t a = vceqq_u8(x, c16);
        uint8x16_t b = vceqq_u8(y, c16);
        uint8x8_t xx = vorr_u8(vget_low_u8(vorrq_u8(a, b)), vget_high_u8(vorrq_u8(a, b)));
        if(vget_lane_u64(vreinterpret_u64_u8(xx), 0))
        {
            uint8x16_t am = vandq_u8(a, compaction_mask);
            uint8x16_t bm = vandq_u8(b, compaction_mask);
            uint8x8_t a_sum = vpadd_u8(vget_low_u8(am), vget_high_u8(am));
            uint8x8_t b_sum = vpadd_u8(vget_low_u8(bm), vget_high_u8(bm));
            a_sum = vpadd_u8(a_sum, b_sum);
            a_sum = vpadd_u8(a_sum, a_sum);
            mask = vget_lane_u32(vreinterpret_u32_u8(a_sum), 0);
        }
#endif
        if(mask)
        {
            return s + __kdBitScanForward(mask) - str;
        }
        s += 32;
    }
#else
    for(; *s; ++s)
    {
        ;
    }
    return (s - str);
#endif
}

/* kdStrnlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrnlen(const KDchar *str, KDsize maxlen)
{
    KDsize len = 0;
    for(; len < maxlen; len++, str++)
    {
        if(!*str)
        {
            break;
        }
    }
    return len;
}

/* kdStrncat_s: Concatenate two strings. */
KD_API KDint KD_APIENTRY kdStrncat_s(KDchar *buf, KDsize buflen, const KDchar *src, KD_UNUSED KDsize srcmaxlen)
{
    KDchar *d = buf;
    const KDchar *s = src;
    KDsize n = buflen;
    KDsize dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while(n-- != 0 && *d != '\0')
    {
        d++;
    }
    dlen = d - buf;
    n = buflen - dlen;

    if(n == 0)
    {
        return (KDint)(dlen + kdStrlen(s));
    }
    while(*s != '\0')
    {
        if(n != 1)
        {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (KDint)(dlen + (s - src)); /* count does not include NUL */
}

/* kdStrncmp: Compares two strings with length limit. */
KD_API KDint KD_APIENTRY kdStrncmp(const KDchar *str1, const KDchar *str2, KDsize maxlen)
{
    if(maxlen == 0)
    {
        return 0;
    }
    do
    {
        if(*str1 != *str2++)
        {
            return (*(const KDuint8 *)str1 - *(const KDuint8 *)(str2 - 1));
        }
        if(*str1++ == '\0')
        {
            break;
        }
    } while(--maxlen != 0);
    return 0;
}


/* kdStrcpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrcpy_s(KDchar *buf, KDsize buflen, const KDchar *src)
{
    KDchar *d = buf;
    const KDchar *s = src;
    KDsize n = buflen;

    /* Copy as many bytes as will fit */
    if(n != 0)
    {
        while(--n != 0)
        {
            if((*d++ = *s++) == '\0')
            {
                break;
            }
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if(n == 0)
    {
        if(buflen != 0)
        {
            *d = '\0';
        } /* NUL-terminate dst */
        while(*s++)
        {
            ;
        }
    }

    return (KDint)(s - src - 1); /* count does not include NUL */
}

/* kdStrncpy_s: Copy a string with an overrun check. */
KD_API KDint KD_APIENTRY kdStrncpy_s(KDchar *buf, KDsize buflen, const KDchar *src, KD_UNUSED KDsize srclen)
{
    if(buflen != 0)
    {
        KDchar *d = buf;
        const KDchar *s = src;

        do
        {
            if((*d++ = *s++) == '\0')
            {
                /* NUL pad the remaining n-1 bytes */
                while(--buflen != 0)
                {
                    *d++ = '\0';
                }
                break;
            }
        } while(--buflen != 0);
    }
    else
    {
        return -1;
    }
    return 0;
}

/* kdStrstrVEN: Locate substring. */
KD_API KDchar *KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2)
{
    KDchar c, sc;
    if((c = *str2++) != '\0')
    {
        KDsize len = kdStrlen(str2);
        do
        {
            do
            {
                if((sc = *str1++) == '\0')
                {
                    return (NULL);
                }
            } while(sc != c);
        } while(kdStrncmp(str1, str2, len) != 0);
        str1--;
    }
    return (KDchar *)str1;
}

/* kdStrcspnVEN:  Get span until character in string. */
KD_API KDsize KD_APIENTRY kdStrcspnVEN(const KDchar *str1, const KDchar *str2)
{
    KDsize retval = 0;
    while(*str1)
    {
        if(kdStrchr(str2,*str1))
        {
            return retval;
        }
        else
        {
            str1++;
            retval++;
        }
    }
    return retval;
}


/******************************************************************************
 * Time functions
 ******************************************************************************/

/* kdGetTimeUST: Get the current unadjusted system time. */
KD_API KDust KD_APIENTRY kdGetTimeUST(void)
{
#if defined(_WIN32)
    LARGE_INTEGER tickspersecond;
    LARGE_INTEGER tick;
    QueryPerformanceFrequency(&tickspersecond);
    QueryPerformanceCounter(&tick);
    return (tick.QuadPart * 1000000000LL) / tickspersecond.QuadPart;
#elif defined(__linux__)
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
#elif defined(__MAC_10_12) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12 && __apple_build_version__ >= 800038
    /* Supported as of XCode 8 / macOS Sierra 10.12 */
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
#elif defined(EGL_NV_system_time) && !defined(__APPLE__)
    if(kdStrstrVEN(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS), "EGL_NV_system_time"))
    {
        PFNEGLGETSYSTEMTIMENVPROC eglGetSystemTimeNV = (PFNEGLGETSYSTEMTIMENVPROC)eglGetProcAddress("eglGetSystemTimeNV");
        PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC eglGetSystemTimeFrequencyNV = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)eglGetProcAddress("eglGetSystemTimeFrequencyNV");
        if(eglGetSystemTimeNV && eglGetSystemTimeFrequencyNV)
        {
            return (eglGetSystemTimeNV() * 1000000000LL) / eglGetSystemTimeFrequencyNV();
        }
    }
#elif defined(__EMSCRIPTEN__)
    return emscripten_get_now() * 1000000LL;
#else
    return (clock() * 1000000000LL) / CLOCKS_PER_SEC;
#endif
}

/* kdTime: Get the current wall clock time. */
KD_API KDtime KD_APIENTRY kdTime(KDtime *timep)
{
#if defined(_WIN32)
    FILETIME filetime;
    ULARGE_INTEGER largeuint;
    GetSystemTimeAsFileTime(&filetime);
    largeuint.LowPart = filetime.dwLowDateTime;
    largeuint.HighPart = filetime.dwHighDateTime;
    /* See RtlTimeToSecondsSince1970 */
    KDtime time = (KDtime)((largeuint.QuadPart / 10000000LL) - 11644473600LL);
    (*timep) = time;
    return time;
#else
    return time((time_t *)timep);
#endif
}

/* kdGmtime_r, kdLocaltime_r: Convert a seconds-since-epoch time into broken-down time. */
static KDboolean __kdIsleap(KDint32 year)
{
    return (!((year) % 4) && (((year) % 100) || !((year) % 400)));
}
KD_API KDTm *KD_APIENTRY kdGmtime_r(const KDtime *timep, KDTm *result)
{
    KDint32 secs_per_day = 3600 * 24;
    KDint32 days_in_secs = (KDint32)(*timep % secs_per_day);
    KDint32 days = (KDint32)(*timep / secs_per_day);
    result->tm_sec = days_in_secs % 60;
    result->tm_min = (days_in_secs % 3600) / 60;
    result->tm_hour = days_in_secs / 3600;
    result->tm_wday = (days + 4) % 7;

    KDint32 year = 1970;
    while(days >= (__kdIsleap(year) ? 366 : 365))
    {
        days -= (__kdIsleap(year) ? 366 : 365);
        year++;
    }
    result->tm_year = year - 1900;
    result->tm_yday = days;
    result->tm_mon = 0;

    const KDint months[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

    while(days >= months[__kdIsleap(year)][result->tm_mon])
    {
        days -= months[__kdIsleap(year)][result->tm_mon];
        result->tm_mon++;
    }
    result->tm_mday = days + 1;
    result->tm_isdst = 0;

    return result;
}
KD_API KDTm *KD_APIENTRY kdLocaltime_r(const KDtime *timep, KDTm *result)
{
    /* No timezone support */
    return kdGmtime_r(timep, result);
}

/* kdUSTAtEpoch: Get the UST corresponding to KDtime 0. */
KD_API KDust KD_APIENTRY kdUSTAtEpoch(void)
{
    /* Implement */
    kdAssert(0);
    return 0;
}

/******************************************************************************
 * Timer functions
 ******************************************************************************/

/* kdSetTimer: Set timer. */
typedef struct {
    KDint64 interval;
    KDint periodic;
    void *eventuserptr;
    KDThread *destination;
} __KDTimerPayload;
static void *__kdTimerHandler(void *arg)
{
    __KDTimerPayload *payload = (__KDTimerPayload *)arg;
    for(;;)
    {
        kdThreadSleepVEN(payload->interval);

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
struct KDTimer {
    KDThread *thread;
    KDThread *originthr;
    __KDTimerPayload *payload;
};
KD_API KDTimer *KD_APIENTRY kdSetTimer(KDint64 interval, KDint periodic, void *eventuserptr)
{
    if(periodic != KD_TIMER_ONESHOT && periodic != KD_TIMER_PERIODIC_AVERAGE && periodic != KD_TIMER_PERIODIC_MINIMUM)
    {
        kdLogMessage("kdSetTimer() encountered unknown periodic value.\n");
        return KD_NULL;
    }

    __KDTimerPayload *payload = (__KDTimerPayload *)kdMalloc(sizeof(__KDTimerPayload));
    if(payload == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    payload->interval = interval;
    payload->periodic = periodic;
    payload->eventuserptr = eventuserptr;
    payload->destination = kdThreadSelf();

    KDTimer *timer = (KDTimer *)kdMalloc(sizeof(KDTimer));
    if(timer == KD_NULL)
    {
        kdFree(payload);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    timer->thread = kdThreadCreate(KD_NULL, __kdTimerHandler, payload);
    if(timer->thread == KD_NULL)
    {
        kdFree(timer);
        kdFree(payload);
        if(kdGetError() == KD_ENOSYS)
        {
            kdLogMessage("kdSetTimer() needs a threading implementation.\n");
            return KD_NULL;
        }
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    timer->originthr = kdThreadSelf();
    timer->payload = payload;
    return timer;
}

/* kdCancelTimer: Cancel and free a timer. */
KD_API KDint KD_APIENTRY kdCancelTimer(KDTimer *timer)
{
    if(timer->originthr != kdThreadSelf())
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
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
    KDFile *file = (KDFile *)kdMalloc(sizeof(KDFile));
    if(file == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    file->pathname = pathname;
    KDint error = 0;
#if defined(_WIN32)
    DWORD access = 0;
    DWORD create = 0;
    switch(mode[0])
    {
        case 'w':
        {
            access = GENERIC_WRITE;
            create = CREATE_ALWAYS;
            break;
        }
        case 'r':
        {
            access = GENERIC_READ;
            create = OPEN_EXISTING;
            break;
        }
        case 'a':
        {
            access = GENERIC_READ | GENERIC_WRITE;
            create = OPEN_ALWAYS;
            break;
        }
        default:
        {
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
    }
    if(mode[1] == '+' || mode[2] == '+')
    {
        access = GENERIC_READ | GENERIC_WRITE;
    }
    file->nativefile = CreateFileA(pathname, access, FILE_SHARE_READ | FILE_SHARE_WRITE, KD_NULL, create, 0, KD_NULL);
    if(file->nativefile != INVALID_HANDLE_VALUE)
    {
        if(mode[0] == 'a')
        {
            SetFilePointer(file->nativefile, 0, KD_NULL, FILE_END);
        }
    }
    else
    {
        error = GetLastError();
#else
    KDint access = 0;
    mode_t create = 0;
    switch(mode[0])
    {
        case 'w':
        {
            access = O_WRONLY | O_CREAT;
            create = S_IRUSR | S_IWUSR;
            break;
        }
        case 'r':
        {
            access = O_RDONLY;
            create = 0;
            break;
        }
        case 'a':
        {
            access = O_WRONLY | O_CREAT | O_APPEND;
            create = 0;
            break;
        }
        default:
        {
            kdFree(file);
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
    }
    if(mode[1] == '+' || mode[2] == '+')
    {
        access = O_RDWR | O_CREAT;
        create = S_IRUSR | S_IWUSR;
    }
    file->nativefile = open(pathname, access | O_CLOEXEC, create);
    if(file->nativefile == -1)
    {
        error = errno;
#endif
        kdFree(file);
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EISDIR | KD_EMFILE | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        return KD_NULL;
    }
    file->eof = KD_FALSE;
    return file;
}

/* kdFclose: Close an open file. */
KD_API KDint KD_APIENTRY kdFclose(KDFile *file)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    retval = CloseHandle(file->nativefile);
    if(retval == 0)
    {
        error = GetLastError();
#else
    retval = close(file->nativefile);
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        kdFree(file);
        return KD_EOF;
    }
    kdFree(file);
    return 0;
}

/* kdFflush: Flush an open file. */
KD_API KDint KD_APIENTRY kdFflush(KD_UNUSED KDFile *file)
{
#if !defined(_WIN32)
    KDint retval = fsync(file->nativefile);
    if(retval == -1)
    {
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(errno, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
#endif
    return 0;
}

/* kdFread: Read from a file. */
KD_API KDsize KD_APIENTRY kdFread(void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDssize retval = 0;
    KDint error = 0;
    KDsize length = count * size;
#if defined(_WIN32)
    BOOL success = ReadFile(file->nativefile, buffer, (DWORD)length, (LPDWORD)&retval, KD_NULL);
    if(success == TRUE && retval == 0)
    {
        file->eof = KD_TRUE;
    }
    else if(success == FALSE)
    {
        error = GetLastError();
#else
    KDchar *temp = buffer;
    while(length != 0 && (retval = read(file->nativefile, temp, length)) != 0)
    {
        if(retval == -1)
        {
            if(errno != EINTR)
            {
                break;
            }
        }
        length -= retval;
        temp += retval;
    }
    length = count * size;
    if(retval == 0)
    {
        file->eof = KD_TRUE;
    }
    else if((KDsize)retval != length)
    {
        error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
    }
    return (KDsize)retval;
}

/* kdFwrite: Write to a file. */
KD_API KDsize KD_APIENTRY kdFwrite(const void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDssize retval = 0;
    KDint error = 0;
    KDsize length = count * size;
#if defined(_WIN32)
    BOOL success = WriteFile(file->nativefile, buffer, (DWORD)length, (LPDWORD)&retval, KD_NULL);
    if(success == FALSE)
    {
        error = GetLastError();
#else
    KDchar *temp = kdMalloc(length);
    KDchar *_temp = temp;
    kdMemcpy(temp, buffer, length);
    while(length != 0 && (retval = write(file->nativefile, temp, length)) != 0)
    {
        if(retval == -1)
        {
            if(errno != EINTR)
            {
                break;
            }
        }
        length -= retval;
        temp += retval;
    }
    kdFree(_temp);
    length = count * size;
    if((KDsize)retval != length)
    {
        error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EBADF | KD_EFBIG | KD_ENOMEM | KD_ENOSPC);
    }
    return (KDsize)retval;
}

/* kdGetc: Read next byte from an open file. */
KD_API KDint KD_APIENTRY kdGetc(KDFile *file)
{
    KDuint8 byte = 0;
    KDint error = 0;
#if defined(_WIN32)
    DWORD byteswritten = 0;
    BOOL success = ReadFile(file->nativefile, &byte, 1, &byteswritten, KD_NULL);
    if(success == TRUE && byteswritten == 0)
    {
        file->eof = KD_TRUE;
        return KD_EOF;
    }
    else if(success == FALSE)
    {
        error = GetLastError();
#else
    KDint success = (KDsize)read(file->nativefile, &byte, 1);
    if(success == 0)
    {
        file->eof = KD_TRUE;
        return KD_EOF;
    }
    else if(success == -1)
    {
        error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
    return (KDint)byte;
}

/* kdPutc: Write a byte to an open file. */
KD_API KDint KD_APIENTRY kdPutc(KDint c, KDFile *file)
{
    KDint error = 0;
    KDuint8 byte = c & 0xFF;
#if defined(_WIN32)
    BOOL success = WriteFile(file->nativefile, &byte, 1, (DWORD[]){0}, KD_NULL);
    if(success == FALSE)
    {
        error = GetLastError();
#else
    KDint success = (KDsize)write(file->nativefile, &byte, 1);
    if(success == -1)
    {
        error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EBADF | KD_EFBIG | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
    return (KDint)byte;
}

/* kdFgets: Read a line of text from an open file. */
KD_API KDchar *KD_APIENTRY kdFgets(KDchar *buffer, KDsize buflen, KDFile *file)
{
    KDchar *line = buffer;
    for(KDsize i = buflen; i > 1; --i)
    {
        KDint character = kdGetc(file);
        if(character == KD_EOF)
        {
            if(i == buflen - 1)
            {
                return KD_NULL;
            }
            break;
        }
        *line++ = (KDchar)character;
        if(character == '\n')
        {
            break;
        }
    }
    return line;
}

/* kdFEOF: Check for end of file. */
KD_API KDint KD_APIENTRY kdFEOF(KDFile *file)
{
    if(file->eof == KD_TRUE)
    {
        return KD_EOF;
    }
    return 0;
}

/* kdFerror: Check for an error condition on an open file. */
KD_API KDint KD_APIENTRY kdFerror(KDFile *file)
{
    if(file->error == KD_TRUE)
    {
        return KD_EOF;
    }
    return 0;
}

/* kdClearerr: Clear a file's error and end-of-file indicators. */
KD_API void KD_APIENTRY kdClearerr(KDFile *file)
{
    file->error = KD_FALSE;
    file->eof = KD_FALSE;
}

/* TODO: Cleanup */
typedef struct {
#if defined(_MSC_VER)
    KDint seekorigin_kd;
#else
    KDuint seekorigin_kd;
#endif
#if defined(_WIN32)
    DWORD seekorigin;
#else
    KDint seekorigin;
#endif
} __KDSeekOrigin;

#if defined(_WIN32)
static __KDSeekOrigin seekorigins[] = {{KD_SEEK_SET, FILE_BEGIN}, {KD_SEEK_CUR, FILE_CURRENT}, {KD_SEEK_END, FILE_END}};
#else
static __KDSeekOrigin seekorigins[] = {{KD_SEEK_SET, SEEK_SET}, {KD_SEEK_CUR, SEEK_CUR}, {KD_SEEK_END, SEEK_END}};
#endif

/* kdFseek: Reposition the file position indicator in a file. */
KD_API KDint KD_APIENTRY kdFseek(KDFile *file, KDoff offset, KDfileSeekOrigin origin)
{
    KDint error = 0;
    for(KDuint i = 0; i < sizeof(seekorigins) / sizeof(seekorigins[0]); i++)
    {
        if(seekorigins[i].seekorigin_kd == origin)
        {
#if defined(_WIN32)
            DWORD retval = SetFilePointer(file->nativefile, (LONG)offset, KD_NULL, seekorigins[i].seekorigin);
            if(retval == INVALID_SET_FILE_POINTER)
            {
                error = GetLastError();
#else
            KDint retval = lseek(file->nativefile, (KDint32)offset, seekorigins[i].seekorigin);
            if(retval != 0)
            {
                error = errno;
#endif
                kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EINVAL | KD_EIO | KD_ENOMEM | KD_ENOSPC | KD_EOVERFLOW);
                return -1;
            }
            break;
        }
    }
    return 0;
}

/* kdFtell: Get the file position of an open file. */
KD_API KDoff KD_APIENTRY kdFtell(KDFile *file)
{
    KDoff position = 0;
    KDint error = 0;
#if defined(_WIN32)
    position = (KDoff)SetFilePointer(file->nativefile, 0, KD_NULL, FILE_CURRENT);
    if(position == INVALID_SET_FILE_POINTER)
    {
        error = GetLastError();
#else
    position = lseek(file->nativefile, 0, SEEK_CUR);
    if(position == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EOVERFLOW);
        return -1;
    }
    return position;
}

/* kdMkdir: Create new directory. */
KD_API KDint KD_APIENTRY kdMkdir(const KDchar *pathname)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    retval = CreateDirectoryA(pathname, KD_NULL);
    if(retval == 0)
    {
        error = GetLastError();
#else
    retval = mkdir(pathname, 0777);
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EEXIST | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        return -1;
    }
    return 0;
}

/* kdRmdir: Delete a directory. */
KD_API KDint KD_APIENTRY kdRmdir(const KDchar *pathname)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    retval = RemoveDirectoryA(pathname);
    if(retval == 0)
    {
        error = GetLastError();
#else
    retval = rmdir(pathname);
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EEXIST | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdRename: Rename a file. */
KD_API KDint KD_APIENTRY kdRename(const KDchar *src, const KDchar *dest)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    retval = MoveFileA(src, dest);
    if(retval == 0)
    {
        error = GetLastError();
        if(error == ERROR_ALREADY_EXISTS || error == ERROR_SEEK)
        {
            kdSetError(KD_EINVAL);
        }
        else
#else
    retval = rename(src, dest);
    if(retval == -1)
    {
        error = errno;
        if(error == ENOTDIR)
        {
            kdSetError(KD_EINVAL);
        }
        else if(error == ENOTEMPTY)
        {
            kdSetError(KD_EACCES);
        }
        else
#endif
        {
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        }
        return -1;
    }
    return 0;
}

/* kdRemove: Delete a file. */
KD_API KDint KD_APIENTRY kdRemove(const KDchar *pathname)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    retval = DeleteFileA(pathname);
    if(retval == 0)
    {
        error = GetLastError();
#else
    retval = remove(pathname);
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdTruncate: Truncate or extend a file. */
KD_API KDint KD_APIENTRY kdTruncate(KD_UNUSED const KDchar *pathname, KD_UNUSED KDoff length)
{
    KDint retval = 0;
    KDint error = 0;
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    HANDLE file = FindFirstFileA(pathname, &data);
    retval = (KDint)SetFileValidData(file, (LONGLONG)length);
    FindClose(file);
    if(retval == 0)
    {
        error = GetLastError();
#else
    retval = truncate(pathname, length);
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdStat, kdFstat: Return information about a file. */
KD_API KDint KD_APIENTRY kdStat(const KDchar *pathname, struct KDStat *buf)
{
    KDint error = 0;
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    if(FindFirstFileA(pathname, &data) != INVALID_HANDLE_VALUE)
    {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            buf->st_mode = 0x4000;
        }
        else if(data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL || data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
        {
            buf->st_mode = 0x8000;
        }
        else
        {
            kdSetError(KD_EACCES);
            return -1;
        }
        LARGE_INTEGER size;
        size.LowPart = data.nFileSizeLow;
        size.HighPart = data.nFileSizeHigh;
        buf->st_size = size.QuadPart;

        ULARGE_INTEGER time;
        time.LowPart = data.ftLastWriteTime.dwLowDateTime;
        time.HighPart = data.ftLastWriteTime.dwHighDateTime;
        /* See RtlTimeToSecondsSince1970 */
        buf->st_mtime = (KDtime)((time.QuadPart / 10000000) - 11644473600LL);
    }
    else
    {
        error = GetLastError();
#else
    struct stat posixstat = {0};
    if(stat(pathname, &posixstat) == 0)
    {
        if(posixstat.st_mode & S_IFDIR)
        {
            buf->st_mode = 0x4000;
        }
        else if(posixstat.st_mode & S_IFREG)
        {
            buf->st_mode = 0x8000;
        }
        else
        {
            kdSetError(KD_EACCES);
            return -1;
        }
        buf->st_size = posixstat.st_size;
#if defined(__APPLE__)
        buf->st_mtime = posixstat.st_mtimespec.tv_sec;
#else
        /* We undef the st_mtime macro*/
        buf->st_mtime = posixstat.st_mtim.tv_sec;
#endif
    }
    else
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_EOVERFLOW);
        return -1;
    }
    return 0;
}

KD_API KDint KD_APIENTRY kdFstat(KDFile *file, struct KDStat *buf)
{
    return kdStat(file->pathname, buf);
}

/* kdAccess: Determine whether the application can access a file or directory. */
KD_API KDint KD_APIENTRY kdAccess(const KDchar *pathname, KDint amode)
{
    KDint error = 0;
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    if(FindFirstFileA(pathname, &data) != INVALID_HANDLE_VALUE)
    {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if(amode & KD_X_OK || amode & KD_R_OK)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        error = GetLastError();
#else
    typedef struct __KDAccessMode {
        KDint accessmode_kd;
        KDint accessmode_posix;
    } __KDAccessMode;
    __KDAccessMode accessmodes[] = {{KD_R_OK, R_OK}, {KD_W_OK, W_OK}, {KD_X_OK, X_OK}};
    KDint accessmode = 0;
    for(KDuint i = 0; i < sizeof(accessmodes) / sizeof(accessmodes[0]); i++)
    {
        if(accessmodes[i].accessmode_kd & amode)
        {
            accessmode |= accessmodes[i].accessmode_posix;
        }
    }
    if(access(pathname, accessmode) == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdOpenDir: Open a directory ready for listing. */
struct KDDir {
#if defined(_WIN32)
    HANDLE nativedir;
#else
    DIR *nativedir;
#endif
    KDDirent *dirent;
};
KD_API KDDir *KD_APIENTRY kdOpenDir(const KDchar *pathname)
{
    KDint error = 0;
    if(kdStrcmp(pathname, "/") == 0)
    {
        kdSetError(KD_EACCES);
        return KD_NULL;
    }
    KDDir *dir = (KDDir *)kdMalloc(sizeof(KDDir));
    if(dir == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    dir->dirent = (KDDirent *)kdMalloc(sizeof(KDDirent));
    if(dir->dirent == KD_NULL)
    {
        kdFree(dir);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
#if defined(_WIN32)
    KDchar dirpath[MAX_PATH];
    WIN32_FIND_DATA data;
    if(kdStrcmp(pathname, ".") == 0)
    {
        GetCurrentDirectoryA(MAX_PATH, dirpath);
    }
    kdStrncat_s(dirpath, MAX_PATH, "\\*", 2);
#if defined(_MSC_VER)
#pragma warning(suppress : 6102)
#endif
    dir->nativedir = FindFirstFileA((const KDchar *)dirpath, &data);
    if(dir->nativedir == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
#else
    dir->nativedir = opendir(pathname);
    if(dir->nativedir == KD_NULL)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        kdFree(dir->dirent);
        kdFree(dir);
        return KD_NULL;
    }
    return dir;
}

/* kdReadDir: Return the next file in a directory. */
KD_API KDDirent *KD_APIENTRY kdReadDir(KDDir *dir)
{
    KDint error = 0;
#if defined(_WIN32)
    WIN32_FIND_DATA data;

    if(FindNextFileA(dir->nativedir, &data) != 0)
    {
        dir->dirent->d_name = data.cFileName;
    }
    else
    {
        error = GetLastError();
        if(error == ERROR_NO_MORE_FILES)
        {
            return KD_NULL;
        }
#else
    struct dirent *de = readdir(dir->nativedir);
    if(de != KD_NULL)
    {
        dir->dirent->d_name = de->d_name;
    }
    else if(errno == 0)
    {
        return KD_NULL;
    }
    else
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EIO | KD_ENOMEM);
        return KD_NULL;
    }
    return dir->dirent;
}

/* kdCloseDir: Close a directory. */
KD_API KDint KD_APIENTRY kdCloseDir(KDDir *dir)
{
#if defined(_WIN32)
    FindClose(dir->nativedir);
#else
    closedir(dir->nativedir);
#endif
    kdFree(dir->dirent);
    kdFree(dir);
    return 0;
}

/* kdGetFree: Get free space on a drive. */
KD_API KDoff KD_APIENTRY kdGetFree(const KDchar *pathname)
{
    KDint error = 0;
    KDoff freespace = 0;
    const KDchar *temp = pathname;
#if defined(_WIN32)
    if(GetDiskFreeSpaceEx(temp, (PULARGE_INTEGER)&freespace, KD_NULL, KD_NULL) == 0)
    {
        error = GetLastError();
#else
    struct statfs buf = {0};
    if(statfs(temp, &buf) == 0)
    {
        freespace = (buf.f_bsize / 1024LL) * buf.f_bavail;
    }
    else
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSYS | KD_EOVERFLOW);
        return (KDoff)-1;
    }
    return freespace;
}

/******************************************************************************
 * Network sockets
 ******************************************************************************/

/* kdNameLookup: Look up a hostname. */
typedef struct {
    const KDchar *hostname;
    void *eventuserptr;
    KDThread *destination;
} __KDNameLookupPayload;
static void *__kdNameLookupHandler(void *arg)
{
    /* TODO: Make async, threadsafe and cancelable */
    __KDNameLookupPayload *payload = (__KDNameLookupPayload *)arg;

    static KDEventNameLookup lookupevent;
    kdMemset(&lookupevent, 0, sizeof(lookupevent));

    static KDSockaddr addr;
    kdMemset(&addr, 0, sizeof(addr));
    addr.family = KD_AF_INET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;
    kdMemset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    KDint retval = getaddrinfo(payload->hostname, 0, &hints, &result);
    if(retval != 0)
    {
        lookupevent.error = KD_EHOST_NOT_FOUND;
    }
    else
    {
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
        addr.data.sin.address = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
#if defined(_WIN32)
#undef s_addr
#endif
        lookupevent.resultlen = 1;
        lookupevent.result = &addr;
        freeaddrinfo(result);
    }

    /* Post event to the original thread */
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_NAME_LOOKUP_COMPLETE;
    event->userptr = payload->eventuserptr;
    event->data.namelookup = lookupevent;
    kdPostThreadEvent(event, payload->destination);

    return 0;
}
KD_API KDint KD_APIENTRY kdNameLookup(KDint af, const KDchar *hostname, void *eventuserptr)
{
    if(af != KD_AF_INET)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }

    static __KDNameLookupPayload payload;
    kdMemset(&payload, 0, sizeof(payload));
    payload.hostname = hostname;
    payload.eventuserptr = eventuserptr;
    payload.destination = kdThreadSelf();

    KDThread *thread = kdThreadCreate(KD_NULL, __kdNameLookupHandler, &payload);
    if(thread == KD_NULL)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            kdLogMessage("kdNameLookup() needs a threading implementation.\n");
            return -1;
        }
        kdSetError(KD_ENOMEM);
        return -1;
    }
    kdThreadDetach(thread);

    return 0;
}

/* kdNameLookupCancel: Selectively cancels ongoing kdNameLookup operations. */
KD_API void KD_APIENTRY kdNameLookupCancel(KD_UNUSED void *eventuserptr)
{
}

/* kdSocketCreate: Creates a socket. */
struct KDSocket {
#if defined(_WIN32)
    SOCKET nativesocket;
#else
    KDint nativesocket;
#endif
    KDint type;
    const struct KDSockaddr *addr;
    void *userptr;
};
KD_API KDSocket *KD_APIENTRY kdSocketCreate(KDint type, void *eventuserptr)
{
    KDSocket *sock = (KDSocket *)kdMalloc(sizeof(KDSocket));
    if(sock == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    sock->type = type;
    sock->addr = KD_NULL;
    sock->userptr = eventuserptr;
    if(sock->type == KD_SOCK_TCP)
    {
        KDint error = 0;
#if defined(_WIN32)
        sock->nativesocket = WSASocketA(AF_UNIX, SOCK_STREAM, 0, 0, 0, 0);
        if(sock->nativesocket == INVALID_SOCKET)
        {
            error = WSAGetLastError();
#else
        sock->nativesocket = socket(AF_UNIX, SOCK_STREAM, 0);
        if(sock->nativesocket == -1)
        {
            error = errno;
#endif
            kdFree(sock);
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EMFILE | KD_ENOMEM | KD_ENOSYS);
            return KD_NULL;
        }
    }
    else if(sock->type == KD_SOCK_UDP)
    {
        KDint error = 0;
#if defined(_WIN32)
        sock->nativesocket = WSASocketA(AF_UNIX, SOCK_DGRAM, 0, 0, 0, 0);
        if(sock->nativesocket == INVALID_SOCKET)
        {
            error = WSAGetLastError();
#else
        sock->nativesocket = socket(AF_UNIX, SOCK_DGRAM, 0);
        if(sock->nativesocket == -1)
        {
            error = errno;
#endif
            kdFree(sock);
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EMFILE | KD_ENOMEM | KD_ENOSYS);
            return KD_NULL;
        }
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_SOCKET_READABLE;
        event->userptr = sock->userptr;
        event->data.socketreadable.socket = sock;
        kdPostEvent(event);
    }
    else
    {
        kdFree(sock);
        kdSetError(KD_EINVAL);
        return KD_NULL;
    }
    return sock;
}

/* kdSocketClose: Closes a socket. */
KD_API KDint KD_APIENTRY kdSocketClose(KDSocket *socket)
{
#if defined(_WIN32)
    closesocket(socket->nativesocket);
#else
    close(socket->nativesocket);
#endif
    return 0;
}

/* kdSocketBind: Bind a socket. */
KD_API KDint KD_APIENTRY kdSocketBind(KDSocket *socket, const KDSockaddr *addr, KD_UNUSED KDboolean reuse)
{
    if(addr->family != KD_AF_INET)
    {
        kdSetError(KD_EAFNOSUPPORT);
        return -1;
    }

    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == KD_INADDR_ANY)
    {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
    KDint error = 0;
    KDint retval = bind(socket->nativesocket, (struct sockaddr *)&address, sizeof(address));
#if defined(_WIN32)
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EADDRINUSE | KD_EADDRNOTAVAIL | KD_EAFNOSUPPORT | KD_EINVAL | KD_EIO | KD_EISCONN | KD_ENOMEM);
        return -1;
    }

    socket->addr = addr;
    if(socket->type == KD_SOCK_TCP)
    {
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_SOCKET_READABLE;
        event->userptr = socket->userptr;
        event->data.socketreadable.socket = socket;
        kdPostEvent(event);
    }
    return 0;
}

/* kdSocketGetName: Get the local address of a socket. */
KD_API KDint KD_APIENTRY kdSocketGetName(KDSocket *socket, KDSockaddr *addr)
{
    kdMemcpy(&addr, &socket->addr, sizeof(KDSockaddr));
    return 0;
}

/* kdSocketConnect: Connects a socket. */
KD_API KDint KD_APIENTRY kdSocketConnect(KDSocket *socket, const KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == KD_INADDR_ANY)
    {
        address.sin_addr.s_addr = kdHtonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
    KDint error = 0;
#if defined(_WIN32)
    KDint retval = WSAConnect(socket->nativesocket, (struct sockaddr *)&address, sizeof(address), 0, 0, 0, 0);
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    KDint retval = connect(socket->nativesocket, (struct sockaddr *)&address, sizeof(address));
    if(retval == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EADDRINUSE | KD_EAFNOSUPPORT | KD_EALREADY | KD_ECONNREFUSED | KD_ECONNRESET | KD_EHOSTUNREACH | KD_EINVAL | KD_EIO | KD_EISCONN | KD_ENOMEM | KD_ETIMEDOUT);
        return -1;
    }
    return 0;
}

/* kdSocketListen: Listen on a socket. */
KD_API KDint KD_APIENTRY kdSocketListen(KD_UNUSED KDSocket *socket, KD_UNUSED KDint backlog)
{
    kdSetError(KD_ENOSYS);
    return -1;
}

/* kdSocketAccept: Accept an incoming connection. */
KD_API KDSocket *KD_APIENTRY kdSocketAccept(KD_UNUSED KDSocket *socket, KD_UNUSED KDSockaddr *addr, KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/* kdSocketSend, kdSocketSendTo: Send data to a socket. */
KD_API KDint KD_APIENTRY kdSocketSend(KDSocket *socket, const void *buf, KDint len)
{
    KDint error = 0;
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = kdMalloc(len);
    kdMemcpy(wsabuf.buf, buf, len);
    KDint retval = WSASend(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, 0, KD_NULL, KD_NULL);
    kdFree(wsabuf.buf);
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    result = send(socket->nativesocket, buf, len, 0);
    if(result == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAFNOSUPPORT | KD_EAGAIN | KD_ECONNRESET | KD_EDESTADDRREQ | KD_EIO | KD_ENOMEM | KD_ENOTCONN);
        return -1;
    }
    return result;
}

KD_API KDint KD_APIENTRY kdSocketSendTo(KDSocket *socket, const void *buf, KDint len, const KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == kdHtonl(KD_INADDR_ANY))
    {
        address.sin_addr.s_addr = kdHtonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
    KDint error = 0;
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = kdMalloc(len);
    kdMemcpy(wsabuf.buf, buf, len);
    KDint retval = WSASendTo(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, 0, (const struct sockaddr *)&address, sizeof(address), KD_NULL, KD_NULL);
    kdFree(wsabuf.buf);
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    result = sendto(socket->nativesocket, buf, len, 0, (struct sockaddr *)&address, sizeof(address));
    if(result == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAFNOSUPPORT | KD_EAGAIN | KD_ECONNRESET | KD_EDESTADDRREQ | KD_EIO | KD_ENOMEM | KD_ENOTCONN);
        return -1;
    }
    return result;
}

/* kdSocketRecv, kdSocketRecvFrom: Receive data from a socket. */
KD_API KDint KD_APIENTRY kdSocketRecv(KDSocket *socket, void *buf, KDint len)
{
    KDint error = 0;
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = buf;
    KDint retval = WSARecv(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, (DWORD[]){0}, KD_NULL, KD_NULL);
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    result = recv(socket->nativesocket, buf, len, 0);
    if(result == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAGAIN | KD_ECONNRESET | KD_EIO | KD_ENOMEM | KD_ENOTCONN | KD_ETIMEDOUT);
        return -1;
    }
    return result;
    ;
}

KD_API KDint KD_APIENTRY kdSocketRecvFrom(KDSocket *socket, void *buf, KDint len, KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    socklen_t addresssize = sizeof(address);

    KDint error = 0;
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = buf;
    KDint retval = WSARecvFrom(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, (DWORD[]){0}, (struct sockaddr *)&address, &addresssize, KD_NULL, KD_NULL);
    if(retval == SOCKET_ERROR)
    {
        error = WSAGetLastError();
#else
    result = recvfrom(socket->nativesocket, buf, len, 0, (struct sockaddr *)&address, &addresssize);
    if(result == -1)
    {
        error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAGAIN | KD_ECONNRESET | KD_EIO | KD_ENOMEM | KD_ENOTCONN | KD_ETIMEDOUT);
        return -1;
    }

    addr->family = KD_AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(address.sin_addr.s_addr == kdHtonl(INADDR_ANY))
    {
        addr->data.sin.address = kdHtonl(KD_INADDR_ANY);
    }
    else
    {
        addr->data.sin.address = kdHtonl(address.sin_addr.s_addr);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    addr->data.sin.port = kdHtons(address.sin_port);
    return 0;
}

/* kdHtonl: Convert a 32-bit integer from host to network byte order. */
KD_API KDuint32 KD_APIENTRY kdHtonl(KDuint32 hostlong)
{
    union {
        KDint i;
        KDchar c;
    } u = {1};
    if(u.c)
    {
        KDuint8 *s = (KDuint8 *)&hostlong;
        return (KDuint32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
    }
    else
    {
        return hostlong;
    }
}

/* kdHtons: Convert a 16-bit integer from host to network byte order. */
KD_API KDuint16 KD_APIENTRY kdHtons(KDuint16 hostshort)
{
    union {
        KDint i;
        KDchar c;
    } u = {1};
    if(u.c)
    {
        KDuint8 *s = (KDuint8 *)&hostshort;
        return (KDuint32)(s[0] << 8 | s[1]);
    }
    else
    {
        return hostshort;
    }
}

/* kdNtohl: Convert a 32-bit integer from network to host byte order. */
KD_API KDuint32 KD_APIENTRY kdNtohl(KDuint32 netlong)
{
    return kdHtonl(netlong);
}

/* kdNtohs: Convert a 16-bit integer from network to host byte order. */
KD_API KDuint16 KD_APIENTRY kdNtohs(KDuint16 netshort)
{
    return kdHtons(netshort);
}

/* kdInetAton: Convert a "dotted quad" format address to an integer. */
KD_API KDint KD_APIENTRY kdInetAton(KD_UNUSED const KDchar *cp, KD_UNUSED KDuint32 *inp)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdInetNtop: Convert a network address to textual form. */
KD_API const KDchar *KD_APIENTRY kdInetNtop(KDuint af, const void *src, KDchar *dst, KDsize cnt)
{
    if(af != KD_AF_INET)
    {
        kdSetError(KD_EAFNOSUPPORT);
        return KD_NULL;
    }
    if(cnt < KD_INET_ADDRSTRLEN)
    {
        kdSetError(KD_ENOSPC);
        return KD_NULL;
    }

    KDuint32 address = kdNtohl(((KDInAddr *)src)->s_addr);
    KDuint8 *s = (KDuint8 *)&address;
    KDchar tempstore[sizeof("255.255.255.255")] = "";
    kdSnprintfKHR(tempstore, sizeof(tempstore), "%u.%u.%u.%u", s[3], s[2], s[1], s[0]);
    kdStrcpy_s(dst, cnt, tempstore);
    return (const KDchar *)dst;
}

/******************************************************************************
 * Input/output
 ******************************************************************************/
/* kdStateGeti, kdStateGetl, kdStateGetf: get state value(s) */
KD_API KDint KD_APIENTRY kdStateGeti(KDint startidx, KDuint numidxs, KDint32 *buffer)
{
    KDint idx = startidx;
    for(KDuint i = 0; i != numidxs; i++)
    {
        switch(idx)
        {
#if defined(KD_WINDOW_SUPPORTED)
            case KD_STATE_POINTER_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.pointer.availability;;
                break;
            }
            case KD_INPUT_POINTER_X:
            {
                buffer[i] = __kd_window->states.pointer.x;
                break;
            }
            case KD_INPUT_POINTER_Y:
            {
                buffer[i] = __kd_window->states.pointer.y;
                break;
            }
            case KD_INPUT_POINTER_SELECT:
            {
                buffer[i] = __kd_window->states.pointer.select;
                break;
            }
            case KD_STATE_KEYBOARD_AVAILABILITY_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.availability;
                break;
            }
            case KD_INPUT_KEYBOARD_FLAGS_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.flags;
                break;
            }
            case KD_INPUT_KEYBOARD_CHAR_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.character;
                break;
            }
            case KD_INPUT_KEYBOARD_KEYCODE_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.keycode;
                break;
            }
            case KD_INPUT_KEYBOARD_CHARFLAGS_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.charflags;
                break;
            }
            case KD_STATE_DPAD_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.dpad.availability;
                break;
            }
            case KD_STATE_DPAD_COPY:
            {
                buffer[i] = -1;
                break;
            }
            case KD_INPUT_DPAD_UP:
            {
                buffer[i] = __kd_window->states.dpad.up;
                break;
            }
            case KD_INPUT_DPAD_LEFT:
            {
                buffer[i] = __kd_window->states.dpad.left;
                break;
            }
            case KD_INPUT_DPAD_RIGHT:
            {
                buffer[i] = __kd_window->states.dpad.right;
                break;
            }
            case KD_INPUT_DPAD_DOWN:
            {
                buffer[i] = __kd_window->states.dpad.down;
                break;
            }
            case KD_INPUT_DPAD_SELECT:
            {
                buffer[i] = __kd_window->states.dpad.select;
                break;
            }
            case KD_STATE_GAMEKEYS_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.gamekeys.availability;
                break;
            }
            case KD_INPUT_GAMEKEYS_UP:
            {
                buffer[i] = __kd_window->states.gamekeys.up;
                break;
            }
            case KD_INPUT_GAMEKEYS_LEFT:
            {
                buffer[i] = __kd_window->states.gamekeys.left;
                break;
            }
            case KD_INPUT_GAMEKEYS_RIGHT:
            {
                buffer[i] = __kd_window->states.gamekeys.right;
                break;
            }
            case KD_INPUT_GAMEKEYS_DOWN:
            {
                buffer[i] = __kd_window->states.gamekeys.down;
                break;
            }
            case KD_INPUT_GAMEKEYS_FIRE:
            {
                buffer[i] = __kd_window->states.gamekeys.fire;
                break;
            }
#endif
            case KD_STATE_EVENT_USING_BATTERY:
            {
#if defined(_WIN32)
                SYSTEM_POWER_STATUS status;
                GetSystemPowerStatus(&status);
                buffer[i] = (status.ACLineStatus == 0);
#else
                buffer[i] = 0;
#endif
                break;
            }
            case KD_STATE_EVENT_LOW_BATTERY:
            {
#if defined(_WIN32)
                SYSTEM_POWER_STATUS status;
                GetSystemPowerStatus(&status);
                buffer[i] = (status.BatteryFlag == 2) || (status.BatteryFlag == 4);
#else
                buffer[i] = 0;
#endif
                break;
            }
            case KD_STATE_BACKLIGHT_AVAILABILITY:
            {
#if defined(_WIN32)
                buffer[i] = 1;
#else
                buffer[i] = 0;
#endif
                break;
            }
            default:
            {
                kdSetError(KD_EIO);
                return -1;
            }
        }
        idx++;
    }
    return 0;
}

KD_API KDint KD_APIENTRY kdStateGetl(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDint64 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}

KD_API KDint KD_APIENTRY kdStateGetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDfloat32 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}


/* kdOutputSeti, kdOutputSetf: set outputs */
KD_API KDint KD_APIENTRY kdOutputSeti(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDint32 *buffer)
{
#if defined(_WIN32)
    if(startidx == KD_OUTPUT_BACKLIGHT_FORCE)
    {
        if(buffer[0])
        {
            SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
            return 0;
        }
    }
#endif
    kdSetError(KD_EIO);
    return -1;
}

KD_API KDint KD_APIENTRY kdOutputSetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDfloat32 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}

/******************************************************************************
 * Windowing
 ******************************************************************************/

#ifdef KD_WINDOW_SUPPORTED
/* kdCreateWindow: Create a window. */
#if defined(KD_WINDOW_WIN32)
static LRESULT CALLBACK __kdWindowsWindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
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
#elif defined(KD_WINDOW_EMSCRIPTEN)
EM_BOOL __kd_EmscriptenMouseCallback(KDint type, const EmscriptenMouseEvent *event, void *userptr)
{
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_INPUT_POINTER;
    if(type == EMSCRIPTEN_EVENT_MOUSEDOWN || type == EMSCRIPTEN_EVENT_MOUSEUP)
    {
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
        kdevent->data.inputpointer.select = (type == EMSCRIPTEN_EVENT_MOUSEDOWN) ? 1 : 0;

        __kd_window->states.pointer.select = kdevent->data.inputpointer.select;
    }
    else if(type == EMSCRIPTEN_EVENT_MOUSEMOVE)
    {
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
    }
    kdevent->data.inputpointer.x = event->canvasX;
    kdevent->data.inputpointer.y = event->canvasY;

    __kd_window->states.pointer.x = kdevent->data.inputpointer.x;
    __kd_window->states.pointer.y = kdevent->data.inputpointer.y;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

EM_BOOL __kd_EmscriptenKeyboardCallback(KDint type, const EmscriptenKeyboardEvent *event, void *userptr)
{
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr =__kd_window->eventuserptr;

    KDint32 keycode = __KDKeycodeLookup(event->keyCode);
    if(keycode)
    {
        kdevent->type = KD_EVENT_INPUT_KEY_ATX;
        KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

        keyevent->flags = 0;
        if(type == EMSCRIPTEN_EVENT_KEYDOWN)
        {
            keyevent->flags |= KD_KEY_PRESS_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_LEFT)
        {
            keyevent->flags |= KD_KEY_LOCATION_LEFT_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_RIGHT)
        {
            keyevent->flags |= KD_KEY_LOCATION_RIGHT_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_NUMPAD)
        {
            keyevent->flags |= KD_KEY_LOCATION_NUMPAD_ATX;
        }
        if(event->shiftKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
        }
        if(event->ctrlKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
        }
        if(event->altKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
        }
        if(event->metaKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
        }
        keyevent->keycode = keycode;

        __kd_window->states.keyboard.flags = keyevent->flags;
        __kd_window->states.keyboard.keycode = keyevent->keycode;
        
        __kdHandleSpecialKeys(__kd_window, keyevent);
    }
    else if(type == EMSCRIPTEN_EVENT_KEYDOWN)
    {
        KDint32 character = 0;
        /* Printable ASCII range. */
        if((event->key[0] >= 20) && (event->key[0] <= 126))
        {
            character = event->key[0];
        }

        if(character)
        {
            kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
            KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
            keycharevent->flags = 0;
            if(event->repeat)
            {
                keycharevent->flags = KD_KEY_AUTOREPEAT_ATX;
            }
            keycharevent->character = character;

            __kd_window->states.keyboard.charflags = keycharevent->flags;
            __kd_window->states.keyboard.character = keycharevent->character;
        }
        else
        {
            kdFreeEvent(kdevent);
            return 1;
        }
    }
    else
    {
        kdFreeEvent(kdevent);
        return 1;
    }

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

EM_BOOL __kd_EmscriptenFocusCallback(int type, const EmscriptenFocusEvent *event, void *user)
{
    if(type == EMSCRIPTEN_EVENT_FOCUSIN)
    {
        __kd_window->properties.focused = 1;
    }
    else if(type == EMSCRIPTEN_EVENT_FOCUSOUT)
    {
        __kd_window->properties.focused = 0;
    }

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = __kd_window->properties.focused;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

EM_BOOL __kd_EmscriptenVisibilityCallback(int type, const EmscriptenVisibilityChangeEvent *event, void *user)
{
    __kd_window->properties.visible = !event->hidden;

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
    kdevent->data.windowproperty.pname = KD_WINDOWPROPERTY_VISIBILITY;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

#elif defined(KD_WINDOW_WAYLAND)
static void __kdWaylandPointerHandleEnter(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED uint32_t serial, KD_UNUSED struct wl_surface *surface, KD_UNUSED wl_fixed_t sx, KD_UNUSED wl_fixed_t sy) 
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = 1;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandPointerHandleLeave(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED uint32_t serial, KD_UNUSED struct wl_surface *surface) 
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = 0;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandPointerHandleMotion(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    static KDuint32 lasttime = 0;
    if((lasttime + 15) < time)
    {
        struct KDWindow *window = data;

        KDEvent *kdevent = kdCreateEvent();
        kdevent->userptr = window->eventuserptr;
        kdevent->type = KD_EVENT_INPUT_POINTER;
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
        kdevent->data.inputpointer.x = wl_fixed_to_int(sx);
        kdevent->data.inputpointer.y = wl_fixed_to_int(sy);

        window->states.pointer.x = kdevent->data.inputpointer.x;
        window->states.pointer.y = kdevent->data.inputpointer.y;

        if(!__kdExecCallback(kdevent))
        {
            kdPostEvent(kdevent);
        }
    }
    lasttime = time;
}

static void __kdWaylandPointerHandleButton(void *data, KD_UNUSED struct wl_pointer *wl_pointer, KD_UNUSED uint32_t serial, KD_UNUSED uint32_t time, KD_UNUSED uint32_t button, uint32_t state)
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_INPUT_POINTER;
    kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
    kdevent->data.inputpointer.select = state;
    kdevent->data.inputpointer.x = window->states.pointer.x;
    kdevent->data.inputpointer.y = window->states.pointer.y;

    window->states.pointer.select = kdevent->data.inputpointer.select;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}

static void __kdWaylandPointerHandleAxis(KD_UNUSED void *data, KD_UNUSED struct wl_pointer *wl_pointer, KD_UNUSED uint32_t time, KD_UNUSED uint32_t axis, KD_UNUSED wl_fixed_t value) {}
static const struct wl_pointer_listener __kd_wl_pointer_listener = {
    __kdWaylandPointerHandleEnter,
    __kdWaylandPointerHandleLeave,
    __kdWaylandPointerHandleMotion,
    __kdWaylandPointerHandleButton,
    __kdWaylandPointerHandleAxis,
};

static void __kdWaylandKeyboardHandleKeymap(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
    struct KDWindow *window = data;
    if(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        KDchar *keymap_string = mmap(KD_NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        xkb_keymap_unref(window->xkb.keymap);
        window->xkb.keymap = xkb_keymap_new_from_string(window->xkb.context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(keymap_string, size);
        xkb_state_unref(window->xkb.state);
        window->xkb.state = xkb_state_new(window->xkb.keymap);
    }
}
static void __kdWaylandKeyboardHandleEnter(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED uint32_t serial, KD_UNUSED struct wl_surface *surface, KD_UNUSED struct wl_array *keys) {}
static void __kdWaylandKeyboardHandleLeave(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED uint32_t serial, KD_UNUSED struct wl_surface *surface) {}
static void __kdWaylandKeyboardHandleKey(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED uint32_t serial, KD_UNUSED uint32_t time, uint32_t key, uint32_t state)
{
    struct KDWindow *window = data;

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;

    xkb_keysym_t keysym = xkb_state_key_get_one_sym(window->xkb.state, key + 8);

    KDint32 character = 0;
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        /* Printable ASCII range. */
        if((keysym >= 20) && (keysym <= 126))
        {
            character = keysym;
        }
    }

    if(character)
    {
        kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
        keycharevent->character = character;

        __kd_window->states.keyboard.charflags = 0;
        __kd_window->states.keyboard.character = keycharevent->character;
    }
    else
    {
        KDint32 keycode = __KDKeycodeLookup(keysym);
        if(keycode)
        {
            kdevent->type = KD_EVENT_INPUT_KEY_ATX;
            KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

            if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
            {
                keyevent->flags |= KD_KEY_PRESS_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
            }

            keyevent->keycode = keycode;

            __kd_window->states.keyboard.flags = keyevent->flags;
            __kd_window->states.keyboard.keycode = keyevent->keycode;
            
            __kdHandleSpecialKeys(__kd_window, keyevent);
        }
        else
        {
            kdFreeEvent(kdevent);
            return;
        }
    }

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandKeyboardHandleModifiers(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    struct KDWindow *window = data;
    xkb_state_update_mask(window->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
static const struct wl_keyboard_listener __kd_wl_keyboard_listener = {
    __kdWaylandKeyboardHandleKeymap,
    __kdWaylandKeyboardHandleEnter,
    __kdWaylandKeyboardHandleLeave,
    __kdWaylandKeyboardHandleKey,
    __kdWaylandKeyboardHandleModifiers,
};

static void __kdWaylandSeatHandleCapabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
    struct KDWindow *window = data;
    if(caps & WL_SEAT_CAPABILITY_POINTER)
    {
        window->wayland.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(window->wayland.pointer, &__kd_wl_pointer_listener, window);
    }
    if(caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        window->wayland.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(window->wayland.keyboard, &__kd_wl_keyboard_listener, window);
    }
}
static const struct wl_seat_listener __kd_wl_seat_listener = {
    __kdWaylandSeatHandleCapabilities};

static void __kdWaylandRegistryAddObject(void *data, struct wl_registry *registry, uint32_t name, const char *interface, KD_UNUSED uint32_t version)
{
    struct KDWindow *window = data;
    if(!kdStrcmp(interface, "wl_compositor"))
    {
        window->wayland.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        window->wayland.surface = wl_compositor_create_surface(window->wayland.compositor);
    }
    else if(!kdStrcmp(interface, "wl_shell"))
    {
        window->wayland.shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
        window->wayland.shell_surface = wl_shell_get_shell_surface(window->wayland.shell, window->wayland.surface);
    }
    else if(!kdStrcmp(interface, "wl_seat"))
    {
        window->wayland.seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(window->wayland.seat, &__kd_wl_seat_listener, window);
    }
}
static void __kdWaylandRegistryRemoveObject(KD_UNUSED void *data, KD_UNUSED struct wl_registry *registry, KD_UNUSED uint32_t name) {}
static const struct wl_registry_listener __kd_wl_registry_listener = {
    __kdWaylandRegistryAddObject,
    __kdWaylandRegistryRemoveObject};

static void __kdWaylandShellSurfacePing(KD_UNUSED void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}
static void __kdWaylandShellSurfaceConfigure(KD_UNUSED void *data, KD_UNUSED struct wl_shell_surface *shell_surface, KD_UNUSED uint32_t edges, KD_UNUSED int32_t width, KD_UNUSED int32_t height) {}
static void __kdWaylandShellSurfacePopupDone(KD_UNUSED void *data, KD_UNUSED struct wl_shell_surface *shell_surface) {}
static struct wl_shell_surface_listener __kd_wl_shell_surface_listener = {
    &__kdWaylandShellSurfacePing,
    &__kdWaylandShellSurfaceConfigure,
    &__kdWaylandShellSurfacePopupDone};
#endif

KD_API NativeDisplayType KD_APIENTRY kdGetDisplayVEN(void)
{
#if defined(KD_WINDOW_WAYLAND)
    KDchar *sessiontype = kdGetEnvVEN("XDG_SESSION_TYPE");
    if(kdStrstrVEN(sessiontype, "wayland"))
    {
        __kd_wl_display = wl_display_connect(KD_NULL);
        return (NativeDisplayType)__kd_wl_display;
    }
#endif
    return (NativeDisplayType)EGL_DEFAULT_DISPLAY;
}

KD_API KDWindow *KD_APIENTRY kdCreateWindow(KD_UNUSED EGLDisplay display, KD_UNUSED EGLConfig config, void *eventuserptr)
{
    if(__kd_window)
    {
        /* One window only */
        kdSetError(KD_EPERM);
        return KD_NULL;
    }

    KDWindow *window = (KDWindow *)kdMalloc(sizeof(KDWindow));
    if(window == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    window->nativewindow = KD_NULL;
    window->nativedisplay = KD_NULL;
    window->platform = 0;
    if(eventuserptr == KD_NULL)
    {
        window->eventuserptr = window;
    }
    else
    {
        window->eventuserptr = eventuserptr;
    }
    window->originthr = kdThreadSelf();

    kdMemset(&window->properties, 0, sizeof(window->properties));
    kdMemset(&window->states.pointer, 0, sizeof(window->states.pointer));
    kdMemset(&window->states.keyboard, 0, sizeof(window->states.keyboard));

    /* If we have a window assume basic input support. */
    window->states.pointer.availability = 7;
    window->states.keyboard.availability = 15;
    window->states.dpad.availability = 31;
    window->states.gamekeys.availability = 31;

    const KDchar *caption = "OpenKODE";
    kdMemcpy(window->properties.caption, caption, kdStrlen(caption));

#if defined(KD_WINDOW_ANDROID)
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->format);
#endif

#if defined(KD_WINDOW_WIN32)
    window->screen.width = GetSystemMetrics(SM_CXSCREEN);
    window->screen.height = GetSystemMetrics(SM_CYSCREEN);
    window->properties.width = window->screen.width;
    window->properties.height = window->screen.height;

    WNDCLASS windowclass = {0};
    HINSTANCE instance = GetModuleHandle(KD_NULL);
    GetClassInfo(instance, "", &windowclass);
    windowclass.lpszClassName = window->properties.caption;
    windowclass.lpfnWndProc = __kdWindowsWindowCallback;
    windowclass.hInstance = instance;
    windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    RegisterClass(&windowclass);
    window->nativewindow = CreateWindow(window->properties.caption, window->properties.caption, WS_POPUP | WS_VISIBLE, 0, 0,
        window->properties.width, window->properties.height,
        KD_NULL, KD_NULL, instance, KD_NULL);

    window->nativedisplay = GetDC(window->nativewindow);
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
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    window->xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    /* Determine size of primary display. */
    xcb_connection_t* dummyconnection = xcb_connect(KD_NULL, KD_NULL);
    xcb_screen_t* dummyscreen = xcb_setup_roots_iterator(xcb_get_setup(dummyconnection)).data;
    xcb_window_t dummywindow = xcb_generate_id(dummyconnection);
    xcb_create_window(dummyconnection, 0, dummywindow, dummyscreen->root, 0, 0, 1, 1, 0, 0, 0, 0, 0);
    xcb_flush(dummyconnection);
    xcb_randr_get_screen_resources_cookie_t dummyscreencookie = xcb_randr_get_screen_resources(dummyconnection, dummywindow);
    xcb_randr_get_screen_resources_reply_t* dummyscreenreply = xcb_randr_get_screen_resources_reply(dummyconnection, dummyscreencookie, 0);
    xcb_randr_crtc_t* crtc = xcb_randr_get_screen_resources_crtcs(dummyscreenreply);
    xcb_randr_get_crtc_info_cookie_t crtccookie = xcb_randr_get_crtc_info(dummyconnection, crtc[0], 0);
    xcb_randr_get_crtc_info_reply_t* crtcreply = xcb_randr_get_crtc_info_reply(dummyconnection, crtccookie, NULL);
    window->screen.width = crtcreply->width;
    window->screen.height = crtcreply->height;
    window->properties.width = window->screen.width;
    window->properties.height = window->screen.height;
    xcb_disconnect(dummyconnection);

#if defined(KD_WINDOW_WAYLAND)
    if(__kd_wl_display)
    {
        window->platform = EGL_PLATFORM_WAYLAND_KHR;
    }
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        window->nativedisplay = __kd_wl_display;
        window->wayland.registry = wl_display_get_registry(window->nativedisplay);
        wl_registry_add_listener(window->wayland.registry, &__kd_wl_registry_listener, window);
        wl_display_roundtrip(window->nativedisplay);
        wl_shell_surface_add_listener(window->wayland.shell_surface, &__kd_wl_shell_surface_listener, window);
    }
#endif

#if defined(KD_WINDOW_X11)
    if(!window->platform)
    {
        /* Fallback to X11*/
        window->platform = EGL_PLATFORM_X11_KHR;
    }
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        if(!window->nativedisplay)
        {
            window->nativedisplay = xcb_connect(KD_NULL, KD_NULL);
        }
        xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(window->nativedisplay)).data;
        window->nativewindow = (void *)(KDuintptr)xcb_generate_id(window->nativedisplay);

        KDuint32 mask = XCB_CW_EVENT_MASK;
        KDuint32 values[1] = {XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW};

        xcb_create_window(window->nativedisplay, screen->root_depth, (KDuintptr)window->nativewindow,
            screen->root, 0, 0, window->properties.width, window->properties.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

        xcb_intern_atom_cookie_t *ewmhcookie = xcb_ewmh_init_atoms(window->nativedisplay, &window->xcb.ewmh);
        xcb_ewmh_init_atoms_replies(&window->xcb.ewmh, ewmhcookie, KD_NULL);

        xcb_intern_atom_cookie_t protcookie = xcb_intern_atom(window->nativedisplay, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_reply_t *protreply = xcb_intern_atom_reply(window->nativedisplay, protcookie, 0);
        xcb_intern_atom_cookie_t delcookie = xcb_intern_atom(window->nativedisplay, 0, 16, "WM_DELETE_WINDOW");
        xcb_intern_atom_reply_t *delreply = xcb_intern_atom_reply(window->nativedisplay, delcookie, 0);
        xcb_change_property(window->nativedisplay, XCB_PROP_MODE_REPLACE, (KDuintptr)window->nativewindow, (*protreply).atom, 4, 32, 1, &(*delreply).atom);

        xkb_x11_setup_xkb_extension(window->nativedisplay, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
            XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, KD_NULL, KD_NULL, &window->xkb.firstevent, KD_NULL);
        KDint32 device = xkb_x11_get_core_keyboard_device_id(window->nativedisplay);
        window->xkb.keymap = xkb_x11_keymap_new_from_device(window->xkb.context, window->nativedisplay, device, XKB_KEYMAP_COMPILE_NO_FLAGS);
        window->xkb.state = xkb_x11_state_new_from_device(window->xkb.keymap, window->nativedisplay, device);

        enum {
            required_events =
                (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                    XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                    XCB_XKB_EVENT_TYPE_STATE_NOTIFY),

            required_map_parts =
                (XCB_XKB_MAP_PART_KEY_TYPES |
                    XCB_XKB_MAP_PART_KEY_SYMS |
                    XCB_XKB_MAP_PART_MODIFIER_MAP |
                    XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                    XCB_XKB_MAP_PART_KEY_ACTIONS |
                    XCB_XKB_MAP_PART_VIRTUAL_MODS |
                    XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP),

            required_state_details =
                (XCB_XKB_STATE_PART_MODIFIER_BASE |
                    XCB_XKB_STATE_PART_MODIFIER_LATCH |
                    XCB_XKB_STATE_PART_MODIFIER_LOCK |
                    XCB_XKB_STATE_PART_GROUP_BASE |
                    XCB_XKB_STATE_PART_GROUP_LATCH |
                    XCB_XKB_STATE_PART_GROUP_LOCK),
        };

        static const xcb_xkb_select_events_details_t details = {
            .affectState = required_state_details,
            .stateDetails = required_state_details,
        };

        xcb_xkb_select_events_aux(window->nativedisplay, device, required_events, 0, 0,
            required_map_parts, required_map_parts, &details);
    }
#endif
#endif
    __kd_window = window;
    return window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
    if(window->originthr != kdThreadSelf())
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
#if defined(KD_WINDOW_WIN32)
    ReleaseDC(window->nativewindow, window->nativedisplay);
    DestroyWindow(window->nativewindow);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    xkb_state_unref(window->xkb.state);
    xkb_keymap_unref(window->xkb.keymap);
    xkb_context_unref(window->xkb.context);

#if defined(KD_WINDOW_WAYLAND)
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        if(window->wayland.keyboard)
        {
            wl_keyboard_destroy(window->wayland.keyboard);
        }
        if(window->wayland.pointer)
        {
            wl_pointer_destroy(window->wayland.pointer);
        }
        if(window->wayland.seat)
        {
            wl_seat_destroy(window->wayland.seat);
        }
        if(window->wayland.shell)
        {
            wl_shell_destroy(window->wayland.shell);
        }
        if(window->wayland.compositor)
        {
            wl_compositor_destroy(window->wayland.compositor);
        }
        wl_egl_window_destroy(window->nativewindow);
        wl_shell_surface_destroy(window->wayland.shell_surface);
        wl_surface_destroy(window->wayland.surface);
        wl_registry_destroy(window->wayland.registry);
        wl_display_disconnect(window->nativedisplay);
    }
#endif
#if defined(KD_WINDOW_X11)
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_disconnect(window->nativedisplay);
    }
#endif
#endif
    kdFree(window);
    __kd_window = KD_NULL;
    return 0;
}

/* kdSetWindowPropertybv, kdSetWindowPropertyiv, kdSetWindowPropertycv: Set a window property to request a change in the on-screen representation of the window. */
KD_API KDint KD_APIENTRY kdSetWindowPropertybv(KD_UNUSED KDWindow *window, KD_UNUSED KDint pname, KD_UNUSED const KDboolean *param)
{
    kdSetError(KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertyiv(KDWindow *window, KDint pname, const KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#if defined(KD_WINDOW_WIN32)
        SetWindowPos(window->nativewindow, 0, 0, 0, param[0], param[1], 0);
#elif defined(KD_WINDOW_EMSCRIPTEN)
        emscripten_set_canvas_element_size("#canvas",param[0], param[1]);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
        KDboolean fullscreen = (param[0] == window->screen.width) && (param[1] == window->screen.height);

#if defined(KD_WINDOW_WAYLAND)   
        if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            if(fullscreen)
            {
                wl_shell_surface_set_fullscreen(window->wayland.shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, KD_NULL);  
            }
            else
            {
                wl_shell_surface_set_toplevel(window->wayland.shell_surface);
            }
            if(window->nativewindow)
            {
                wl_egl_window_resize(window->nativewindow, param[0], param[1], 0, 0);
            }
        }
#endif
#if defined(KD_WINDOW_X11)
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            xcb_size_hints_t hints;
            kdMemset(&hints, 0, sizeof(hints));
            xcb_icccm_size_hints_set_min_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_base_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_max_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_size(&hints, 0, param[0], param[1]);
            xcb_icccm_size_hints_set_position(&hints, 0, 0, 0);
            xcb_icccm_set_wm_size_hints(window->nativedisplay, (KDuintptr)window->nativewindow, XCB_ATOM_WM_NORMAL_HINTS, &hints);  
            xcb_flush(window->nativedisplay);

            if(fullscreen)
            {
                xcb_change_property(window->nativedisplay, XCB_PROP_MODE_REPLACE, (KDuintptr)window->nativewindow,
                    window->xcb.ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &(window->xcb.ewmh._NET_WM_STATE_FULLSCREEN));
                xcb_flush(window->nativedisplay);
            }

        }
#endif   
#else
        kdSetError(KD_EINVAL);
        return -1;
#endif
        window->properties.width = param[0];
        window->properties.height = param[1];

        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        event->data.windowproperty.pname = KD_WINDOWPROPERTY_SIZE;
        kdPostEvent(event);
        return 0;
    }
    kdSetError(KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertycv(KDWindow *window, KDint pname, const KDchar *param)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#if defined(KD_WINDOW_WIN32)
        SetWindowTextA(window->nativewindow, param);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
        if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            wl_shell_surface_set_title(window->wayland.shell_surface, param);
        }
#endif
#if defined(KD_WINDOW_X11)
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            xcb_icccm_set_wm_name(window->nativedisplay, (KDuintptr)window->nativewindow, XCB_ATOM_STRING, 8, kdStrlen(window->properties.caption), window->properties.caption);
            xcb_flush(window->nativedisplay);
        }
#endif
#else
        kdSetError(KD_EINVAL);
        return -1;
#endif
        kdMemcpy(window->properties.caption, param, kdStrlen(param));

        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        event->data.windowproperty.pname = KD_WINDOWPROPERTY_CAPTION;
        kdPostEvent(event);
        return 0;
    }
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdGetWindowPropertybv, kdGetWindowPropertyiv, kdGetWindowPropertycv: Get the current value of a window property. */
KD_API KDint KD_APIENTRY kdGetWindowPropertybv(KDWindow *window, KDint pname, KDboolean *param)
{
    if(pname == KD_WINDOWPROPERTY_FOCUS)
    {
        param[0] = window->properties.focused;
    }
    else if(pname == KD_WINDOWPROPERTY_VISIBILITY)
    {
        param[0] = window->properties.visible;
    }
    kdSetError(KD_EINVAL);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertyiv(KDWindow *window, KDint pname, KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
        param[0] = window->properties.width;
        param[1] = window->properties.height;
    }
    kdSetError(KD_EINVAL);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertycv(KDWindow *window, KDint pname, KDchar *param, KDsize *size)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {   
        *size = kdStrlen(window->properties.caption);
        kdMemcpy(param, window->properties.caption, *size);
    }
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdRealizeWindow: Realize the window as a displayable entity and get the native window handle for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizeWindow(KDWindow *window, EGLNativeWindowType *nativewindow)
{
    KDint32 windowsize[2];
    windowsize[0] = window->properties.width;
    windowsize[1] = window->properties.height;
    kdSetWindowPropertyiv(window, KD_WINDOWPROPERTY_SIZE, windowsize);
    kdSetWindowPropertycv(window, KD_WINDOWPROPERTY_CAPTION, window->properties.caption);
    window->properties.focused = 1;
    window->properties.visible = 1;

#if defined(KD_WINDOW_ANDROID)
    for(;;)
    {
        kdThreadMutexLock(__kd_androidwindow_mutex);
        if(__kd_androidwindow)
        {
            window->nativewindow = __kd_androidwindow;
            kdThreadMutexUnlock(__kd_androidwindow_mutex);
            break;
        }
        kdThreadMutexUnlock(__kd_androidwindow_mutex);
    }
    ANativeWindow_setBuffersGeometry(window->nativewindow, 0, 0, window->format);
#elif defined(KD_WINDOW_EMSCRIPTEN)
    EmscriptenFullscreenStrategy strategy;
    kdMemset(&strategy, 0, sizeof(strategy));
    strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
    strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
    strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
    emscripten_enter_soft_fullscreen(0, &strategy);
    window->properties.width = 800;
    window->properties.height = 600;
    //emscripten_get_element_css_size("#canvas", (KDfloat64KHR*)&window->properties.width, (KDfloat64KHR*)&window->properties.height); 
    emscripten_set_canvas_element_size("#canvas", window->properties.width, window->properties.height);
    emscripten_set_mousedown_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_mouseup_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_mousemove_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_keydown_callback(0, 0, 1, __kd_EmscriptenKeyboardCallback);
    emscripten_set_keyup_callback(0, 0, 1, __kd_EmscriptenKeyboardCallback);
    emscripten_set_focusin_callback(0, 0, 1, __kd_EmscriptenFocusCallback);
    emscripten_set_focusout_callback(0, 0, 1, __kd_EmscriptenFocusCallback);
    emscripten_set_visibilitychange_callback(0, 1, __kd_EmscriptenVisibilityCallback);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        window->nativewindow = wl_egl_window_create(window->wayland.surface, window->properties.width, window->properties.height);
    }
#endif
#if defined(KD_WINDOW_X11)
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_map_window(window->nativedisplay, (KDuintptr)window->nativewindow);
        xcb_flush(window->nativedisplay);
    }
#endif
#endif

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
    kdevent->data.windowproperty.pname = KD_EVENT_WINDOW_REDRAW;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }

    if(nativewindow)
    {
        *nativewindow = (EGLNativeWindowType)window->nativewindow;
    }
    return 0;
}

#endif

/******************************************************************************
 * Assertions and logging
 ******************************************************************************/

/* kdHandleAssertion: Handle assertion failure. */
KD_API void KD_APIENTRY kdHandleAssertion(const KDchar *condition, const KDchar *filename, KDint linenumber)
{
    kdLogMessagefKHR("---Assertion---\nCondition: %s\nFile: %s(%i)\n", condition, filename, linenumber);

#if defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#elif defined(_MSC_VER) || defined(__MINGW32__)
    __debugbreak();
#else
    kdExit(-1);
#endif
}

/* kdLogMessage: Output a log message. */
#ifndef KD_NDEBUG
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
    KDsize length = kdStrlen(string);
    if(!length)
    {
        return;
    }
    kdLogMessagefKHR("%s", string);
}
#endif
