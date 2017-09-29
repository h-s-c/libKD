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
 * Header workarounds
 ******************************************************************************/

/* clang-format off */
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#   define _GNU_SOURCE /* clock_gettime */
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/kdext.h>

#include "kd_internal.h"

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(KD_FREESTANDING)
#   include <errno.h>
#   include <locale.h> /* setlocale */
#   include <time.h> /* clock, time */
#   include <stdlib.h> /* malloc etc. */
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__APPLE__)
#   include <TargetConditionals.h>
#endif

#if defined(__EMSCRIPTEN__)
#   include <emscripten/emscripten.h>
#endif

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <winerror.h>
#   include <winnls.h> /* GetLocaleInfoA */
#endif
/* clang-format on */

/******************************************************************************
 * Errors
 ******************************************************************************/

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
} _KDTimerPayload;
static void *__kdTimerHandler(void *arg)
{
    _KDTimerPayload *payload = (_KDTimerPayload *)arg;
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
    _KDTimerPayload *payload;
};
KD_API KDTimer *KD_APIENTRY kdSetTimer(KDint64 interval, KDint periodic, void *eventuserptr)
{
    if(periodic != KD_TIMER_ONESHOT && periodic != KD_TIMER_PERIODIC_AVERAGE && periodic != KD_TIMER_PERIODIC_MINIMUM)
    {
        kdLogMessage("kdSetTimer() encountered unknown periodic value.\n");
        return KD_NULL;
    }

    _KDTimerPayload *payload = (_KDTimerPayload *)kdMalloc(sizeof(_KDTimerPayload));
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
