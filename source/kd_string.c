// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2018 Kevin Schmidt
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
#include "kdplatform.h"  // for KDsize, KD_API, KD_APIENTRY, KDuint32, KDuin...
#include <KD/kd.h>       // for KDchar, KDint, KDuint8, KDint8, KD_NULL
#include <KD/kdext.h>    // IWYU pragma: keep
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__SSE4_2__)
#include <nmmintrin.h>
#elif defined(__SSE4_1__)
#include <smmintrin.h>
#endif

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

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
 * Copyright (c) 1998 Todd C. Miller
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
    return (KDuint32)__builtin_ctz(x);
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

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
/* Silence -Wunused-function */
static KD_UNUSED KDuint32 (*__dummyfunc)(KDuint32) = &__kdBitScanForward;
#endif

/* kdMemchr: Scan memory for a byte value. */
KD_API void *KD_APIENTRY kdMemchr(const void *src, KDint byte, KDsize len)
{
    if(!len)
    {
        return KD_NULL;
    }

    KDchar _p;
    KDchar *p = &_p;
    kdMemcpy(&p, &src, sizeof(KDchar *));

#if defined(__SSE2__) || defined(__ARM_NEON__)
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
            ip &= (KDuintptr)~15;
            KDuint32 mask = 0;
#if defined(__SSE2__)
            __m128i x = *(const __m128i *)ip;
            __m128i a = _mm_cmpeq_epi8(x, c16);
            mask = (KDuint32)_mm_movemask_epi8(a);
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
            __m128i x;
            __m128i y;
            kdMemcpy(&x, &p[0], sizeof(__m128i));
            kdMemcpy(&y, &p[16], sizeof(__m128i));
            __m128i a = _mm_cmpeq_epi8(x, c16);
            __m128i b = _mm_cmpeq_epi8(y, c16);
            mask = (KDuint32)((_mm_movemask_epi8(b) << 16) | _mm_movemask_epi8(a));
#elif defined(__ARM_NEON__)
            uint8x16_t x;
            uint8x16_t y;
            kdMemcpy(&x, &p[0], sizeof(uint8x16_t));
            kdMemcpy(&y, &p[16], sizeof(uint8x16_t));
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
    do
    {
        if(*p++ == (KDuint8)byte)
        {
            return ((void *)(p - 1));
        }
    } while(--len != 0);
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
    __m128i _ptr1;
    __m128i _ptr2;
    __m128i *ptr1 = &_ptr1;
    __m128i *ptr2 = &_ptr2;
    kdMemcpy(&ptr1, &src1, sizeof(__m128i *));
    kdMemcpy(&ptr2, &src2, sizeof(__m128i *));
    enum { mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_LEAST_SIGNIFICANT };

    for(; len != 0; ptr1++, ptr2++)
    {
        const __m128i a = _mm_loadu_si128(ptr1);
        const __m128i b = _mm_loadu_si128(ptr2);

        if(_mm_cmpestrc(a, (KDint)len, b, (KDint)len, mode))
        {
            const KDint idx = _mm_cmpestri(a, (KDint)len, b, (KDint)len, mode);
            const KDuint8 b1 = (KDuint8)(((KDchar *)ptr1)[idx]);
            const KDuint8 b2 = (KDuint8)(((KDchar *)ptr2)[idx]);

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

    __m128i _mem;
    __m128i *mem = &_mem;
    kdMemcpy(&mem, &str, sizeof(KDchar *));
    const __m128i set = _mm_setr_epi8((KDchar)ch, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
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
            KDchar _p;
            KDchar *p = &_p;
            kdMemcpy(&p, &str, sizeof(KDchar *));
            return p;
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
    while(*str1 == *str2++)
    {
        if(*str1++ == '\0')
        {
            return 0;
        }
    }
    return *str1 - *(str2 - 1);
}

/* kdStrlen: Determine the length of a string. */
KD_API KDsize KD_APIENTRY kdStrlen(const KDchar *str)
{
    const KDchar *s = str;
#if defined(__SSE4_2__) && !defined(KD_ASAN)
    KDsize result = 0;
    const __m128i zeros = _mm_setzero_si128();

    __m128i _mem;
    __m128i *mem = &_mem;
    kdMemcpy(&mem, &s, sizeof(KDchar *));

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
            KDint idx = _mm_cmpistri(data, zeros, mode);
            return result + (KDsize)idx;
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
#elif(defined(__SSE2__) && !defined(KD_ASAN)) || defined(__ARM_NEON__)
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
        ip &= (KDuintptr)~15;
        KDuint32 mask = 0;
#if defined(__SSE2__)
        __m128i x = *(const __m128i *)ip;
        __m128i a = _mm_cmpeq_epi8(x, c16);
        mask = (KDuint32)_mm_movemask_epi8(a);
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
        __m128i x;
        kdMemcpy(&x, &s[0], sizeof(__m128i));
        __m128i a = _mm_cmpeq_epi8(x, c16);
        mask = (KDuint32)_mm_movemask_epi8(a);
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
            return (KDsize)(s + __kdBitScanForward(mask) - str);
        }
        s += 16;
    }
    kdAssert(((KDuintptr)s & 31) == 0);
    for(;;)
    {
        KDuint32 mask = 0;
#if defined(__SSE2__)
        __m128i x;
        __m128i y;
        kdMemcpy(&x, &s[0], sizeof(__m128i));
        kdMemcpy(&y, &s[16], sizeof(__m128i));
        __m128i a = _mm_cmpeq_epi8(x, c16);
        __m128i b = _mm_cmpeq_epi8(y, c16);
        mask = (KDuint32)((_mm_movemask_epi8(b) << 16) | _mm_movemask_epi8(a));
#elif defined(__ARM_NEON__)
        uint8x16_t x;
        uint8x16_t y;
        kdMemcpy(&x, &s[0], sizeof(uint8x16_t));
        kdMemcpy(&y, &s[16], sizeof(uint8x16_t));
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
            return (KDsize)(s + __kdBitScanForward(mask) - str);
        }
        s += 32;
    }
#else
    for(; *s; ++s)
    {
        ;
    }
    return (KDsize)(s - str);
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
    dlen = (KDsize)(d - buf);
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

    return (KDint)(dlen + (KDsize)(s - src)); /* count does not include NUL */
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
                    return (KD_NULL);
                }
            } while(sc != c);
        } while(kdStrncmp(str1, str2, len) != 0);
        str1--;
    }
    KDchar _p;
    KDchar *p = &_p;
    kdMemcpy(&p, &str1, sizeof(KDchar *));
    return p;
}

/* kdStrcspnVEN:  Get span until character in string. */
KD_API KDsize KD_APIENTRY kdStrcspnVEN(const KDchar *str1, const KDchar *str2)
{
    KDsize retval = 0;
    while(*str1)
    {
        if(kdStrchr(str2, *str1))
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
