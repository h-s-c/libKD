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
#include "kdplatform.h"        // for KDsize, KDVaListKHR, KD_API, KD_APIENTRY
#include <KD/kd.h>             // for KDchar, KDint, kdStrncpy_s, kdSetError, kdS...
#include "KD/KHR_formatted.h"  // for kdFprintfKHR, kdFscanfKHR, kdLogMessag...
#include <KD/kdext.h>          // for kdIsspaceVEN, kdStrcspnVEN, kdIsdigitVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

#if defined(__EMSCRIPTEN__) && !defined(KD_FREESTANDING)
#include <stdio.h> /* vprintf */
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__)
#if defined(__ANDROID__)
#include <android/log.h>  // for __android_log_vprint, android_LogPriority::...
#else
#include <unistd.h>  // for STDOUT_FILENO
#endif
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> /* GetStdHandle */
#include <fileapi.h> /* WriteFile etc. */
#endif

/******************************************************************************
 * Thirdparty includes
 ******************************************************************************/

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_VA_LIST KDVaListKHR
#define STB_SPRINTF_VA_START KD_VA_START_KHR
#define STB_SPRINTF_VA_END KD_VA_END_KHR
#define STB_SPRINTF_VA_ARG_INT(i) KD_VA_ARG_INT_KHR(i)
#define STB_SPRINTF_VA_ARG_INT32(i) KD_VA_ARG_INT32_KHR(i)
#define STB_SPRINTF_VA_ARG_INT64(i) KD_VA_ARG_INT64_KHR(i)
#define STB_SPRINTF_VA_ARG_FLOAT64(d) KD_VA_ARG_FLOAT64_KHR(d)
#define STB_SPRINTF_VA_ARG_PTR(p) KD_VA_ARG_PTR_KHR(p)
#define STB_SPRINTF_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#if __has_warning("-Wextra-semi-stmt")
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#endif
#if __has_warning("-Wimplicit-fallthrough")
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#endif
#include "stb_sprintf.h"  // for stbsp_vsprintfcb, STB_SPRINTF_MIN, stbsp_vs...
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/******************************************************************************
 * OpenKODE Core extension: KD_KHR_formatted
 *
 * Notes:
 * - kdVsscanfKHR based on work by Opsycon AB
 ******************************************************************************/
/******************************************************************************
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ******************************************************************************/

/* kdSnprintfKHR, kdVsnprintfKHR, kdSprintfKHR, kdVsprintfKHR: Formatted output to a buffer. */
KD_API KDint KD_APIENTRY kdSnprintfKHR(KDchar *buf, KDsize bufsize, const KDchar *format, ...)
{
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    result = kdVsnprintfKHR(buf, bufsize, format, ap);
    KD_VA_END_KHR(ap);
    return result;
}

KD_API KDint KD_APIENTRY kdVsnprintfKHR(KDchar *buf, KDsize bufsize, const KDchar *format, KDVaListKHR ap)
{
    return stbsp_vsnprintf(buf, (KDint)bufsize, format, ap);
}

KD_API KDint KD_APIENTRY kdSprintfKHR(KDchar *buf, const KDchar *format, ...)
{
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    result = kdVsprintfKHR(buf, format, ap);
    KD_VA_END_KHR(ap);
    return result;
}

KD_API KDint KD_APIENTRY kdVsprintfKHR(KDchar *buf, const KDchar *format, KDVaListKHR ap)
{
    return stbsp_vsprintf(buf, format, ap);
}

/* kdFprintfKHR, kdVfprintfKHR: Formatted output to an open file. */
KD_API KDint KD_APIENTRY kdFprintfKHR(KDFile *file, const KDchar *format, ...)
{
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    result = kdVfprintfKHR(file, format, ap);
    KD_VA_END_KHR(ap);
    return result;
}

static KDchar *__kdVfprintfCallback(KDchar *buf, void *user, KDint len)
{
    KDFile *file = (KDFile *)user;
    for(KDint i = 0; i < len; i++)
    {
        kdPutc(buf[i], file);
    }
    if(len < STB_SPRINTF_MIN)
    {
        return KD_NULL;
    }
    /* Reuse buffer */
    return buf;
}

KD_API KDint KD_APIENTRY kdVfprintfKHR(KDFile *file, const KDchar *format, KDVaListKHR ap)
{
    KDchar buf[STB_SPRINTF_MIN];
    return stbsp_vsprintfcb(&__kdVfprintfCallback, &file, buf, format, ap);
}

/* kdLogMessagefKHR: Formatted output to the platform's debug logging facility. */
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
static KDchar *__kdLogMessagefCallback(KDchar *buf, KD_UNUSED void *user, KDint len)
{
#if defined(_WIN32)
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(out, buf, len, (DWORD[]) {0}, KD_NULL);
#else
    KDssize result = write(STDOUT_FILENO, buf, (KDsize)len);
    if(result != len)
    {
        return KD_NULL;
    }
#endif
    if(len < STB_SPRINTF_MIN)
    {
#if defined(_WIN32)
        FlushFileBuffers(out);
#endif
        return KD_NULL;
    }
    /* Reuse buffer */
    return buf;
}
#endif

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
__attribute__((__format__(__printf__, 1, 2)))
#endif
KD_API KDint KD_APIENTRY
kdLogMessagefKHR(const KDchar *format, ...)
{
#ifdef KD_NDEBUG
    return 0;
#endif
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);

