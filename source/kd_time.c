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
#define _GNU_SOURCE  // for clock_gettime, CLOCK_MONOTONIC_RAW
#endif
#include "kdplatform.h"  // for KDint32, KD_API, KD_APIENTRY
#include <KD/kd.h>       // for KDTm, KDtime, KDint, kdMemset
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(KD_FREESTANDING)
#include <time.h>  // for clock_gettime, time
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h> /* emscripten_get_now */
#endif

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
// IWYU pragma: no_include <bits/types/struct_timespec.h>
// IWYU pragma: no_include <bits/types/time_t.h>
// IWYU pragma: no_include <linux/time.h>
#include <sys/time.h>  // for CLOCK_MONOTONIC_RAW
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

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
    struct timespec ts;
    kdMemset(&ts, 0, sizeof(ts));
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * 1000000000LL) + ts.tv_nsec;
#elif defined(__MAC_10_12) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12 && __apple_build_version__ >= 800038
    /* Supported as of XCode 8 / macOS Sierra 10.12 */
    struct timespec ts;
    kdMemset(&ts, 0, sizeof(ts));
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
    return (KDust)(emscripten_get_now()) * 1000000LL;
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
    if(timep)
    {
        (*timep) = time;
    }
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
