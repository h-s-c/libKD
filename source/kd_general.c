// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2019 Kevin Schmidt
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
#include "kdplatform.h"        // for KD_API, KD_APIENTRY, KD_UNUSED, KDsize
#include <KD/kd.h>             // for KDint, KD_NULL, KDchar, kdThreadSelf
#include <KD/KHR_formatted.h>  // for kdLogMessagefKHR
#include <KD/kdext.h>          // for kdGetEnvVEN, kdStrstrVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // for KDThread

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(KD_FREESTANDING)
#include <errno.h>   // for EACCES, EAGAIN, EBADF, EBUSY, EEXIST
#include <locale.h>  // for setlocale, LC_ALL, LC_CTYPE
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winerror.h>
#include <winnls.h> /* GetLocaleInfoA */
#endif

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
        case(ERROR_INVALID_USER_BUFFER):
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
        return "1.0.3";
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
    static KDchar localestore[5];
    kdMemset(localestore, 0, sizeof(localestore));
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
 * Assertions and logging
 ******************************************************************************/

/* kdHandleAssertion: Handle assertion failure. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif
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
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/* kdLogMessage: Output a log message. */
#ifndef KD_NDEBUG
KD_API void KD_APIENTRY kdLogMessage(const KDchar *string)
{
    KDsize length = kdStrlen(string);
    if(!length)
    {
        return;
    }
    kdLogMessagefKHR("%s\n", string);
}
#endif
