// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2021 Kevin Schmidt
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

#if defined(__clang__)
#if __has_warning("-Wreserved-identifier")
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#endif

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
#include "kdplatform.h"        // for KD_API, KD_APIENTRY, KDssize, KDint64
#include <KD/kd.h>             // for KDchar, KDint, kdMemcpy, kdSetError
#include <KD/KHR_float64.h>    // for KDfloat64KHR
#include <KD/KHR_formatted.h>  // for kdSnprintfKHR
#include <KD/kdext.h>          // IWYU pragma: keep
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(KD_FREESTANDING)
#include <stdlib.h>  // for getenv
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__)
// IWYU pragma: no_include  <features.h>
#include <unistd.h>  // IWYU pragma: keep
#include <fcntl.h>   // IWYU pragma: keep
#if defined(__APPLE__) || defined(__GLIBC__)
#if(__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25) || (defined(__MAC_10_12) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12 && __apple_build_version__ >= 800038)
#include <sys/random.h>  // for getrandom, GRND_NONBLOCK
#endif
#endif
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h> /* emscripten_random */
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <processenv.h> /* GetEnvironmentVariable */
#include <wincrypt.h>   /* CryptGenRandom etc. */
#endif

/******************************************************************************
 * Utility library functions
 *
 * Notes:
 * - Based on the BSD libc developed at the University of California, Berkeley
 * - kdStrtof based on K&R Second Edition
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

/* kdIsalphaVEN: Check if character is alphabetic.*/
KD_API KDint KD_APIENTRY kdIsalphaVEN(KDint c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/* kdIsdigitVEN: Check if character is decimal digit. */
KD_API KDint KD_APIENTRY kdIsdigitVEN(KDint c)
{
    return ((c >= '0') && (c <= '9'));
}

/* kdIsspaceVEN: Check if character is a white-space. */
KD_API KDint KD_APIENTRY kdIsspaceVEN(KDint c)
{
    return ((c >= 0x09 && c <= 0x0D) || (c == 0x20));
}

/* kdIsupperVEN: Check if character is uppercase letter. */
KD_API KDint KD_APIENTRY kdIsupperVEN(KDint c)
{
    return ((c >= 'A') && (c <= 'Z'));
}

/* kdAbs: Compute the absolute value of an integer. */
KD_API KDint KD_APIENTRY kdAbs(KDint i)
{
    return (i < 0) ? -i : i;
}

/* kdMaxVEN: Returns the bigger of the given values. */
KD_API KDint KD_APIENTRY kdMaxVEN(KDint a, KDint b)
{
    return (b > a) ? b : a;
}

/* kdMinVEN: Returns the smaller of the given values. */
KD_API KDint KD_APIENTRY kdMinVEN(KDint a, KDint b)
{
    return (b < a) ? b : a;
}

/* kdStrtof: Convert a string to a floating point number. */
KD_API KDfloat32 KD_APIENTRY kdStrtof(const KDchar *s, KDchar **endptr)
{
    return (KDfloat32)kdStrtodKHR(s, endptr);
}

/* kdStrtodKHR: Convert a string to a 64-bit floating point number. */
KD_API KDfloat64KHR KD_APIENTRY kdStrtodKHR(const KDchar *s, KD_UNUSED KDchar **endptr)
{
    KDfloat64KHR val;
    KDfloat64KHR power;
    KDint i = 0;
    while(kdIsspaceVEN(s[i]))
    {
        i++;
    }
    KDint sign = (s[i] == '-') ? -1 : 1;
    if(s[i] == '+' || s[i] == '-')
    {
        i++;
    }
    for(val = 0.0; kdIsdigitVEN(s[i]); i++)
    {
        val = 10.0 * val + (s[i] - '0');
    }
    if(s[i] == '.')
    {
        i++;
    }
    for(power = 1.0; kdIsdigitVEN(s[i]); i++)
    {
        val = 10.0 * val + (s[i] - '0');
        power *= 10.0;
    }
    return sign * val / power;
}

/* kdStrtol, kdStrtoul: Convert a string to an integer. */
KD_API KDint KD_APIENTRY kdStrtol(const KDchar *nptr, KDchar **endptr, KDint base)
{
    KDchar _s;
    KDchar *s = &_s;
    KDint64 acc;
    KDint64 cutoff;
    KDint c;
    KDint neg;
    KDint any;
    KDint cutlim;
    /*
     * Ensure that base is between 2 and 36 inclusive, or the special
     * value of 0.
     */
    if(base < 0 || base == 1 || base > 36)
    {
        if(endptr != 0)
        {
            kdMemcpy(endptr, &nptr, sizeof(KDchar *));
        }
        kdSetError(KD_EINVAL);
        return 0;
    }
    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    kdMemcpy(&s, &nptr, sizeof(KDchar *));
    do
    {
        c = (KDuint8)*s++;
    } while(kdIsspaceVEN(c));
    if(c == '-')
    {
        neg = 1;
        c = *s++;
    }
    else
    {
        neg = 0;
        if(c == '+')
        {
            c = *s++;
        }
    }
    if((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
    {
        c = s[1];
        s += 2;
        base = 16;
    }
    if(base == 0)
    {
        base = c == '0' ? 8 : 10;
    }
    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for intmax_t is
     * [-9223372036854775808..9223372036854775807] and the input base
     * is 10, cutoff will be set to 922337203685477580 and cutlim to
     * either 7 (neg==0) or 8 (neg==1), meaning that if we have
     * accumulated a value > 922337203685477580, or equal but the
     * next digit is > 7 (or 8), the number is too big, and we will
     * return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */
    cutoff = neg ? KDINT_MIN : KDINT_MAX;
    cutlim = cutoff % base;
    cutoff /= base;
    if(neg)
    {
        if(cutlim > 0)
        {
            cutlim -= base;
            cutoff += 1;
        }
        cutlim = -cutlim;
    }
    for(acc = 0, any = 0;; c = (KDuint8)*s++)
    {
        if(kdIsdigitVEN(c))
        {
            c -= '0';
        }
        else if(kdIsalphaVEN(c))
        {
            c -= kdIsupperVEN(c) ? 'A' - 10 : 'a' - 10;
        }
        else
        {
            break;
        }
        if(c >= base)
        {
            break;
        }
        if(any < 0)
        {
            continue;
        }
        if(neg)
        {
            if(acc < cutoff || (acc == cutoff && c > cutlim))
            {
                any = -1;
                acc = KDINT_MIN;
                kdSetError(KD_ERANGE);
            }
            else
            {
                any = 1;
                acc *= base;
                acc -= c;
            }
        }
        else
        {
            if(acc > cutoff || (acc == cutoff && c > cutlim))
            {
                any = -1;
                acc = KDINT_MAX;
                kdSetError(KD_ERANGE);
            }
            else
            {
                any = 1;
                acc *= base;
                acc += c;
            }
        }
    }
    if(endptr)
    {
        KDchar _p;
        KDchar *p = &_p;
        kdMemcpy(&p, &nptr, sizeof(KDchar *));
        *endptr = (KDchar *)(any ? s - 1 : p);
    }
    return (KDint)acc;
}

KD_API KDuint KD_APIENTRY kdStrtoul(const KDchar *nptr, KDchar **endptr, KDint base)
{
    KDchar *s;
    KDint64 acc;
    KDint64 cutoff;
    KDint c;
    KDint neg;
    KDint any;
    KDint cutlim;
    /*
     * See strtoimax for comments as to the logic used.
     */
    if(base < 0 || base == 1 || base > 36)
    {
        if(endptr != 0)
        {
            kdMemcpy(endptr, &nptr, sizeof(KDchar *));
        }
        kdSetError(KD_EINVAL);
        return 0;
    }
    kdMemcpy(&s, &nptr, sizeof(KDchar *));
    do
    {
        c = (KDuint8)*s++;
    } while(kdIsspaceVEN(c));
    if(c == '-')
    {
        neg = 1;
        c = *s++;
    }
    else
    {
        neg = 0;
        if(c == '+')
        {
            c = *s++;
        }
    }
    if((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
    {
        c = s[1];
        s += 2;
        base = 16;
    }
    if(base == 0)
    {
        base = c == '0' ? 8 : 10;
    }
    cutoff = (KDint64)(KDUINT_MAX / (KDuint)base);
    cutlim = (KDint)(KDUINT_MAX % (KDuint)base);
    for(acc = 0, any = 0;; c = (KDuint8)*s++)
    {
        if(kdIsdigitVEN(c))
        {
            c -= '0';
        }
        else if(kdIsalphaVEN(c))
        {
            c -= kdIsupperVEN(c) ? 'A' - 10 : 'a' - 10;
        }
        else
        {
            break;
        }
        if(c >= base)
        {
            break;
        }
        if(any < 0)
        {
            continue;
        }
        if(acc > cutoff || (acc == cutoff && c > cutlim))
        {
            any = -1;
            acc = KDUINT_MAX;
            kdSetError(KD_ERANGE);
        }
        else
        {
            any = 1;
            acc *= (KDuint)base;
            acc += c;
        }
    }
    if(neg && any > 0)
    {
        acc = -acc;
    }
    if(endptr != 0)
    {
        KDchar _p;
        KDchar *p = &_p;
        kdMemcpy(&p, &nptr, sizeof(KDchar *));
        *endptr = (KDchar *)(any ? s - 1 : p);
    }
    return (KDuint)acc;
}

/* kdLtostr, kdUltostr: Convert an integer to a string. */
KD_API KDssize KD_APIENTRY kdLtostr(KDchar *buffer, KDsize buflen, KDint number)
{
    if(buflen == 0)
    {
        return -1;
    }
    KDssize retval = (KDssize)kdSnprintfKHR(buffer, buflen, "%d", number);
    if(retval > (KDssize)buflen)
    {
        return -1;
    }
    return retval;
}

KD_API KDssize KD_APIENTRY kdUltostr(KDchar *buffer, KDsize buflen, KDuint number, KDint base)
{
    if(buflen == 0)
    {
        return -1;
    }
    KDchar *fmt = "";
    if(base == 8)
    {
        fmt = "%o";
    }
    else if(base == 10)
    {
        fmt = "%u";
    }
    else if(base == 16)
    {
        fmt = "%x";
    }
    else
    {
        kdAssert(0);
    }
    KDssize retval = (KDssize)kdSnprintfKHR(buffer, buflen, (const KDchar *)fmt, number);
    if(retval > (KDssize)buflen)
    {
        return -1;
    }
    return retval;
}

/* kdFtostr: Convert a float to a string. */
KD_API KDssize KD_APIENTRY kdFtostr(KDchar *buffer, KDsize buflen, KDfloat32 number)
{
    if(buflen == 0)
    {
        return -1;
    }

    KDssize retval = (KDssize)kdSnprintfKHR(buffer, buflen, "%.9g", (KDfloat64KHR)number);
    if(retval > (KDssize)buflen)
    {
        return -1;
    }
    return retval;
}

/* kdDtostrKHR: Convert a 64-bit float to a string. */
KD_API KDssize KD_APIENTRY kdDtostrKHR(KDchar *buffer, KDsize buflen, KDfloat64KHR number)
{
    if(buflen == 0)
    {
        return -1;
    }
    KDssize retval = (KDssize)kdSnprintfKHR(buffer, buflen, "%.17g", number);
    if(retval > (KDssize)buflen)
    {
        return -1;
    }
    return retval;
}

/* kdCryptoRandom: Return random data. */
KD_API KDint KD_APIENTRY kdCryptoRandom(KD_UNUSED KDuint8 *buf, KD_UNUSED KDsize buflen)
{
    KDint retval = 0;
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ >= 25
    retval = (KDint)getrandom(buf, buflen, GRND_NONBLOCK);
#elif defined(__OpenBSD__) || (defined(__MAC_10_12) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12 && __apple_build_version__ >= 800038)
    /* Non-conforming to OpenKODE spec (blocking). */
    retval = (KDint)getentropy(buf, buflen);
#elif defined(_WIN32) && !defined(_M_ARM)
    /* Non-conforming to OpenKODE spec (blocking). */
    HCRYPTPROV provider = 0;
    retval = CryptAcquireContextA(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT) - 1;
    if(retval == 0)
    {
        retval = CryptGenRandom(provider, (KDuint32)buflen, buf) - 1;
    }
    CryptReleaseContext(provider, 0);
#elif defined(__EMSCRIPTEN__)
    /* TODO: Use window.crypto.getRandomValues() instead */
    for(KDsize i = 0; i < buflen; i++)
    {
        buf[i] = (KDuint8)(emscripten_random() * 255) % 256;
    }
#elif defined(__unix__) || defined(__APPLE__)
    KDint urandom = open("/dev/urandom", O_RDONLY);
    if(urandom)
    {
        KDsize randomlen = 0;
        while(randomlen < buflen)
        {
            KDssize result = read(urandom, buf + randomlen, buflen - randomlen);
            if(result < 0)
            {
                retval = -1;
            }
            randomlen += (KDsize)result;
        }
        close(urandom);
    }
#else
    kdLogMessage("No cryptographic RNG available.");
    kdAssert(0);
#endif
    /* cppcheck-suppress knownConditionTrueFalse */
    if(retval == -1)
    {
        kdSetError(KD_ENOMEM);
    }
    return retval;
}

/* kdGetEnvVEN: Get an environment variable. */
KD_API KDchar *KD_APIENTRY kdGetEnvVEN(const KDchar *env)
{
#if defined(_WIN32)
    static KDchar buf[32767];
    DWORD result = GetEnvironmentVariableA(env, (KDchar *)buf, 32767);
    if(result == 0)
    {
        return KD_NULL;
    }
    return (KDchar *)buf;
#else
    return getenv(env);
#endif
}