#if defined(__ANDROID__)
    result = __android_log_vprint(ANDROID_LOG_INFO, "OpenKODE", format, ap);
#elif defined(__EMSCRIPTEN__)
    result = vprintf(format, ap);
#else
    KDchar buf[STB_SPRINTF_MIN];
    result = stbsp_vsprintfcb(&__kdLogMessagefCallback, KD_NULL, buf, format, ap);
#endif

    KD_VA_END_KHR(ap);
    return result;
}

/* kdSscanfKHR, kdVsscanfKHR: Read formatted input from a buffer. */
KD_API KDint KD_APIENTRY kdSscanfKHR(const KDchar *str, const KDchar *format, ...)
{
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    result = kdVsscanfKHR(str, format, ap);
    KD_VA_END_KHR(ap);
    return result;
}

KD_API KDint KD_APIENTRY kdVsscanfKHR(const KDchar *str, const KDchar *format, KDVaListKHR ap)
{
    KDint count;
    KDint noassign;
    KDint width;
    KDint base;
    KDint lflag;
    const KDchar *tc;
    KDchar *t;
    KDchar tmp[128];

    count = noassign = width = lflag = 0;
    while(*format && *str)
    {
        while(kdIsspaceVEN(*format))
        {
            format++;
        }
        if(*format == '%')
        {
            format++;
            for(; *format; format++)
            {
                if(kdStrchr("dibouxcsefg%", *format))
                {
                    break;
                }
                if(*format == '*')
                {
                    noassign = 1;
                }
                else if(*format == 'l' || *format == 'L')
                {
                }
                else if(*format >= '1' && *format <= '9')
                {
                    tc = format;
                    while(kdIsdigitVEN(*format))
                    {
                        format++;
                    }
                    kdStrncpy_s(tmp, (KDsize)(format - tc), tc, (KDsize)(format - tc));
                    tmp[format - tc] = '\0';
                    width = kdStrtol(tmp, KD_NULL, 10);
                    format--;
                }
            }
            if(*format == 's')
            {
                while(kdIsspaceVEN(*str))
                {
                    str++;
                }
                if(!width)
                {
                    width = (KDint)kdStrcspnVEN(str, " \t\n\r\f\v");
                }
                if(!noassign)
                {
                    kdStrncpy_s(t = (KDchar *)KD_VA_ARG_PTR_KHR(ap), (KDsize)width, str, (KDsize)width);
                    t[width] = '\0';
                }
                str += width;
            }
            else if(*format == 'c')
            {
                if(!width)
                {
                    width = 1;
                }
                if(!noassign)
                {
                    kdStrncpy_s(t = (KDchar *)KD_VA_ARG_PTR_KHR(ap), (KDsize)width, str, (KDsize)width);
                    t[width] = '\0';
                }
                str += width;
            }
            else if(kdStrchr("dobxu", *format))
            {
                while(kdIsspaceVEN(*str))
                {
                    str++;
                }
                if(*format == 'd' || *format == 'u')
                {
                    base = 10;
                }
                else if(*format == 'x')
                {
                    base = 16;
                }
                else if(*format == 'o')
                {
                    base = 8;
                }
                else if(*format == 'b')
                {
                    base = 2;
                }
                else
                {
                    base = 10;
                }
                if(!width)
                {
                    if(kdIsspaceVEN(*(format + 1)) || *(format + 1) == 0)
                    {
                        width = (KDint)kdStrcspnVEN(str, " \t\n\r\f\v");
                    }
                    else
                    {
                        width = (KDint)(kdStrchr(str, *(format + 1)) - str);
                    }
                }
                kdStrncpy_s(tmp, (KDsize)width, str, (KDsize)width);
                tmp[width] = '\0';
                str += width;
                if(!noassign)
                {
                    KDchar *i = KD_VA_ARG_PTR_KHR(ap);
                    *i = (KDchar)kdStrtol(tmp, KD_NULL, base);
                }
            }
            if(!noassign)
            {
                count++;
            }
            width = noassign = 0;
            format++;
        }
        else
        {
            while(kdIsspaceVEN(*str))
            {
                str++;
            }
            if(*format != *str)
            {
                break;
            }
            else
            {
                format++;
                str++;
            }
        }
    }
    return (count);
}

/* kdFscanfKHR, kdVfscanfKHR: Read formatted input from a file. */
KD_API KDint KD_APIENTRY kdFscanfKHR(KDFile *file, const KDchar *format, ...)
{
    KDint result = 0;
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    result = kdVfscanfKHR(file, format, ap);
    KD_VA_END_KHR(ap);
    return result;
}

KD_API KDint KD_APIENTRY kdVfscanfKHR(KDFile *file, const KDchar *format, KDVaListKHR ap)
{
    KDStat st;
    if(kdFstat(file, &st) == -1)
    {
        kdSetError(KD_EIO);
        return KD_EOF;
    }
    KDsize size = (KDsize)st.st_size;
    void *buffer = kdMalloc(size);
    if(buffer == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_EOF;
    }
    if(kdFread(buffer, 1, size, file) != size)
    {
        kdFree(buffer);
        kdSetError(KD_EIO);
        return KD_EOF;
    }
    KDint retval = kdVsscanfKHR((const KDchar *)buffer, format, ap);
    kdFree(buffer);
    return retval;
}
