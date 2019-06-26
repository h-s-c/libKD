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
#include "kdplatform.h"      // for KDint32, KDuint32, KD_API, KD_APIENTRY
#include <KD/kd.h>           // for KDfloat32, KDINT32_MAX, KDint
#include <KD/kdext.h>        // for kdBitsToFloatNV
#include <KD/KHR_float64.h>  // for KDfloat64KHR, KD_PI_2_KHR, KD_PI_KHR
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

#include <float.h>  // for LDBL_MAX_EXP

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef inline
#define inline __inline /* MSVC redefinition fix */
#endif
#endif

/******************************************************************************
 * Mathematical functions
 *
 * Notes:
 * - Based on FDLIBM developed at Sun Microsystems, Inc.
 * - kdRoundf/kdRoundKHR based on work by Steven G. Kargl
 ******************************************************************************/
/******************************************************************************
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 ******************************************************************************/
/******************************************************************************
 * Copyright (c) 2003, Steven G. Kargl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#if defined(_MSC_VER)
#pragma warning(disable : 4204)
#pragma warning(disable : 4723)
#pragma warning(disable : 4756)
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif

/* TODO: Replace macros (see uses of __KDFloatShape) */
/*
 * A union which permits us to convert between a float and a 32 bit
 * int.
 */
typedef union {
    KDfloat32 f32;
    KDuint32 u32;
} __KDFloatShape;

/* Get a 32 bit int from a float.  */
#define GET_FLOAT_WORD(u, f)  \
    do                        \
    {                         \
        __KDFloatShape shape; \
        shape.f32 = (f);      \
        (u) = shape.u32;      \
    } while(0)

/* Set a float from a 32 bit int.  */
#define SET_FLOAT_WORD(f, u)  \
    do                        \
    {                         \
        __KDFloatShape shape; \
        shape.u32 = (u);      \
        (f) = shape.f32;      \
    } while(0)

/*
 * A union which permits us to convert between a double and two 32 bit
 * ints.
 */

typedef union {
    KDfloat64KHR value;
    struct {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        /* Big endian */
        KDuint32 msw;
        KDuint32 lsw;
#else
        /* Little endian */
        KDuint32 lsw;
        KDuint32 msw;
#endif
    } parts;
    struct {
        KDuint64 w;
    } xparts;
} ieee_double_shape_type;

typedef union {
    KDfloat64KHR f64;
    KDuint64 u64;
} __KDFloat64Shape;

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(ix0, ix1, d)   \
    do                               \
    {                                \
        ieee_double_shape_type ew_u; \
        ew_u.value = (d);            \
        (ix0) = ew_u.parts.msw;      \
        (ix1) = ew_u.parts.lsw;      \
    } while(0)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d, ix0, ix1)    \
    do                               \
    {                                \
        ieee_double_shape_type iw_u; \
        iw_u.parts.msw = (ix0);      \
        iw_u.parts.lsw = (ix1);      \
        (d) = iw_u.value;            \
    } while(0)

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD(d, v)          \
    do                               \
    {                                \
        ieee_double_shape_type sh_u; \
        sh_u.value = (d);            \
        sh_u.parts.msw = (v);        \
        (d) = sh_u.value;            \
    } while(0)

/* Get the more significant 32 bit int from a double.  */
#define GET_HIGH_WORD(i, d)          \
    do                               \
    {                                \
        ieee_double_shape_type gh_u; \
        gh_u.value = (d);            \
        (i) = gh_u.parts.msw;        \
    } while(0)

/* Set the less significant 32 bits of a double from an int.  */
#define SET_LOW_WORD(d, v)           \
    do                               \
    {                                \
        ieee_double_shape_type sl_u; \
        sl_u.value = (d);            \
        sl_u.parts.lsw = (v);        \
        (d) = sl_u.value;            \
    } while(0)

/* Get the less significant 32 bit int from a double.  */
#define GET_LOW_WORD(i, d)           \
    do                               \
    {                                \
        ieee_double_shape_type gl_u; \
        gl_u.value = (d);            \
        (i) = gl_u.parts.lsw;        \
    } while(0)

static inline KDfloat32 __kdCosdfKernel(KDfloat64KHR x)
{
    /* |cos(x) - c(x)| < 2**-34.1 (~[-5.37e-11, 5.295e-11]). */
    const KDfloat64KHR C0 = -4.9999999725103100e-01; /* -0x1ffffffd0c5e81.0p-54 */
    const KDfloat64KHR C1 = 4.1666623323739063e-02;  /*  0x155553e1053a42.0p-57 */
    const KDfloat64KHR C2 = -1.3886763774609929e-03; /* -0x16c087e80f1e27.0p-62 */
    const KDfloat64KHR C3 = 2.4390448796277409e-05;  /*  0x199342e0ee5069.0p-68 */

    /* Try to optimize for parallel evaluation. */
    KDfloat64KHR z = x * x;
    KDfloat64KHR w = z * z;
    KDfloat64KHR r = C2 + z * C3;
    return (KDfloat32)(((1.0 + z * C0) + w * C1) + (w * z) * r);
}

static inline KDfloat32 __kdSindfKernel(KDfloat64KHR x)
{
    /* |sin(x)/x - s(x)| < 2**-37.5 (~[-4.89e-12, 4.824e-12]). */
    const KDfloat64KHR S1 = -1.6666666641626524e-01; /* -0x15555554cbac77.0p-55 */
    const KDfloat64KHR S2 = 8.3333293858894632e-03;  /* 0x111110896efbb2.0p-59 */
    const KDfloat64KHR S3 = -1.9839334836096632e-04; /* -0x1a00f9e2cae774.0p-65 */
    const KDfloat64KHR S4 = 2.7183114939898219e-06;  /* 0x16cd878c3b46a7.0p-71 */

    /* Try to optimize for parallel evaluation. */
    KDfloat64KHR z = x * x;
    KDfloat64KHR w = z * z;
    KDfloat64KHR r = S3 + z * S4;
    KDfloat64KHR s = z * x;
    return (KDfloat32)((x + s * (S1 + z * S2)) + s * w * r);
}

static inline KDfloat32 __kdTandfKernel(KDfloat64KHR x, KDint iy)
{
    /* |tan(x)/x - t(x)| < 2**-25.5 (~[-2e-08, 2e-08]). */
    const KDfloat64KHR T[] = {
        3.3333139503079140e-01, /* 0x15554d3418c99f.0p-54 */
        1.3339200271297674e-01, /* 0x1112fd38999f72.0p-55 */
        5.3381237844567039e-02, /* 0x1b54c91d865afe.0p-57 */
        2.4528318116654728e-02, /* 0x191df3908c33ce.0p-58 */
        2.9743574335996730e-03, /* 0x185dadfcecf44e.0p-61 */
        9.4656478494367317e-03, /* 0x1362b9bf971bcd.0p-59 */
    };

    KDfloat64KHR z = x * x;
    /*
     * Split up the polynomial into small independent terms to give
     * opportunities for parallel evaluation.  The chosen splitting is
     * micro-optimized for Athlons (XP, X64).  It costs 2 multiplications
     * relative to Horner's method on sequential machines.
     *
     * We add the small terms from lowest degree up for efficiency on
     * non-sequential machines (the lowest degree terms tend to be ready
     * earlier).  Apart from this, we don't care about order of
     * operations, and don't need to to care since we have precision to
     * spare.  However, the chosen splitting is good for accuracy too,
     * and would give results as accurate as Horner's method if the
     * small terms were added from highest degree down.
     */
    KDfloat64KHR r = T[4] + z * T[5];
    KDfloat64KHR t = T[2] + z * T[3];
    KDfloat64KHR w = z * z;
    KDfloat64KHR s = z * x;
    KDfloat64KHR u = T[0] + z * T[1];
    r = (x + s * u) + (s * w) * (t + w * r);
    if(iy == 1)
    {
        return (KDfloat32)r;
    }
    else
    {
        return (KDfloat32)(-1.0 / r);
    }
}

/* __kdCosKernel
 * Algorithm
 *  1. Since cos(-x) = cos(x), we need only to consider positive x.
 *  2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
 *  3. cos(x) is approximated by a polynomial of degree 14 on
 *     [0,pi/4]
 *                           4            14
 *      cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
 *     where the remez error is
 *  
 *  |              2     4     6     8     10    12     14 |     -58
 *  |cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
 *  |                                      | 
 * 
 *                 4     6     8     10    12     14 
 *  4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
 *         cos(x) ~ 1 - x*x/2 + r
 *     since cos(x+y) ~ cos(x) - sin(x)*y 
 *            ~ cos(x) - x*y,
 *     a correction term is necessary in cos(x) and hence
 *      cos(x+y) = 1 - (x*x/2 - (r - x*y))
 *     For better accuracy, rearrange to
 *      cos(x+y) ~ w + (tmp + (r-x*y))
 *     where w = 1 - x*x/2 and tmp is a tiny correction term
 *     (1 - x*x/2 == w + tmp exactly in infinite precision).
 *     The exactness of w + tmp in infinite precision depends on w
 *     and tmp having the same precision as x.  If they have extra
 *     precision due to compiler bugs, then the extra precision is
 *     only good provided it is retained in all terms of the final
 *     expression for cos().  Retention happens in all cases tested
 *     under FreeBSD, so don't pessimize things by forcibly clipping
 *     any extra precision in w.
 */
static KDfloat64KHR __kdCosKernel(KDfloat64KHR x, KDfloat64KHR y)
{
    const KDfloat64KHR one = 1.00000000000000000000e+00; /* 0x3FF00000, 0x00000000 */
    const KDfloat64KHR C1 = 4.16666666666666019037e-02;  /* 0x3FA55555, 0x5555554C */
    const KDfloat64KHR C2 = -1.38888888888741095749e-03; /* 0xBF56C16C, 0x16C15177 */
    const KDfloat64KHR C3 = 2.48015872894767294178e-05;  /* 0x3EFA01A0, 0x19CB1590 */
    const KDfloat64KHR C4 = -2.75573143513906633035e-07; /* 0xBE927E4F, 0x809C52AD */
    const KDfloat64KHR C5 = 2.08757232129817482790e-09;  /* 0x3E21EE9E, 0xBDB4B1C4 */
    const KDfloat64KHR C6 = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */

    KDfloat64KHR z = x * x;
    KDfloat64KHR w = z * z;
    KDfloat64KHR r = z * (C1 + z * (C2 + z * C3)) + w * w * (C4 + z * (C5 + z * C6));
    KDfloat64KHR hz = 0.5 * z;
    w = one - hz;
    return w + (((one - w) - hz) + (z * r - x * y));
}

/* __kdSinKernel
 * Algorithm
 *  1. Since sin(-x) = -sin(x), we need only to consider positive x. 
 *  2. Callers must return sin(-0) = -0 without calling here since our
 *     odd polynomial is not evaluated in a way that preserves -0.
 *     Callers may do the optimization sin(x) ~ x for tiny x.
 *  3. sin(x) is approximated by a polynomial of degree 13 on
 *     [0,pi/4]
 *                   3            13
 *      sin(x) ~ x + S1*x + ... + S6*x
 *     where
 *  
 *  |sin(x)         2     4     6     8     10     12  |     -58
 *  |----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
 *  |  x                               | 
 * 
 *  4. sin(x+y) = sin(x) + sin'(x')*y
 *          ~ sin(x) + (1-x*x/2)*y
 *     For better accuracy, let 
 *           3      2      2      2      2
 *      r = x *(S2+x *(S3+x *(S4+x *(S5+x *S6))))
 *     then                   3    2
 *      sin(x) = x + (S1*x + (x *(r-y/2)+y))
 */
static KDfloat64KHR __kdSinKernel(KDfloat64KHR x, KDfloat64KHR y, KDint iy)
{
    const KDfloat64KHR half = 5.00000000000000000000e-01; /* 0x3FE00000, 0x00000000 */
    const KDfloat64KHR S1 = -1.66666666666666324348e-01;  /* 0xBFC55555, 0x55555549 */
    const KDfloat64KHR S2 = 8.33333333332248946124e-03;   /* 0x3F811111, 0x1110F8A6 */
    const KDfloat64KHR S3 = -1.98412698298579493134e-04;  /* 0xBF2A01A0, 0x19C161D5 */
    const KDfloat64KHR S4 = 2.75573137070700676789e-06;   /* 0x3EC71DE3, 0x57B1FE7D */
    const KDfloat64KHR S5 = -2.50507602534068634195e-08;  /* 0xBE5AE5E6, 0x8A2B9CEB */
    const KDfloat64KHR S6 = 1.58969099521155010221e-10;   /* 0x3DE5D93A, 0x5ACFD57C */

    KDfloat64KHR z = x * x;
    KDfloat64KHR w = z * z;
    KDfloat64KHR r = S2 + z * (S3 + z * S4) + z * w * (S5 + z * S6);
    KDfloat64KHR v = z * x;

    if(iy == 0)
    {
        return x + v * (S1 + z * r);
    }
    else
    {
        return x - ((z * (half * y - v * r) - y) - v * S1);
    }
}

/* __kdTanKernel
 * Algorithm:
 *  1. Since tan(-x) = -tan(x), we need only to consider positive x.
 *  2. Callers must return tan(-0) = -0 without calling here since our
 *     odd polynomial is not evaluated in a way that preserves -0.
 *     Callers may do the optimization tan(x) ~ x for tiny x.
 *  3. tan(x) is approximated by a odd polynomial of degree 27 on
 *     [0,0.67434]
 *                   3             27
 *      tan(x) ~ x + T1*x + ... + T13*x
 *     where
 *
 *          |tan(x)         2     4            26   |     -59.2
 *          |----- - (1+T1*x +T2*x +.... +T13*x    )| <= 2
 *          |  x                    |
 *
 *     Note: tan(x+y) = tan(x) + tan'(x)*y
 *                ~ tan(x) + (1+x*x)*y
 *     Therefore, for better accuracy in computing tan(x+y), let
 *           3      2      2       2       2
 *      r = x *(T2+x *(T3+x *(...+x *(T12+x *T13))))
 *     then
 *                  3    2
 *      tan(x+y) = x + (T1*x + (x *(r+y)+y))
 *
 *      4. For x in [0.67434,pi/4],  let y = pi/4 - x, then
 *      tan(x) = tan(pi/4-y) = (1-tan(y))/(1+tan(y))
 *             = 1 - 2*(tan(y) - (tan(y)^2)/(1+tan(y)))
 */
static KDfloat64KHR __kdTanKernel(KDfloat64KHR x, KDfloat64KHR y, KDint iy)
{
    const KDfloat64KHR T[] = {
        3.33333333333334091986e-01,  /* 3FD55555, 55555563 */
        1.33333333333201242699e-01,  /* 3FC11111, 1110FE7A */
        5.39682539762260521377e-02,  /* 3FABA1BA, 1BB341FE */
        2.18694882948595424599e-02,  /* 3F9664F4, 8406D637 */
        8.86323982359930005737e-03,  /* 3F8226E3, E96E8493 */
        3.59207910759131235356e-03,  /* 3F6D6D22, C9560328 */
        1.45620945432529025516e-03,  /* 3F57DBC8, FEE08315 */
        5.88041240820264096874e-04,  /* 3F4344D8, F2F26501 */
        2.46463134818469906812e-04,  /* 3F3026F7, 1A8D1068 */
        7.81794442939557092300e-05,  /* 3F147E88, A03792A6 */
        7.14072491382608190305e-05,  /* 3F12B80F, 32F0A7E9 */
        -1.85586374855275456654e-05, /* BEF375CB, DB605373 */
        2.59073051863633712884e-05,  /* 3EFB2A70, 74BF7AD4 */
    };

    const KDfloat64KHR pio4 = 7.85398163397448278999e-01;   /* 3FE921FB, 54442D18 */
    const KDfloat64KHR pio4lo = 3.06161699786838301793e-17; /* 3C81A626, 33145C07 */

    KDfloat64KHR z = 0.0;
    KDfloat64KHR r = 0.0;
    KDfloat64KHR v = 0.0;
    KDfloat64KHR w = 0.0;
    KDfloat64KHR s = 0.0;
    KDint32 hx = 0;

    GET_HIGH_WORD(hx, x);
    KDint32 ix = hx & KDINT32_MAX; /* high word of |x| */
    if(ix >= 0x3FE59428)
    { /* |x| >= 0.6744 */
        if(hx < 0)
        {
            x = -x;
            y = -y;
        }
        z = pio4 - x;
        w = pio4lo - y;
        x = z + w;
        y = 0.0;
    }
    z = x * x;
    w = z * z;
    /*
     * Break x^5*(T[1]+x^2*T[2]+...) into
     * x^5(T[1]+x^4*T[3]+...+x^20*T[11]) +
     * x^5(x^2*(T[2]+x^4*T[4]+...+x^22*[T12]))
     */
    r = T[1] + w * (T[3] + w * (T[5] + w * (T[7] + w * (T[9] + w * T[11]))));
    v = z * (T[2] + w * (T[4] + w * (T[6] + w * (T[8] + w * (T[10] + w * T[12])))));
    s = z * x;
    r = y + z * (s * (r + v) + y);
    r += T[0] * s;
    w = x + r;
    if(ix >= 0x3FE59428)
    {
        v = (KDfloat64KHR)iy;
        return (KDfloat64KHR)(1 - ((hx >> 30) & 2)) *
            (v - 2.0 * (x - (w * w / (w + v) - r)));
    }
    if(iy == 1)
    {
        return w;
    }
    else
    {
        /*
         * if allow error up to 2 ulp, simply return
         * -1.0 / (x+r) here
         */
        /* compute -1.0 / (x+r) accurately */
        z = w;
        SET_LOW_WORD(z, 0);
        v = r - (z - x);           /* z+v = r+x */
        KDfloat64KHR a = -1.0 / w; /* a = -1.0/w */
        KDfloat64KHR t = a;
        SET_LOW_WORD(t, 0);
        s = 1.0 + t * z;
        return t + a * (s + t * v);
    }
}

static KDfloat32 __kdCopysignf(KDfloat32 x, KDfloat32 y)
{
    __KDFloatShape shape_x = {x};
    __KDFloatShape shape_y = {y};
    shape_x.u32 = (shape_x.u32 & KDINT32_MAX) | (shape_y.u32 & KDINT32_MIN);
    return shape_x.f32;
}

static KDfloat64KHR __kdCopysign(KDfloat64KHR x, KDfloat64KHR y)
{
    __KDFloat64Shape shape_x = {x};
    __KDFloat64Shape shape_y = {y};
    shape_x.u64 = (shape_x.u64 & KDINT64_MAX) | (shape_y.u64 & KDINT64_MIN);
    return shape_x.f64;
}

static KDfloat32 __kdScalbnf(KDfloat32 x, KDint n)
{
    static const KDfloat32 two25 = 3.355443200e+07f;   /* 0x4c000000 */
    static const KDfloat32 twom25 = 2.9802322388e-08f; /* 0x33000000 */
    static const KDfloat32 huge = 1.0e+30f;
    static const KDfloat32 tiny = 1.0e-30f;

    __KDFloatShape shape_x = {x};
    KDint32 k = (shape_x.u32 & 0x7f800000) >> 23; /* extract exponent */
    if(k == 0)
    { /* 0 or subnormal x */
        if((shape_x.u32 & KDINT32_MAX) == 0)
        {
            return shape_x.f32;
        } /* +-0 */
        shape_x.f32 *= two25;
        k = ((shape_x.u32 & 0x7f800000) >> 23) - 25;
        if(n < -50000)
        {
            return tiny * shape_x.f32; /*underflow*/
        }
    }
    if(k == 0xff)
    {
        return shape_x.f32 + shape_x.f32;
    } /* NaN or Inf */
    k = k + n;
    if(k > 0xfe)
    {
        return huge * __kdCopysignf(huge, shape_x.f32); /* overflow  */
    }
    if(k > 0) /* normal result */
    {
        shape_x.u32 = (shape_x.u32 & 0x807fffff) | ((KDuint32)k << 23);
        return shape_x.f32;
    }
    if(k <= -25)
    {
        if(n > 50000) /* in case integer overflow in n+k */
        {
            return huge * __kdCopysignf(huge, shape_x.f32); /*overflow*/
        }
        else
        {
            return tiny * __kdCopysignf(tiny, shape_x.f32); /*underflow*/
        }
    }
    k += 25; /* subnormal result */
    shape_x.u32 = (shape_x.u32 & 0x807fffff) | ((KDuint32)k << 23);
    return shape_x.f32 * twom25;
}

static KDfloat64KHR __kdScalbn(KDfloat64KHR x, KDint n)
{
    static const KDfloat64KHR two54 = 1.80143985094819840000e+16;  /* 0x43500000, 0x00000000 */
    static const KDfloat64KHR twom54 = 5.55111512312578270212e-17; /* 0x3C900000, 0x00000000 */
    static const KDfloat64KHR huge = 1.0e+300;
    static const KDfloat64KHR tiny = 1.0e-300;

    KDint32 hx = 0;
    KDint32 lx = 0;
    EXTRACT_WORDS(hx, lx, x);
    KDint32 k = (hx & 0x7ff00000) >> 20; /* extract exponent */
    if(k == 0)
    { /* 0 or subnormal x */
        if((lx | (hx & KDINT32_MAX)) == 0)
        {
            return x;
        } /* +-0 */
        x *= two54;
        GET_HIGH_WORD(hx, x);
        k = ((hx & 0x7ff00000) >> 20) - 54;
        if(n < -50000)
        {
            return tiny * x; /*underflow*/
        }
    }
    if(k == 0x7ff)
    {
        return x + x;
    } /* NaN or Inf */
    k = k + n;
    if(k > 0x7fe)
    {
        return huge * __kdCopysign(huge, x); /* overflow  */
    }
    if(k > 0) /* normal result */
    {
        SET_HIGH_WORD(x, ((KDuint32)hx & 0x800fffff) | ((KDuint32)k << 20));
        return x;
    }
    if(k <= -54)
    {
        if(n > 50000) /* in case integer overflow in n+k */
        {
            return huge * __kdCopysign(huge, x); /*overflow*/
        }
        else
        {
            return tiny * __kdCopysign(tiny, x); /*underflow*/
        }
    }
    k += 54; /* subnormal result */
    SET_HIGH_WORD(x, ((KDuint32)hx & 0x800fffff) | ((KDuint32)k << 20));
    return x * twom54;
}

static KDint __kdIrint(KDfloat64KHR x)
{
#ifdef __SSE2__
    return _mm_cvtsd_si32(_mm_load_sd(&x));
#else
    if(x >= 0)
    {
        return (KDint)(x + 0.5);
    }
    return (KDint)(x - 0.5);
#endif
}

/*
 * Table of constants for 2/pi, 396 Hex digits (476 decimal) of 2/pi
 *
 *      integer array, contains the (24*i)-th to (24*i+23)-th
 *      bit of 2/pi after binary point. The corresponding
 *      floating value is
 *
 *          ipio2[i] * 2^(-24(i+1)).
 *
 * NB: This table must have at least (e0-3)/24 + jk terms.
 *     For quad precision (e0 <= 16360, jk = 6), this is 686.
 */
static const KDint32 ipio2[] = {
    0xA2F983,
    0x6E4E44,
    0x1529FC,
    0x2757D1,
    0xF534DD,
    0xC0DB62,
    0x95993C,
    0x439041,
    0xFE5163,
    0xABDEBB,
    0xC561B7,
    0x246E3A,
    0x424DD2,
    0xE00649,
    0x2EEA09,
    0xD1921C,
    0xFE1DEB,
    0x1CB129,
    0xA73EE8,
    0x8235F5,
    0x2EBB44,
    0x84E99C,
    0x7026B4,
    0x5F7E41,
    0x3991D6,
    0x398353,
    0x39F49C,
    0x845F8B,
    0xBDF928,
    0x3B1FF8,
    0x97FFDE,
    0x05980F,
    0xEF2F11,
    0x8B5A0A,
    0x6D1F6D,
    0x367ECF,
    0x27CB09,
    0xB74F46,
    0x3F669E,
    0x5FEA2D,
    0x7527BA,
    0xC7EBE5,
    0xF17B3D,
    0x0739F7,
    0x8A5292,
    0xEA6BFB,
    0x5FB11F,
    0x8D5D08,
    0x560330,
    0x46FC7B,
    0x6BABF0,
    0xCFBC20,
    0x9AF436,
    0x1DA9E3,
    0x91615E,
    0xE61B08,
    0x659985,
    0x5F14A0,
    0x68408D,
    0xFFD880,
    0x4D7327,
    0x310606,
    0x1556CA,
    0x73A8C9,
    0x60E27B,
    0xC08C6B,
#if LDBL_MAX_EXP > 1024
#if LDBL_MAX_EXP > 16384
#error "ipio2 table needs to be expanded"
#endif
    0x47C419,
    0xC367CD,
    0xDCE809,
    0x2A8359,
    0xC4768B,
    0x961CA6,
    0xDDAF44,
    0xD15719,
    0x053EA5,
    0xFF0705,
    0x3F7E33,
    0xE832C2,
    0xDE4F98,
    0x327DBB,
    0xC33D26,
    0xEF6B1E,
    0x5EF89F,
    0x3A1F35,
    0xCAF27F,
    0x1D87F1,
    0x21907C,
    0x7C246A,
    0xFA6ED5,
    0x772D30,
    0x433B15,
    0xC614B5,
    0x9D19C3,
    0xC2C4AD,
    0x414D2C,
    0x5D000C,
    0x467D86,
    0x2D71E3,
    0x9AC69B,
    0x006233,
    0x7CD2B4,
    0x97A7B4,
    0xD55537,
    0xF63ED7,
    0x1810A3,
    0xFC764D,
    0x2A9D64,
    0xABD770,
    0xF87C63,
    0x57B07A,
    0xE71517,
    0x5649C0,
    0xD9D63B,
    0x3884A7,
    0xCB2324,
    0x778AD6,
    0x23545A,
    0xB91F00,
    0x1B0AF1,
    0xDFCE19,
    0xFF319F,
    0x6A1E66,
    0x615799,
    0x47FBAC,
    0xD87F7E,
    0xB76522,
    0x89E832,
    0x60BFE6,
    0xCDC4EF,
    0x09366C,
    0xD43F5D,
    0xD7DE16,
    0xDE3B58,
    0x929BDE,
    0x2822D2,
    0xE88628,
    0x4D58E2,
    0x32CAC6,
    0x16E308,
    0xCB7DE0,
    0x50C017,
    0xA71DF3,
    0x5BE018,
    0x34132E,
    0x621283,
    0x014883,
    0x5B8EF5,
    0x7FB0AD,
    0xF2E91E,
    0x434A48,
    0xD36710,
    0xD8DDAA,
    0x425FAE,
    0xCE616A,
    0xA4280A,
    0xB499D3,
    0xF2A606,
    0x7F775C,
    0x83C2A3,
    0x883C61,
    0x78738A,
    0x5A8CAF,
    0xBDD76F,
    0x63A62D,
    0xCBBFF4,
    0xEF818D,
    0x67C126,
    0x45CA55,
    0x36D9CA,
    0xD2A828,
    0x8D61C2,
    0x77C912,
    0x142604,
    0x9B4612,
    0xC459C4,
    0x44C5C8,
    0x91B24D,
    0xF31700,
    0xAD43D4,
    0xE54929,
    0x10D5FD,
    0xFCBE00,
    0xCC941E,
    0xEECE70,
    0xF53E13,
    0x80F1EC,
    0xC3E7B3,
    0x28F8C7,
    0x940593,
    0x3E71C1,
    0xB3092E,
    0xF3450B,
    0x9C1288,
    0x7B20AB,
    0x9FB52E,
    0xC29247,
    0x2F327B,
    0x6D550C,
    0x90A772,
    0x1FE76B,
    0x96CB31,
    0x4A1679,
    0xE27941,
    0x89DFF4,
    0x9794E8,
    0x84E6E2,
    0x973199,
    0x6BED88,
    0x365F5F,
    0x0EFDBB,
    0xB49A48,
    0x6CA467,
    0x427271,
    0x325D8D,
    0xB8159F,
    0x09E5BC,
    0x25318D,
    0x3974F7,
    0x1C0530,
    0x010C0D,
    0x68084B,
    0x58EE2C,
    0x90AA47,
    0x02E774,
    0x24D6BD,
    0xA67DF7,
    0x72486E,
    0xEF169F,
    0xA6948E,
    0xF691B4,
    0x5153D1,
    0xF20ACF,
    0x339820,
    0x7E4BF5,
    0x6863B2,
    0x5F3EDD,
    0x035D40,
    0x7F8985,
    0x295255,
    0xC06437,
    0x10D86D,
    0x324832,
    0x754C5B,
    0xD4714E,
    0x6E5445,
    0xC1090B,
    0x69F52A,
    0xD56614,
    0x9D0727,
    0x50045D,
    0xDB3BB4,
    0xC576EA,
    0x17F987,
    0x7D6B49,
    0xBA271D,
    0x296996,
    0xACCCC6,
    0x5414AD,
    0x6AE290,
    0x89D988,
    0x50722C,
    0xBEA404,
    0x940777,
    0x7030F3,
    0x27FC00,
    0xA871EA,
    0x49C266,
    0x3DE064,
    0x83DD97,
    0x973FA3,
    0xFD9443,
    0x8C860D,
    0xDE4131,
    0x9D3992,
    0x8C70DD,
    0xE7B717,
    0x3BDF08,
    0x2B3715,
    0xA0805C,
    0x93805A,
    0x921110,
    0xD8E80F,
    0xAF806C,
    0x4BFFDB,
    0x0F9038,
    0x761859,
    0x15A562,
    0xBBCB61,
    0xB989C7,
    0xBD4010,
    0x04F2D2,
    0x277549,
    0xF6B6EB,
    0xBB22DB,
    0xAA140A,
    0x2F2689,
    0x768364,
    0x333B09,
    0x1A940E,
    0xAA3A51,
    0xC2A31D,
    0xAEEDAF,
    0x12265C,
    0x4DC26D,
    0x9C7A2D,
    0x9756C0,
    0x833F03,
    0xF6F009,
    0x8C402B,
    0x99316D,
    0x07B439,
    0x15200C,
    0x5BC3D8,
    0xC492F5,
    0x4BADC6,
    0xA5CA4E,
    0xCD37A7,
    0x36A9E6,
    0x9492AB,
    0x6842DD,
    0xDE6319,
    0xEF8C76,
    0x528B68,
    0x37DBFC,
    0xABA1AE,
    0x3115DF,
    0xA1AE00,
    0xDAFB0C,
    0x664D64,
    0xB705ED,
    0x306529,
    0xBF5657,
    0x3AFF47,
    0xB9F96A,
    0xF3BE75,
    0xDF9328,
    0x3080AB,
    0xF68C66,
    0x15CB04,
    0x0622FA,
    0x1DE4D9,
    0xA4B33D,
    0x8F1B57,
    0x09CD36,
    0xE9424E,
    0xA4BE13,
    0xB52333,
    0x1AAAF0,
    0xA8654F,
    0xA5C1D2,
    0x0F3F0B,
    0xCD785B,
    0x76F923,
    0x048B7B,
    0x721789,
    0x53A6C6,
    0xE26E6F,
    0x00EBEF,
    0x584A9B,
    0xB7DAC4,
    0xBA66AA,
    0xCFCF76,
    0x1D02D1,
    0x2DF1B1,
    0xC1998C,
    0x77ADC3,
    0xDA4886,
    0xA05DF7,
    0xF480C6,
    0x2FF0AC,
    0x9AECDD,
    0xBC5C3F,
    0x6DDED0,
    0x1FC790,
    0xB6DB2A,
    0x3A25A3,
    0x9AAF00,
    0x9353AD,
    0x0457B6,
    0xB42D29,
    0x7E804B,
    0xA707DA,
    0x0EAA76,
    0xA1597B,
    0x2A1216,
    0x2DB7DC,
    0xFDE5FA,
    0xFEDB89,
    0xFDBE89,
    0x6C76E4,
    0xFCA906,
    0x70803E,
    0x156E85,
    0xFF87FD,
    0x073E28,
    0x336761,
    0x86182A,
    0xEABD4D,
    0xAFE7B3,
    0x6E6D8F,
    0x396795,
    0x5BBF31,
    0x48D784,
    0x16DF30,
    0x432DC7,
    0x356125,
    0xCE70C9,
    0xB8CB30,
    0xFD6CBF,
    0xA200A4,
    0xE46C05,
    0xA0DD5A,
    0x476F21,
    0xD21262,
    0x845CB9,
    0x496170,
    0xE0566B,
    0x015299,
    0x375550,
    0xB7D51E,
    0xC4F133,
    0x5F6E13,
    0xE4305D,
    0xA92E85,
    0xC3B21D,
    0x3632A1,
    0xA4B708,
    0xD4B1EA,
    0x21F716,
    0xE4698F,
    0x77FF27,
    0x80030C,
    0x2D408D,
    0xA0CD4F,
    0x99A520,
    0xD3A2B3,
    0x0A5D2F,
    0x42F9B4,
    0xCBDA11,
    0xD0BE7D,
    0xC1DB9B,
    0xBD17AB,
    0x81A2CA,
    0x5C6A08,
    0x17552E,
    0x550027,
    0xF0147F,
    0x8607E1,
    0x640B14,
    0x8D4196,
    0xDEBE87,
    0x2AFDDA,
    0xB6256B,
    0x34897B,
    0xFEF305,
    0x9EBFB9,
    0x4F6A68,
    0xA82A4A,
    0x5AC44F,
    0xBCF82D,
    0x985AD7,
    0x95C7F4,
    0x8D4D0D,
    0xA63A20,
    0x5F57A4,
    0xB13F14,
    0x953880,
    0x0120CC,
    0x86DD71,
    0xB6DEC9,
    0xF560BF,
    0x11654D,
    0x6B0701,
    0xACB08C,
    0xD0C0B2,
    0x485551,
    0x0EFB1E,
    0xC37295,
    0x3B06A3,
    0x3540C0,
    0x7BDC06,
    0xCC45E0,
    0xFA294E,
    0xC8CAD6,
    0x41F3E8,
    0xDE647C,
    0xD8649B,
    0x31BED9,
    0xC397A4,
    0xD45877,
    0xC5E369,
    0x13DAF0,
    0x3C3ABA,
    0x461846,
    0x5F7555,
    0xF5BDD2,
    0xC6926E,
    0x5D2EAC,
    0xED440E,
    0x423E1C,
    0x87C461,
    0xE9FD29,
    0xF3D6E7,
    0xCA7C22,
    0x35916F,
    0xC5E008,
    0x8DD7FF,
    0xE26A6E,
    0xC6FDB0,
    0xC10893,
    0x745D7C,
    0xB2AD6B,
    0x9D6ECD,
    0x7B723E,
    0x6A11C6,
    0xA9CFF7,
    0xDF7329,
    0xBAC9B5,
    0x5100B7,
    0x0DB2E2,
    0x24BA74,
    0x607DE5,
    0x8AD874,
    0x2C150D,
    0x0C1881,
    0x94667E,
    0x162901,
    0x767A9F,
    0xBEFDFD,
    0xEF4556,
    0x367ED9,
    0x13D9EC,
    0xB9BA8B,
    0xFC97C4,
    0x27A831,
    0xC36EF1,
    0x36C594,
    0x56A8D8,
    0xB5A8B4,
    0x0ECCCF,
    0x2D8912,
    0x34576F,
    0x89562C,
    0xE3CE99,
    0xB920D6,
    0xAA5E6B,
    0x9C2A3E,
    0xCC5F11,
    0x4A0BFD,
    0xFBF4E1,
    0x6D3B8E,
    0x2C86E2,
    0x84D4E9,
    0xA9B4FC,
    0xD1EEEF,
    0xC9352E,
    0x61392F,
    0x442138,
    0xC8D91B,
    0x0AFC81,
    0x6A4AFB,
    0xD81C2F,
    0x84B453,
    0x8C994E,
    0xCC2254,
    0xDC552A,
    0xD6C6C0,
    0x96190B,
    0xB8701A,
    0x649569,
    0x605A26,
    0xEE523F,
    0x0F117F,
    0x11B5F4,
    0xF5CBFC,
    0x2DBC34,
    0xEEBC34,
    0xCC5DE8,
    0x605EDD,
    0x9B8E67,
    0xEF3392,
    0xB817C9,
    0x9B5861,
    0xBC57E1,
    0xC68351,
    0x103ED8,
    0x4871DD,
    0xDD1C2D,
    0xA118AF,
    0x462C21,
    0xD7F359,
    0x987AD9,
    0xC0549E,
    0xFA864F,
    0xFC0656,
    0xAE79E5,
    0x362289,
    0x22AD38,
    0xDC9367,
    0xAAE855,
    0x382682,
    0x9BE7CA,
    0xA40D51,
    0xB13399,
    0x0ED7A9,
    0x480569,
    0xF0B265,
    0xA7887F,
    0x974C88,
    0x36D1F9,
    0xB39221,
    0x4A827B,
    0x21CF98,
    0xDC9F40,
    0x5547DC,
    0x3A74E1,
    0x42EB67,
    0xDF9DFE,
    0x5FD45E,
    0xA4677B,
    0x7AACBA,
    0xA2F655,
    0x23882B,
    0x55BA41,
    0x086E59,
    0x862A21,
    0x834739,
    0xE6E389,
    0xD49EE5,
    0x40FB49,
    0xE956FF,
    0xCA0F1C,
    0x8A59C5,
    0x2BFA94,
    0xC5C1D3,
    0xCFC50F,
    0xAE5ADB,
    0x86C547,
    0x624385,
    0x3B8621,
    0x94792C,
    0x876110,
    0x7B4C2A,
    0x1A2C80,
    0x12BF43,
    0x902688,
    0x893C78,
    0xE4C4A8,
    0x7BDBE5,
    0xC23AC4,
    0xEAF426,
    0x8A67F7,
    0xBF920D,
    0x2BA365,
    0xB1933D,
    0x0B7CBD,
    0xDC51A4,
    0x63DD27,
    0xDDE169,
    0x19949A,
    0x9529A8,
    0x28CE68,
    0xB4ED09,
    0x209F44,
    0xCA984E,
    0x638270,
    0x237C7E,
    0x32B90F,
    0x8EF5A7,
    0xE75614,
    0x08F121,
    0x2A9DB5,
    0x4D7E6F,
    0x5119A5,
    0xABF9B5,
    0xD6DF82,
    0x61DD96,
    0x023616,
    0x9F3AC4,
    0xA1A283,
    0x6DED72,
    0x7A8D39,
    0xA9B882,
    0x5C326B,
    0x5B2746,
    0xED3400,
    0x7700D2,
    0x55F4FC,
    0x4D5901,
    0x8071E0,
#endif
};

static KDint __kdRemPio2Kernel(const KDfloat64KHR *x, KDfloat64KHR *y, KDint e0, KDint nx)
{
    const KDfloat64KHR PIo2[] = {
        1.57079625129699707031e+00, /* 0x3FF921FB, 0x40000000 */
        7.54978941586159635335e-08, /* 0x3E74442D, 0x00000000 */
        5.39030252995776476554e-15, /* 0x3CF84698, 0x80000000 */
        3.28200341580791294123e-22, /* 0x3B78CC51, 0x60000000 */
        1.27065575308067607349e-29, /* 0x39F01B83, 0x80000000 */
        1.22933308981111328932e-36, /* 0x387A2520, 0x40000000 */
        2.73370053816464559624e-44, /* 0x36E38222, 0x80000000 */
        2.16741683877804819444e-51, /* 0x3569F31D, 0x00000000 */
    };


    const KDfloat64KHR two24 = 1.67772160000000000000e+07;  /* 0x41700000, 0x00000000 */
    const KDfloat64KHR twon24 = 5.96046447753906250000e-08; /* 0x3E700000, 0x00000000 */

    KDint32 n = 0;
    KDint32 ih = 0;
    KDint32 iq[20];
    KDfloat64KHR z = 0.0;
    KDfloat64KHR fw = 0.0;
    KDfloat64KHR f[20];
    KDfloat64KHR fq[20];
    KDfloat64KHR q[20];
    kdMemset(f, 0, sizeof(f));

    /* initialize jk*/
    KDint32 jk = 3;

    /* determine jx,jv,q0, note that 3>q0 */
    KDint32 jx = nx - 1;
    KDint32 jv = (e0 - 3) / 24;
    if(jv < 0)
    {
        jv = 0;
    }
    KDint32 q0 = e0 - 24 * (jv + 1);

    /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
    KDint32 j = jv - jx;
    KDint32 m = jx + jk;
    for(KDint32 i = 0; i <= m; i++, j++)
    {
        f[i] = (j < 0) ? 0.0 : (KDfloat64KHR)ipio2[j];
    }

    /* compute q[0],q[1],...q[jk] */
    for(KDint32 i = 0; i <= jk; i++)
    {
        fw = 0.0;
        for(KDint32 l = 0; l <= jx; l++)
        {
            fw += x[l] * f[jx + i - l];
        }
        q[i] = fw;
    }

    KDint32 jz = jk;

    KDboolean recompute = KD_FALSE;
    do
    {
        /* distill q[] into iq[] reversingly */
        z = q[jz];
        j = jz;
        for(KDint32 i = 0; j > 0; i++, j--)
        {
            fw = (KDfloat64KHR)((KDint32)(twon24 * z));
            iq[i] = (KDint32)(z - two24 * fw);
            z = q[j - 1] + fw;
        }
        /* compute n */
        z = __kdScalbn(z, q0);            /* actual value of z */
        z -= 8.0 * kdFloorKHR(z * 0.125); /* trim off integer >= 8 */
        n = (KDint32)z;
        z -= (KDfloat64KHR)n;
        ih = 0;
        if(q0 > 0)
        { /* need iq[jz-1] to determine n */
            KDint32 l = (iq[jz - 1] >> (24 - q0));
            n += l;
            iq[jz - 1] -= l << (24 - q0);
            ih = iq[jz - 1] >> (23 - q0);
        }
        else if(q0 == 0)
        {
            ih = iq[jz - 1] >> 23;
        }
        else if(z >= 0.5)
        {
            ih = 2;
        }
        if(ih > 0)
        { /* q > 0.5 */
            n += 1;
            KDint32 carry = 0;
            for(KDint32 i = 0; i < jz; i++)
            { /* compute 1-q */
                j = iq[i];
                if(carry == 0)
                {
                    if(j != 0)
                    {
                        carry = 1;
                        iq[i] = 0x1000000 - j;
                    }
                }
                else
                {
                    iq[i] = 0xffffff - j;
                }
            }
            if(q0 > 0)
            { /* rare case: chance is 1 in 12 */
                switch(q0)
                {
                    case 1:
                    {
                        iq[jz - 1] &= 0x7fffff;
                        break;
                    }
                    case 2:
                    {
                        iq[jz - 1] &= 0x3fffff;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            if(ih == 2)
            {
                z = 1.0 - z;
                if(carry != 0)
                {
                    z -= __kdScalbn(1.0, q0);
                }
            }
        }
        /* check if recomputation is needed */
        if(z == 0.0)
        {
            j = 0;
            for(KDint32 i = jz - 1; i >= jk; i--)
            {
                j |= iq[i];
            }
            if(j == 0)
            { /* need recomputation */
                KDint32 k = 0;
                for(k = 1; iq[jk - k] == 0; k++)
                {
                } /* k = no. of terms needed */
                for(KDint32 i = jz + 1; i <= jz + k; i++)
                { /* add q[jz+1] to q[jz+k] */
                    f[jx + i] = (KDfloat64KHR)ipio2[jv + i];
                    for(j = 0, fw = 0.0; j <= jx; j++)
                    {
                        fw += x[j] * f[jx + i - j];
                    }
                    q[i] = fw;
                }
                jz += k;
                recompute = KD_TRUE;
            }
        }
    } while(recompute);

    /* chop off zero terms */
    if(z == 0.0)
    {
        jz -= 1;
        q0 -= 24;
        while(iq[jz] == 0)
        {
            jz--;
            q0 -= 24;
        }
    }
    else
    { /* break z into 24-bit if necessary */
        z = __kdScalbn(z, -q0);
        if(z >= two24)
        {
            fw = (KDfloat64KHR)((KDint32)(twon24 * z));
            iq[jz] = (KDint32)(z - two24 * fw);
            jz += 1;
            q0 += 24;
            iq[jz] = (KDint32)fw;
        }
        else
        {
            iq[jz] = (KDint32)z;
        }
    }
    /* convert integer "bit" chunk to floating-point value */
    fw = __kdScalbn(1.0, q0);
    for(KDint32 i = jz; i >= 0; i--)
    {
        q[i] = fw * (KDfloat64KHR)iq[i];
        fw *= twon24;
    }
    /* compute PIo2[0,...,jk]*q[jz,...,0] */
    for(KDint32 i = jz; i >= 0; i--)
    {
        KDint32 k = 0;
        for(fw = 0.0, k = 0; k <= jk && k <= jz - i; k++)
        {
            fw += PIo2[k] * q[i + k];
        }
        fq[jz - i] = fw;
    }
    /* compress fq[] into y[] */
    fw = 0.0;
    for(KDint32 i = jz; i >= 0; i--)
    {
        fw += fq[i];
    }
    y[0] = (ih == 0) ? fw : -fw;
    return n & 7;
}

static inline KDint __kdRemPio2f(KDfloat32 x, KDfloat64KHR *y)
{
    const KDfloat64KHR pio2_1 = 1.57079631090164184570e+00;  /* 0x3FF921FB, 0x50000000 */
    const KDfloat64KHR pio2_1t = 1.58932547735281966916e-08; /* 0x3E5110b4, 0x611A6263 */


    __KDFloatShape shape_x = {x};
    KDfloat64KHR tx[1];
    KDfloat64KHR ty[1];
    KDint32 n = 0;
    KDint32 ix = shape_x.u32 & KDINT32_MAX;
    KDboolean sign = shape_x.u32 >> 31;

    /* 33+53 bit pi is good enough for medium size */
    if(ix < 0x4dc90fdb)
    { /* |x| ~< 2^28*(pi/2), medium size */
        /* Use a specialized rint() to get fn.  Assume round-to-nearest. */
        KDfloat64KHR fn = (KDfloat64KHR)shape_x.f32 * KD_2_PI_KHR + 6.7553994410557440e+15;
        fn = fn - 6.7553994410557440e+15;
        n = __kdIrint(fn);
        *y = (KDfloat64KHR)shape_x.f32 - fn * pio2_1 - fn * pio2_1t;
        return n;
    }
    /*
     * all other (large) arguments
     */
    if(ix >= 0x7f800000)
    { /* x is inf or NaN */
        *y = (KDfloat64KHR)(shape_x.f32 - shape_x.f32);
        return 0;
    }

    KDint32 e0 = (ix >> 23) - 150; /* e0 = ilogb(|x|)-23; */
    shape_x.u32 = (KDuint32)(ix - (e0 << 23));
    tx[0] = (KDfloat64KHR)shape_x.f32;
    n = __kdRemPio2Kernel(tx, ty, e0, 1);
    if(sign)
    {
        *y = -ty[0];
        return -n;
    }
    *y = ty[0];
    return n;
}

static KDint __kdRemPio2(KDfloat64KHR x, KDfloat64KHR *y)
{
    const KDfloat64KHR zero = 0.00000000000000000000e+00;    /* 0x00000000, 0x00000000 */
    const KDfloat64KHR two24 = 1.67772160000000000000e+07;   /* 0x41700000, 0x00000000 */
    const KDfloat64KHR invpio2 = 6.36619772367581382433e-01; /* 0x3FE45F30, 0x6DC9C883 */
    const KDfloat64KHR pio2_1 = 1.57079632673412561417e+00;  /* 0x3FF921FB, 0x54400000 */
    const KDfloat64KHR pio2_1t = 6.07710050650619224932e-11; /* 0x3DD0B461, 0x1A626331 */
    const KDfloat64KHR pio2_2 = 6.07710050630396597660e-11;  /* 0x3DD0B461, 0x1A600000 */
    const KDfloat64KHR pio2_2t = 2.02226624879595063154e-21; /* 0x3BA3198A, 0x2E037073 */
    const KDfloat64KHR pio2_3 = 2.02226624871116645580e-21;  /* 0x3BA3198A, 0x2E000000 */
    const KDfloat64KHR pio2_3t = 8.47842766036889956997e-32; /* 0x397B839A, 0x252049C1 */

    KDfloat64KHR z;
    KDfloat64KHR tx[3], ty[2];
    KDint32 e0, i, nx, n, ix, hx;
    KDuint32 low;
    KDboolean medium = KD_FALSE;

    GET_HIGH_WORD(hx, x); /* high word of x */
    ix = hx & KDINT32_MAX;
    if(ix <= 0x400f6a7a)
    {                                 /* |x| ~<= 5pi/4 */
        if((ix & 0xfffff) == 0x921fb) /* |x| ~= pi/2 or 2pi/2 */
        {
            medium = KD_TRUE; /* cancellation -- use medium case */
        }
        else
        {
            if(ix <= 0x4002d97c)
            { /* |x| ~<= 3pi/4 */
                if(hx > 0)
                {
                    z = x - pio2_1; /* one round good to 85 bits */
                    y[0] = z - pio2_1t;
                    y[1] = (z - y[0]) - pio2_1t;
                    return 1;
                }
                else
                {
                    z = x + pio2_1;
                    y[0] = z + pio2_1t;
                    y[1] = (z - y[0]) + pio2_1t;
                    return -1;
                }
            }
            else
            {
                if(hx > 0)
                {
                    z = x - 2 * pio2_1;
                    y[0] = z - 2 * pio2_1t;
                    y[1] = (z - y[0]) - 2 * pio2_1t;
                    return 2;
                }
                else
                {
                    z = x + 2 * pio2_1;
                    y[0] = z + 2 * pio2_1t;
                    y[1] = (z - y[0]) + 2 * pio2_1t;
                    return -2;
                }
            }
        }
    }
    if(ix <= 0x401c463b && !medium)
    { /* |x| ~<= 9pi/4 */
        if(ix <= 0x4015fdbc)
        {                        /* |x| ~<= 7pi/4 */
            if(ix == 0x4012d97c) /* |x| ~= 3pi/2 */
            {
                medium = KD_TRUE;
            }
            else
            {
                if(hx > 0)
                {
                    z = x - 3 * pio2_1;
                    y[0] = z - 3 * pio2_1t;
                    y[1] = (z - y[0]) - 3 * pio2_1t;
                    return 3;
                }
                else
                {
                    z = x + 3 * pio2_1;
                    y[0] = z + 3 * pio2_1t;
                    y[1] = (z - y[0]) + 3 * pio2_1t;
                    return -3;
                }
            }
        }
        else
        {
            if(ix == 0x401921fb) /* |x| ~= 4pi/2 */
            {
                medium = KD_TRUE;
            }
            else
            {
                if(hx > 0)
                {
                    z = x - 4 * pio2_1;
                    y[0] = z - 4 * pio2_1t;
                    y[1] = (z - y[0]) - 4 * pio2_1t;
                    return 4;
                }
                else
                {
                    z = x + 4 * pio2_1;
                    y[0] = z + 4 * pio2_1t;
                    y[1] = (z - y[0]) + 4 * pio2_1t;
                    return -4;
                }
            }
        }
    }
    if(ix < 0x413921fb || medium)
    { /* |x| ~< 2^20*(pi/2), medium size */
        /* Use a specialized rint() to get fn.  Assume round-to-nearest. */
        KDfloat64KHR fn = x * invpio2 + 6.7553994410557440e+15;
        fn = fn - 6.7553994410557440e+15;
        n = __kdIrint(fn);
        KDfloat64KHR r = x - fn * pio2_1;
        KDfloat64KHR w = fn * pio2_1t; /* 1st round good to 85 bit */
        {
            KDuint32 high;
            KDint32 j = ix >> 20;
            y[0] = r - w;
            GET_HIGH_WORD(high, y[0]);
            i = j - ((high >> 20) & 0x7ff);
            if(i > 16)
            { /* 2nd iteration needed, good to 118 */
                KDfloat64KHR t = r;
                w = fn * pio2_2;
                r = t - w;
                w = fn * pio2_2t - ((t - r) - w);
                y[0] = r - w;
                GET_HIGH_WORD(high, y[0]);
                i = j - ((high >> 20) & 0x7ff);
                if(i > 49)
                {          /* 3rd iteration need, 151 bits acc */
                    t = r; /* will cover all possible cases */
                    w = fn * pio2_3;
                    r = t - w;
                    w = fn * pio2_3t - ((t - r) - w);
                    y[0] = r - w;
                }
            }
        }
        y[1] = (r - y[0]) - w;
        return n;
    }
    /* 
     * all other (large) arguments
     */
    if(ix >= 0x7ff00000)
    { /* x is inf or NaN */
        y[0] = y[1] = x - x;
        return 0;
    }
    /* set z = scalbn(|x|,ilogb(x)-23) */
    GET_LOW_WORD(low, x);
    e0 = (ix >> 20) - 1046; /* e0 = ilogb(z)-23; */
    INSERT_WORDS(z, ix - ((KDint32)((KDuint32)e0 << 20)), low);
    for(i = 0; i < 2; i++)
    {
        tx[i] = (KDfloat64KHR)((KDint32)(z));
        z = (z - tx[i]) * two24;
    }
    tx[2] = z;
    nx = 3;
    while(tx[nx - 1] == zero)
    {
        nx--; /* skip zero term */
        if(nx == 0)
        {
            break;
        }
    }
    n = __kdRemPio2Kernel(tx, ty, e0, nx);
    if(hx < 0)
    {
        y[0] = -ty[0];
        y[1] = -ty[1];
        return -n;
    }
    y[0] = ty[0];
    y[1] = ty[1];
    return n;
}

/* kdAcosf: Arc cosine function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdAcosf(KDfloat32 x)
{
    volatile KDfloat32
        pio2_lo = 7.5497894159e-08f; /* 0x33a22168 */
    const KDfloat32
        pS0 = 1.6666586697e-01f,
        pS1 = -4.2743422091e-02f,
        pS2 = -8.6563630030e-03f,
        qS1 = -7.0662963390e-01f;

    __KDFloatShape shape_x = {x};
    KDfloat32 z, p, q, r, w, s, c;
    KDint32 ix = shape_x.u32 & KDINT32_MAX;
    KDboolean sign = shape_x.u32 >> 31;

    if(ix >= 0x3f800000)
    { /* |x| >= 1 */
        if(ix == 0x3f800000)
        { /* |x| == 1 */
            if(sign)
            {
                return KD_PI_F + 2.0f * pio2_lo; /* acos(-1)= pi */
            }
            else
            {
                return 0.0; /* acos(1) = 0 */
            }
        }
        return KD_NANF; /* acos(|x|>1) is NaN */
    }
    if(ix < 0x3f000000)
    { /* |x| < 0.5 */
        if(ix <= 0x32800000)
        {
            return KD_PI_2_F + pio2_lo;
        } /*if|x|<2**-26*/
        z = x * x;
        p = z * (pS0 + z * (pS1 + z * pS2));
        q = 1.0f + z * qS1;
        r = p / q;
        return KD_PI_2_F - (shape_x.f32 - (pio2_lo - shape_x.f32 * r));
    }
    else if(sign)
    { /* x < -0.5 */
        z = (1.0f + shape_x.f32) * 0.5f;
        p = z * (pS0 + z * (pS1 + z * pS2));
        q = 1.0f + z * qS1;
        s = kdSqrtf(z);
        r = p / q;
        w = r * s - pio2_lo;
        return KD_PI_F - 2.0f * (s + w);
    }
    else
    { /* x > 0.5 */
        z = (1.0f - shape_x.f32) * 0.5f;
        s = kdSqrtf(z);
        __KDFloatShape df = {s};
        __KDFloatShape idf = {df.f32};
        df.u32 = idf.u32 & 0xfffff000;
        c = (z - df.f32 * df.f32) / (s + df.f32);
        p = z * (pS0 + z * (pS1 + z * pS2));
        q = 1.0f + z * qS1;
        r = p / q;
        w = r * s + c;
        return 2.0f * (df.f32 + w);
    }
}

/* kdAsinf: Arc sine function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdAsinf(KDfloat32 x)
{
    const KDfloat32
        huge = 1.000e+30f,
        /* coefficient for R(x^2) */
        pS0 = 1.6666586697e-01f,
        pS1 = -4.2743422091e-02f,
        pS2 = -8.6563630030e-03f,
        qS1 = -7.0662963390e-01f;

    __KDFloatShape shape_x = {x};
    KDfloat32 t, w, p, q, s;
    KDint32 ix = shape_x.u32 & KDINT32_MAX;
    KDboolean sign = shape_x.u32 >> 31;

    if(ix >= 0x3f800000)
    { /* |x| >= 1 */
        if(ix == 0x3f800000)
        { /* |x| == 1 */
            return shape_x.f32 * KD_PI_2_F;
        }               /* asin(+-1) = +-pi/2 with inexact */
        return KD_NANF; /* asin(|x|>1) is NaN */
    }
    else if(ix < 0x3f000000)
    { /* |x|<0.5 */
        if(ix < 0x39800000)
        { /* |x| < 2**-12 */
            if(huge + shape_x.f32 > 1.0f)
            {
                return shape_x.f32;
            } /* return x with inexact if x!=0*/
        }
        t = shape_x.f32 * shape_x.f32;
        p = t * (pS0 + t * (pS1 + t * pS2));
        q = 1.0f + t * qS1;
        w = p / q;
        return shape_x.f32 + shape_x.f32 * w;
    }
    /* 1> |x|>= 0.5 */
    w = 1.0f - kdFabsf(shape_x.f32);
    t = w * 0.5f;
    p = t * (pS0 + t * (pS1 + t * pS2));
    q = 1.0f + t * qS1;
    s = kdSqrtf(t);
    w = p / q;
    t = KD_PI_2_F - 2.0f * (s + s * w);
    if(sign)
    {
        return -t;
    }
    else
    {
        return t;
    }
}

/* kdAtanf: Arc tangent function. */
KD_API KDfloat32 KD_APIENTRY kdAtanf(KDfloat32 x)
{
    const KDfloat32 atanhi[] = {
        4.6364760399e-01f, /* atan(0.5)hi 0x3eed6338 */
        7.8539812565e-01f, /* atan(1.0)hi 0x3f490fda */
        9.8279368877e-01f, /* atan(1.5)hi 0x3f7b985e */
        1.5707962513e+00f, /* atan(inf)hi 0x3fc90fda */
    };

    const KDfloat32 atanlo[] = {
        5.0121582440e-09f, /* atan(0.5)lo 0x31ac3769 */
        3.7748947079e-08f, /* atan(1.0)lo 0x33222168 */
        3.4473217170e-08f, /* atan(1.5)lo 0x33140fb4 */
        7.5497894159e-08f, /* atan(inf)lo 0x33a22168 */
    };

    const KDfloat32 aT[] = {
        3.3333328366e-01f,
        -1.9999158382e-01f,
        1.4253635705e-01f,
        -1.0648017377e-01f,
        6.1687607318e-02f,
    };

    const KDfloat32 huge = 1.0e30f;
    KDfloat32 w, s1, s2, z;
    KDint32 ix, hx, id;
    GET_FLOAT_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix >= 0x4c800000)
    { /* if |x| >= 2**26 */
        if(ix > 0x7f800000)
        {
            return x + x;
        } /* NaN */
        if(hx > 0)
        {
            return atanhi[3] + *(volatile KDfloat32 *)&atanlo[3];
        }
        else
        {
            return -atanhi[3] - *(volatile KDfloat32 *)&atanlo[3];
        }
    }
    if(ix < 0x3ee00000)
    { /* |x| < 0.4375 */
        if(ix < 0x39800000)
        { /* |x| < 2**-12 */
            if(huge + x > 1.0f)
            {
                return x;
            } /* raise inexact */
        }
        id = -1;
    }
    else
    {
        x = kdFabsf(x);
        if(ix < 0x3f980000)
        { /* |x| < 1.1875 */
            if(ix < 0x3f300000)
            { /* 7/16 <=|x|<11/16 */
                id = 0;
                x = (2.0f * x - 1.0f) / (2.0f + x);
            }
            else
            { /* 11/16<=|x|< 19/16 */
                id = 1;
                x = (x - 1.0f) / (x + 1.0f);
            }
        }
        else
        {
            if(ix < 0x401c0000)
            { /* |x| < 2.4375 */
                id = 2;
                x = (x - 1.5f) / (1.0f + 1.5f * x);
            }
            else
            { /* 2.4375 <= |x| < 2**26 */
                id = 3;
                x = -1.0f / x;
            }
        }
    }
    /* end of argument reduction */
    z = x * x;
    w = z * z;
    /* break sum from i=0 to 10 _aT[i]z**(i+1) into odd and even poly */
    s1 = z * (aT[0] + w * (aT[2] + w * aT[4]));
    s2 = w * (aT[1] + w * aT[3]);
    if(id < 0)
    {
        return x - x * (s1 + s2);
    }
    else
    {
        z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
        return (hx < 0) ? -z : z;
    }
}

/* kdAtan2f: Arc tangent function. */
KD_API KDfloat32 KD_APIENTRY kdAtan2f(KDfloat32 y, KDfloat32 x)
{
    volatile KDfloat32
        tiny = 1.0e-30f,
        pi_lo = -8.7422776573e-08f; /* 0xb3bbbd2e */

    KDfloat32 z;
    KDint32 k, m, hx, hy, ix, iy;
    GET_FLOAT_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    GET_FLOAT_WORD(hy, y);
    iy = hy & KDINT32_MAX;
    if((ix > 0x7f800000) || (iy > 0x7f800000))
    { /* x or y is NaN */
        return x + y;
    }
    if(hx == 0x3f800000)
    {
        return kdAtanf(y);
    }                                                /* x=1.0 */
    m = (((KDuint)hy >> 31) & 1) | ((hx >> 30) & 2); /* 2*sign(x)+sign(y) */

    /* when y = 0 */
    if(iy == 0)
    {
        switch(m)
        {
            case 0:
            case 1:
            {
                return y; /* atan(+-0,+anything)=+-0 */
            }
            case 2:
            {
                return KD_PI_F + tiny; /* atan(+0,-anything) = pi */
            }
            case 3:
            {
                return -KD_PI_F - tiny; /* atan(-0,-anything) =-pi */
            }
            default:
            {
                break;
            }
        }
    }
    /* when x = 0 */
    if(ix == 0)
    {
        return (hy < 0) ? -KD_PI_2_F - tiny : KD_PI_2_F + tiny;
    }

    /* when x is INF */
    if(ix == 0x7f800000)
    {
        if(iy == 0x7f800000)
        {
            switch(m)
            {
                case 0:
                {
                    return KD_PI_4_F + tiny; /* atan(+INF,+INF) */
                }
                case 1:
                {
                    return -KD_PI_4_F - tiny; /* atan(-INF,+INF) */
                }
                case 2:
                {
                    return 3.0f * KD_PI_4_F + tiny; /*atan(+INF,-INF)*/
                }
                case 3:
                {
                    return -3.0f * KD_PI_4_F - tiny; /*atan(-INF,-INF)*/
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
            switch(m)
            {
                case 0:
                {
                    return 0.0f; /* atan(+...,+INF) */
                }
                case 1:
                {
                    return -0.0f; /* atan(-...,+INF) */
                }
                case 2:
                {
                    return KD_PI_F + tiny; /* atan(+...,-INF) */
                }
                case 3:
                {
                    return -KD_PI_F - tiny; /* atan(-...,-INF) */
                }
                default:
                {
                    break;
                }
            }
        }
    }
    /* when y is INF */
    if(iy == 0x7f800000)
    {
        return (hy < 0) ? -KD_PI_2_F - tiny : KD_PI_2_F + tiny;
    }

    /* compute y/x */
    k = (iy - ix) >> 23;
    if(k > 26)
    { /* |y/x| >  2**26 */
        z = KD_PI_2_F + 0.5f * pi_lo;
        m &= 1;
    }
    else if(k < -26 && hx < 0)
    {
        z = 0.0f; /* 0 > |y|/x > -2**-26 */
    }
    else
    {
        z = kdAtanf(kdFabsf(y / x)); /* safe to do y/x */
    }
    switch(m)
    {
        case 0:
        {
            return z; /* atan(+,+) */
        }
        case 1:
        {
            return -z; /* atan(-,+) */
        }
        case 2:
        {
            return KD_PI_F - (z - pi_lo); /* atan(+,-) */
        }
        default: /* case 3 */
        {
            return (z - pi_lo) - KD_PI_F; /* atan(-,-) */
        }
    }
}

/* kdCosf: Cosine function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdCosf(KDfloat32 x)
{
    KDfloat64KHR y;
    KDint32 n, hx, ix;
    GET_FLOAT_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix <= 0x3f490fda)
    { /* |x| ~<= pi/4 */
        if(ix < 0x39800000)
        { /* |x| < 2**-12 */
            if(((KDint)x) == 0)
            {
                return 1.0f;
            }
        } /* 1 with inexact if x != 0 */
        return __kdCosdfKernel((KDfloat64KHR)x);
    }
    if(ix <= 0x407b53d1)
    { /* |x| ~<= 5*pi/4 */
        if(ix > 0x4016cbe3)
        { /* |x|  ~> 3*pi/4 */
            return -__kdCosdfKernel((KDfloat64KHR)x + (hx > 0 ? -(2 * KD_PI_2_KHR) : (2 * KD_PI_2_KHR)));
        }
        else
        {
            if(hx > 0)
            {
                return __kdSindfKernel(KD_PI_2_KHR - (KDfloat64KHR)x);
            }
            else
            {
                return __kdSindfKernel((KDfloat64KHR)x + KD_PI_2_KHR);
            }
        }
    }
    if(ix <= 0x40e231d5)
    { /* |x| ~<= 9*pi/4 */
        if(ix > 0x40afeddf)
        { /* |x|  ~> 7*pi/4 */
            return __kdCosdfKernel((KDfloat64KHR)x + (hx > 0 ? -(4 * KD_PI_2_KHR) : (4 * KD_PI_2_KHR)));
        }
        else
        {
            if(hx > 0)
            {
                return __kdSindfKernel((KDfloat64KHR)x - (3 * KD_PI_2_KHR));
            }
            else
            {
                return __kdSindfKernel(-(3 * KD_PI_2_KHR) - (KDfloat64KHR)x);
            }
        }
    }
    /* cos(Inf or NaN) is NaN */
    else if(ix >= 0x7f800000)
    {
        return KD_NANF;
        /* general argument reduction needed */
    }
    else
    {
        n = __kdRemPio2f(x, &y);
        switch(n & 3)
        {
            case 0:
            {
                return __kdCosdfKernel(y);
            }
            case 1:
            {
                return __kdSindfKernel(-y);
            }
            case 2:
            {
                return -__kdCosdfKernel(y);
            }
            default:
            {
                return __kdSindfKernel(y);
            }
        }
    }
}

/* kdSinf: Sine function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdSinf(KDfloat32 x)
{
    KDfloat64KHR y;
    KDint32 n, hx, ix;
    GET_FLOAT_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix <= 0x3f490fda)
    { /* |x| ~<= pi/4 */
        if(ix < 0x39800000)
        { /* |x| < 2**-12 */
            if(((KDint)x) == 0)
            {
                return x;
            }
        } /* x with inexact if x != 0 */
        return __kdSindfKernel((KDfloat64KHR)x);
    }
    if(ix <= 0x407b53d1)
    { /* |x| ~<= 5*pi/4 */
        if(ix <= 0x4016cbe3)
        { /* |x| ~<= 3pi/4 */
            if(hx > 0)
            {
                return __kdCosdfKernel((KDfloat64KHR)x - KD_PI_2_KHR);
            }
            else
            {
                return -__kdCosdfKernel((KDfloat64KHR)x + KD_PI_2_KHR);
            }
        }
        else
        {
            return __kdSindfKernel((hx > 0 ? (2 * KD_PI_2_KHR) : -(2 * KD_PI_2_KHR)) - (KDfloat64KHR)x);
        }
    }
    if(ix <= 0x40e231d5)
    { /* |x| ~<= 9*pi/4 */
        if(ix <= 0x40afeddf)
        { /* |x| ~<= 7*pi/4 */
            if(hx > 0)
            {
                return -__kdCosdfKernel((KDfloat64KHR)x - (3 * KD_PI_2_KHR));
            }
            else
            {
                return __kdCosdfKernel((KDfloat64KHR)x + (3 * KD_PI_2_KHR));
            }
        }
        else
        {
            return __kdSindfKernel((KDfloat64KHR)x + (hx > 0 ? -(4 * KD_PI_2_KHR) : (4 * KD_PI_2_KHR)));
        }
    }
    /* sin(Inf or NaN) is NaN */
    else if(ix >= 0x7f800000)
    {
        return KD_NANF;
        /* general argument reduction needed */
    }
    else
    {
        n = __kdRemPio2f(x, &y);
        switch(n & 3)
        {
            case 0:
            {
                return __kdSindfKernel(y);
            }
            case 1:
            {
                return __kdCosdfKernel(y);
            }
            case 2:
            {
                return __kdSindfKernel(-y);
            }
            default:
            {
                return -__kdCosdfKernel(y);
            }
        }
    }
}

/* kdTanf: Tangent function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdTanf(KDfloat32 x)
{
    KDfloat64KHR y;
    KDint32 n, hx, ix;
    GET_FLOAT_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix <= 0x3f490fda)
    { /* |x| ~<= pi/4 */
        if(ix < 0x39800000)
        { /* |x| < 2**-12 */
            if(((KDint)x) == 0)
            {
                return x;
            }
        } /* x with inexact if x != 0 */
        return __kdTandfKernel((KDfloat64KHR)x, 1);
    }
    if(ix <= 0x407b53d1)
    { /* |x| ~<= 5*pi/4 */
        if(ix <= 0x4016cbe3)
        { /* |x| ~<= 3pi/4 */
            return __kdTandfKernel((KDfloat64KHR)x + (hx > 0 ? -KD_PI_2_KHR : KD_PI_2_KHR), -1);
        }
        else
        {
            return __kdTandfKernel((KDfloat64KHR)x + (hx > 0 ? -(2 * KD_PI_2_KHR) : (2 * KD_PI_2_KHR)), 1);
        }
    }
    if(ix <= 0x40e231d5)
    { /* |x| ~<= 9*pi/4 */
        if(ix <= 0x40afeddf)
        { /* |x| ~<= 7*pi/4 */
            return __kdTandfKernel((KDfloat64KHR)x + (hx > 0 ? -(3 * KD_PI_2_KHR) : (3 * KD_PI_2_KHR)), -1);
        }
        else
        {
            return __kdTandfKernel((KDfloat64KHR)x + (hx > 0 ? -(4 * KD_PI_2_KHR) : (4 * KD_PI_2_KHR)), 1);
        }
    }
    /* tan(Inf or NaN) is NaN */
    else if(ix >= 0x7f800000)
    {
        return KD_NANF;
        /* general argument reduction needed */
    }
    else
    {
        n = __kdRemPio2f(x, &y);
        /* integer parameter: 1 -- n even; -1 -- n odd */
        return __kdTandfKernel(y, 1 - ((n & 1) << 1));
    }
}

/* kdExpf: Exponential function. */
KD_API KDfloat32 KD_APIENTRY kdExpf(KDfloat32 x)
{
    const KDfloat32
        halF[2] = {
            0.5f,
            -0.5f,
        },
        o_threshold = 8.8721679688e+01f,  /* 0x42b17180 */
        u_threshold = -1.0397208405e+02f, /* 0xc2cff1b5 */
        ln2HI[2] = {
            6.9314575195e-01f, /* 0x3f317200 */
            -6.9314575195e-01f,
        }, /* 0xbf317200 */
        ln2LO[2] = {
            1.4286067653e-06f, /* 0x35bfbe8e */
            -1.4286067653e-06f,
        },                          /* 0xb5bfbe8e */
        invln2 = 1.4426950216e+00f, /* 0x3fb8aa3b */
        /*
         * Domain [-0.34568, 0.34568], range ~[-4.278e-9, 4.447e-9]:
         * |x*(exp(x)+1)/(exp(x)-1) - p(x)| < 2**-27.74
         */
        P1 = 1.6666625440e-1f,  /*  0xaaaa8f.0p-26 */
        P2 = -2.7667332906e-3f; /* -0xb55215.0p-32 */

    volatile KDfloat32
        huge = 1.0e+30f,
        twom100 = 7.8886090522e-31f; /* 2**-100=0x0d800000 */

    KDfloat32 y, hi = 0.0f, lo = 0.0f, c, t, twopk;
    KDint32 k = 0, xsb;
    KDuint32 hx;
    GET_FLOAT_WORD(hx, x);
    xsb = (hx >> 31) & 1; /* sign bit of x */
    hx &= KDINT32_MAX;    /* high word of |x| */
    /* filter out non-finite argument */
    if(hx >= 0x42b17218)
    { /* if |x|>=88.721... */
        if(hx > 0x7f800000)
        {
            return x + x;
        } /* NaN */
        if(hx == 0x7f800000)
        {
            return (xsb == 0) ? x : 0.0f; /* exp(+-inf)={inf,0} */
        }
        if(x > o_threshold)
        {
            return huge * huge; /* overflow */
        }
        if(x < u_threshold)
        {
            return twom100 * twom100; /* underflow */
        }
    }
    /* argument reduction */
    if(hx > 0x3eb17218)
    { /* if  |x| > 0.5 ln2 */
        if(hx < 0x3F851592)
        { /* and |x| < 1.5 ln2 */
            hi = x - ln2HI[xsb];
            lo = ln2LO[xsb];
            k = 1 - xsb - xsb;
        }
        else
        {
            k = (KDint32)(invln2 * x + halF[xsb]);
            t = (KDfloat32)k;
            hi = x - t * ln2HI[0]; /* t*ln2HI is exact here */
            lo = t * ln2LO[0];
        }
        x = hi - lo;
    }
    else if(hx < 0x39000000)
    { /* when |x|<2**-14 */
        if(huge + x > 1.0f)
        {
            return 1.0f + x;
        } /* trigger inexact */
    }
    else
    {
        k = 0;
    }
    /* x is now in primary range */
    t = x * x;
    if(k >= -125)
    {
        SET_FLOAT_WORD(twopk, 0x3f800000 + ((KDuint32)k << 23));
    }
    else
    {
        SET_FLOAT_WORD(twopk, 0x3f800000 + ((k + 100) << 23));
    }
    c = x - t * (P1 + t * P2);
    if(k == 0)
    {
        return 1.0f - ((x * c) / (c - 2.0f) - x);
    }
    else
    {
        y = 1.0f - ((lo - (x * c) / (2.0f - c)) - hi);
    }
    if(k >= -125)
    {
        if(k == 128)
        {
            return y * 2.0f * 1.7014118346046923e+38f;
        }
        return y * twopk;
    }
    else
    {
        return y * twopk * twom100;
    }
}

/* kdLogf: Natural logarithm function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdLogf(KDfloat32 x)
{
    const KDfloat32
        ln2_hi = 6.9313812256e-01f, /* 0x3f317180 */
        ln2_lo = 9.0580006145e-06f, /* 0x3717f7d1 */
        two25 = 3.355443200e+07f,   /* 0x4c000000 */
        /* |(log(1+s)-log(1-s))/s - Lg(s)| < 2**-34.24 (~[-4.95e-11, 4.97e-11]). */
        Lg1 = 6.6666662693023682e-01f, /* 0xaaaaaa.0p-24f */
        Lg2 = 4.0000972151756287e-01f, /* 0xccce13.0p-25f */
        Lg3 = 2.8498786687850952e-01f, /* 0x91e9ee.0p-25f */
        Lg4 = 2.4279078841209412e-01f; /* 0xf89e26.0p-26f */

    KDfloat32 f, s, z, R, w, t1, t2, dk;
    KDint32 k, ix, i, j;

    GET_FLOAT_WORD(ix, x);
    k = 0;
    if(ix < 0x00800000)
    { /* x < 2**-126  */
        if((ix & 0x7f800000) == 0)
        { /* log(+-0)=-inf */
            return -KD_INFINITY;
        }
        if(ix < 0)
        { /* log(-#) = NaN */
            return KD_NANF;
        }
        k -= 25;
        x *= two25; /* subnormal number, scale up x */
        GET_FLOAT_WORD(ix, x);
    }
    if(ix >= 0x7f800000)
    {
        return x + x;
    }
    k += (ix >> 23) - 127;
    ix &= 0x007fffff;
    i = (ix + (0x95f64 << 3)) & 0x800000;
    SET_FLOAT_WORD(x, ix | (i ^ 0x3f800000)); /* normalize x or x/2 */
    k += (i >> 23);
    f = x - 1.0f;
    if((0x007fffff & (0x8000 + ix)) < 0xc000)
    { /* -2**-9 <= f < 2**-9 */
        if(f == 0.0f)
        {
            if(k == 0)
            {
                return 0.0f;
            }
            else
            {
                dk = (KDfloat32)k;
                return dk * ln2_hi + dk * ln2_lo;
            }
        }
        R = f * f * (0.5f - 0.33333333333333333f * f);
        if(k == 0)
        {
            return f - R;
        }
        else
        {
            dk = (KDfloat32)k;
            return dk * ln2_hi - ((R - dk * ln2_lo) - f);
        }
    }
    s = f / (2.0f + f);
    dk = (KDfloat32)k;
    z = s * s;
    i = ix - (0x6147a << 3);
    w = z * z;
    j = (0x6b851 << 3) - ix;
    t1 = w * (Lg2 + w * Lg4);
    t2 = z * (Lg1 + w * Lg3);
    i |= j;
    R = t2 + t1;
    if(i > 0)
    {
        KDfloat32 hfsq = 0.5f * f * f;
        if(k == 0)
        {
            return f - (hfsq - s * (hfsq + R));
        }
        else
        {
            return dk * ln2_hi - ((hfsq - (s * (hfsq + R) + dk * ln2_lo)) - f);
        }
    }
    else
    {
        if(k == 0)
        {
            return f - s * (f - R);
        }
        else
        {
            return dk * ln2_hi - ((s * (f - R) - dk * ln2_lo) - f);
        }
    }
}

/* kdFabsf: Absolute value. */
KD_API KDfloat32 KD_APIENTRY kdFabsf(KDfloat32 x)
{
    KDuint32 ix;
    GET_FLOAT_WORD(ix, x);
    SET_FLOAT_WORD(x, ix & KDINT32_MAX);
    return x;
}

/* kdPowf: Power function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdPowf(KDfloat32 x, KDfloat32 y)
{
    static const KDfloat32
        bp[] = {
            1.0f,
            1.5f,
        },
        dp_h[] = {
            0.0f,
            5.84960938e-01f,
        }, /* 0x3f15c000 */
        dp_l[] = {
            0.0f,
            1.56322085e-06f,
        },                   /* 0x35d1cfdc */
        two24 = 16777216.0f, /* 0x4b800000 */
        huge = 1.0e30f, tiny = 1.0e-30f,
        /* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
        L1 = 6.0000002384e-01f,      /* 0x3f19999a */
        L2 = 4.2857143283e-01f,      /* 0x3edb6db7 */
        L3 = 3.3333334327e-01f,      /* 0x3eaaaaab */
        L4 = 2.7272811532e-01f,      /* 0x3e8ba305 */
        L5 = 2.3066075146e-01f,      /* 0x3e6c3255 */
        L6 = 2.0697501302e-01f,      /* 0x3e53f142 */
        P1 = 1.6666667163e-01f,      /* 0x3e2aaaab */
        P2 = -2.7777778450e-03f,     /* 0xbb360b61 */
        P3 = 6.6137559770e-05f,      /* 0x388ab355 */
        P4 = -1.6533901999e-06f,     /* 0xb5ddea0e */
        P5 = 4.1381369442e-08f,      /* 0x3331bb4c */
        lg2 = 6.9314718246e-01f,     /* 0x3f317218 */
        lg2_h = 6.93145752e-01f,     /* 0x3f317200 */
        lg2_l = 1.42860654e-06f,     /* 0x35bfbe8c */
        ovt = 4.2995665694e-08f,     /* -(128-log2(ovfl+.5ulp)) */
        cp = 9.6179670095e-01f,      /* 0x3f76384f =2/(3ln2) */
        cp_h = 9.6191406250e-01f,    /* 0x3f764000 =12b cp */
        cp_l = -1.1736857402e-04f,   /* 0xb8f623c6 =tail of cp_h */
        ivln2 = 1.4426950216e+00f,   /* 0x3fb8aa3b =1/ln2 */
        ivln2_h = 1.4426879883e+00f, /* 0x3fb8aa00 =16b 1/ln2*/
        ivln2_l = 7.0526075433e-06f; /* 0x36eca570 =1/ln2 tail*/

    KDfloat32 z, ax, p_h, p_l;
    KDfloat32 y1, t1, t2, r, sn, t, u, v, w;
    KDint32 i, j, k, yisint, n;
    KDint32 hx, hy, ix, iy, is;
    GET_FLOAT_WORD(hx, x);
    GET_FLOAT_WORD(hy, y);
    ix = hx & KDINT32_MAX;
    iy = hy & KDINT32_MAX;
    /* y==zero: x**0 = 1 */
    if(iy == 0)
    {
        return 1.0f;
    }
    /* x==1: 1**y = 1, even if y is NaN */
    if(hx == 0x3f800000)
    {
        return 1.0f;
    }
    /* y!=zero: result is NaN if either arg is NaN */
    if(ix > 0x7f800000 || iy > 0x7f800000)
    {
        return (x + 0.0f) + (y + 0.0f);
    }
    /* determine if y is an odd int when x < 0
     * yisint = 0   ... y is not an integer
     * yisint = 1   ... y is an odd int
     * yisint = 2   ... y is an even int
     */
    yisint = 0;
    if(hx < 0)
    {
        if(iy >= 0x4b800000)
        {
            yisint = 2; /* even integer y */
        }
        else if(iy >= 0x3f800000)
        {
            k = (iy >> 23) - 0x7f; /* exponent */
            j = iy >> (23 - k);
            if((j << (23 - k)) == iy)
            {
                yisint = 2 - (j & 1);
            }
        }
    }
    /* special value of y */
    if(iy == 0x7f800000)
    { /* y is +-inf */
        if(ix == 0x3f800000)
        {
            return 1.0f; /* (-1)**+-inf is NaN */
        }
        else if(ix > 0x3f800000)
        { /* (|x|>1)**+-inf = inf,0 */
            return (hy >= 0) ? y : 0.0f;
        }
        else
        { /* (|x|<1)**-,+inf = inf,0 */
            return (hy < 0) ? -y : 0.0f;
        }
    }
    if(iy == 0x3f800000)
    { /* y is  +-1 */
        if(hy < 0)
        {
            return 1.0f / x;
        }
        else
        {
            return x;
        }
    }
    if(hy == 0x40000000)
    {
        return x * x;
    } /* y is  2 */
    if(hy == 0x3f000000)
    {               /* y is  0.5 */
        if(hx >= 0) /* x >= +0 */
        {
            return kdSqrtf(x);
        }
    }
    ax = kdFabsf(x);
    /* special value of x */
    if(ix == 0x7f800000 || ix == 0 || ix == 0x3f800000)
    {
        z = ax; /*x is +-0,+-inf,+-1*/
        if(hy < 0)
        {
            z = 1.0f / z; /* z = (1/|x|) */
        }
        if(hx < 0)
        {
            if(((ix - 0x3f800000) | yisint) == 0)
            {
                z = KD_NANF; /* (-1)**non-int is NaN */
            }
            else if(yisint == 1)
            {
                z = -z; /* (x<0)**odd = -(|x|**odd) */
            }
        }
        return z;
    }
    n = ((KDuint32)hx >> 31) - 1;
    /* (x<0)**(non-int) is NaN */
    if((n | yisint) == 0)
    {
        return KD_NANF;
    }
    sn = 1.0f; /* s (sign of result -ve**odd) = -1 else = 1 */
    if((n | (yisint - 1)) == 0)
    {
        sn = -1.0f; /* (-ve)**(odd int) */
    }
    /* |y| is huge */
    if(iy > 0x4d000000)
    { /* if |y| > 2**27 */
        /* over/underflow if x is not close to one */
        if(ix < 0x3f7ffff8)
        {
            return (hy < 0) ? sn * huge * huge : sn * tiny * tiny;
        }
        if(ix > 0x3f800007)
        {
            return (hy > 0) ? sn * huge * huge : sn * tiny * tiny;
        }
        /* now |1-x| is tiny <= 2**-20, suffice to compute
       log(x) by x-x^2/2+x^3/3-x^4/4 */
        t = ax - 1; /* t has 20 trailing zeros */
        w = (t * t) * (0.5f - t * (0.333333333333f - t * 0.25f));
        u = ivln2_h * t; /* ivln2_h has 16 sig. bits */
        v = t * ivln2_l - w * ivln2;
        t1 = u + v;
        GET_FLOAT_WORD(is, t1);
        SET_FLOAT_WORD(t1, is & 0xfffff000);
        t2 = v - (t1 - u);
    }
    else
    {
        KDfloat32 s2, s_h, s_l, t_h, t_l;
        n = 0;
        /* take care subnormal number */
        if(ix < 0x00800000)
        {
            ax *= two24;
            n -= 24;
            GET_FLOAT_WORD(ix, ax);
        }
        n += ((ix) >> 23) - 0x7f;
        j = ix & 0x007fffff;
        /* determine interval */
        ix = j | 0x3f800000; /* normalize ix */
        if(j <= 0x1cc471)
        {
            k = 0; /* |x|<sqrt(3/2) */
        }
        else if(j < 0x5db3d7)
        {
            k = 1; /* |x|<sqrt(3)   */
        }
        else
        {
            k = 0;
            n += 1;
            ix -= 0x00800000;
        }
        SET_FLOAT_WORD(ax, ix);
        /* compute s = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
        u = ax - bp[k]; /* bp[0]=1.0, bp[1]=1.5 */
        v = 1.0f / (ax + bp[k]);
        KDfloat32 s = u * v;
        s_h = s;
        GET_FLOAT_WORD(is, s_h);
        SET_FLOAT_WORD(s_h, is & 0xfffff000);
        /* t_h=ax+bp[k] High */
        is = ((ix >> 1) & 0xfffff000) | 0x20000000;
        SET_FLOAT_WORD(t_h, is + 0x00400000 + (k << 21));
        t_l = ax - (t_h - bp[k]);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);
        /* compute log(ax) */
        s2 = s * s;
        r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 + s2 * L6)))));
        r += s_l * (s_h + s);
        s2 = s_h * s_h;
        t_h = 3.0f + s2 + r;
        GET_FLOAT_WORD(is, t_h);
        SET_FLOAT_WORD(t_h, is & 0xfffff000);
        t_l = r - ((t_h - 3.0f) - s2);
        /* u+v = s*(1+...) */
        u = s_h * t_h;
        v = s_l * t_h + t_l * s;
        /* 2/(3log2)*(s+...) */
        p_h = u + v;
        GET_FLOAT_WORD(is, p_h);
        SET_FLOAT_WORD(p_h, is & 0xfffff000);
        p_l = v - (p_h - u);
        KDfloat32 z_h = cp_h * p_h; /* cp_h+cp_l = 2/(3*log2) */
        KDfloat32 z_l = cp_l * p_h + p_l * cp + dp_l[k];
        /* log2(ax) = (s+..)*2/(3*log2) = n + dp_h + z_h + z_l */
        t = (KDfloat32)n;
        t1 = (((z_h + z_l) + dp_h[k]) + t);
        GET_FLOAT_WORD(is, t1);
        SET_FLOAT_WORD(t1, is & 0xfffff000);
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }
    /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
    GET_FLOAT_WORD(is, y);
    SET_FLOAT_WORD(y1, is & 0xfffff000);
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    GET_FLOAT_WORD(j, z);
    if(j > 0x43000000) /* if z > 128 */
    {
        return sn * huge * huge; /* overflow */
    }
    else if(j == 0x43000000)
    { /* if z == 128 */
        if(p_l + ovt > z - p_h)
        {
            return sn * huge * huge; /* overflow */
        }
    }
    else if((j & KDINT32_MAX) > 0x43160000) /* z <= -150 */
    {
        return sn * tiny * tiny; /* underflow */
    }
    else if((j & KDUINT_MAX) == 0xc3160000)
    { /* z == -150 */
        if(p_l <= z - p_h)
        {
            return sn * tiny * tiny; /* underflow */
        }
    }
    /*
     * compute 2**(p_h+p_l)
     */
    i = j & KDINT32_MAX;
    k = (i >> 23) - 0x7f;
    n = 0;
    if(i > 0x3f000000)
    { /* if |z| > 0.5, set n = [z+0.5] */
        n = j + (0x00800000 >> (k + 1));
        k = ((n & KDINT32_MAX) >> 23) - 0x7f; /* new k for n */
        SET_FLOAT_WORD(t, n & ~(0x007fffff >> k));
        n = ((n & 0x007fffff) | 0x00800000) >> (23 - k);
        if(j < 0)
        {
            n = -n;
        }
        p_h -= t;
    }
    t = p_l + p_h;
    GET_FLOAT_WORD(is, t);
    SET_FLOAT_WORD(t, is & 0xffff8000);
    u = t * lg2_h;
    v = (p_l - (t - p_h)) * lg2 + t * lg2_l;
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
    r = (z * t1) / (t1 - 2.0f) - (w + z * w);
    z = 1.0f - (r - z);
    GET_FLOAT_WORD(j, z);
    j += ((KDuint32)n << 23);
    if((j >> 23) <= 0)
    {
        z = __kdScalbnf(z, n); /* subnormal output */
    }
    else
    {
        SET_FLOAT_WORD(z, j);
    }
    return sn * z;
}

/* kdSqrtf: Square root function. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdSqrtf(KDfloat32 x)
{
#ifdef __SSE2__
    KDfloat32 result = 0.0f;
    _mm_store_ss(&result, _mm_sqrt_ss(_mm_load_ss(&x)));
    return result;
#else
    const KDfloat32 tiny = 1.0e-30f;
    KDfloat32 z;
    KDint32 sign = (KDint32)KDINT32_MIN;
    KDint32 ix, s, q, m, i;
    KDuint32 r;

    GET_FLOAT_WORD(ix, x);
    /* take care of Inf and NaN */
    if((ix & 0x7f800000) == 0x7f800000)
    {
        return x * x + x; /* sqrt(NaN)=NaN, sqrt(+inf)=+inf, sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if(ix <= 0)
    {
        if((ix & (~sign)) == 0)
        { /* sqrt(+-0) = +-0 */
            return x;
        }
        else if(ix < 0)
        { /* sqrt(-ve) = sNaN */
            return KD_NANF;
        }
    }
    /* normalize x */
    m = (ix >> 23);
    if(m == 0)
    { /* subnormal x */
        for(i = 0; (ix & 0x00800000) == 0; i++)
        {
            ix <<= 1;
        }
        m -= i - 1;
    }
    m -= 127; /* unbias exponent */
    ix = (ix & 0x007fffff) | 0x00800000;
    if(m & 1)
    { /* odd m, double x to make it even */
        ix += ix;
    }
    m >>= 1; /* m = [m/2] */
    /* generate sqrt(x) bit by bit */
    ix += ix;
    q = s = 0;      /* q = sqrt(x) */
    r = 0x01000000; /* r = moving bit from right to left */
    while(r != 0)
    {
        KDint32 t = s + r;
        if(t <= ix)
        {
            s = t + r;
            ix -= t;
            q += r;
        }
        ix += ix;
        r >>= 1;
    }
    /* use floating add to find out rounding direction */
    if(ix != 0)
    {
        z = 1.0f - tiny; /* trigger inexact flag */
        if(z >= 1.0f)
        {
            z = 1.0f + tiny;
            if(z > 1.0f)
            {
                q += 2;
            }
            else
            {
                q += (q & 1);
            }
        }
    }
    ix = (q >> 1) + 0x3f000000;
    ix += (m << 23);
    SET_FLOAT_WORD(z, ix);
    return z;
#endif
}

/* kdCeilf: Return ceiling value. */
KD_API KDfloat32 KD_APIENTRY kdCeilf(KDfloat32 x)
{
#ifdef __SSE4_1__
    KDfloat32 result = 0.0f;
    _mm_store_ss(&result, _mm_ceil_ss(_mm_load_ss(&result), _mm_load_ss(&x)));
    return result;
#else
    const KDfloat32 huge = 1.0e30f;
    KDint32 i0, j0;
    KDuint32 i;

    GET_FLOAT_WORD(i0, x);
    j0 = ((i0 >> 23) & 0xff) - 0x7f;
    if(j0 < 23)
    {
        if(j0 < 0)
        { /* raise inexact if x != 0 */
            if(huge + x > 0.0f)
            { /* return 0*sign(x) if |x|<1 */
                if(i0 < 0)
                {
                    i0 = KDINT32_MIN;
                }
                else if(i0 != 0)
                {
                    i0 = 0x3f800000;
                }
            }
        }
        else
        {
            i = (0x007fffff) >> j0;
            if((i0 & i) == 0)
            {
                return x;
            } /* x is integral */
            if(huge + x > 0.0f)
            { /* raise inexact flag */
                if(i0 > 0)
                {
                    i0 += (0x00800000) >> j0;
                }
                i0 &= (~i);
            }
        }
    }
    else
    {
        if(j0 == 0x80)
        {
            return x + x; /* inf or NaN */
        }
        else
        {
            return x;
        } /* x is integral */
    }
    SET_FLOAT_WORD(x, i0);
    return x;
#endif
}

/* kdFloorf: Return floor value. */
KD_API KDfloat32 KD_APIENTRY kdFloorf(KDfloat32 x)
{
#ifdef __SSE4_1__
    KDfloat32 result = 0.0f;
    _mm_store_ss(&result, _mm_floor_ss(_mm_load_ss(&result), _mm_load_ss(&x)));
    return result;
#else
    const KDfloat32 huge = 1.0e30f;
    KDint32 i0, j0;
    KDuint32 i;

    GET_FLOAT_WORD(i0, x);
    j0 = ((i0 >> 23) & 0xff) - 0x7f;
    if(j0 < 23)
    {
        if(j0 < 0)
        { /* raise inexact if x != 0 */
            if(huge + x > 0.0f)
            { /* return 0*sign(x) if |x|<1 */
                if(i0 >= 0)
                {
                    i0 = 0;
                }
                else if((i0 & KDINT32_MAX) != 0)
                {
                    i0 = 0xbf800000;
                }
            }
        }
        else
        {
            i = (0x007fffff) >> j0;
            if((i0 & i) == 0)
            {
                return x;
            } /* x is integral */
            if(huge + x > 0.0f)
            { /* raise inexact flag */
                if(i0 < 0)
                {
                    i0 += (0x00800000) >> j0;
                }
                i0 &= (~i);
            }
        }
    }
    else
    {
        if(j0 == 0x80)
        {
            return x + x; /* inf or NaN */
        }
        else
        {
            return x;
        } /* x is integral */
    }
    SET_FLOAT_WORD(x, i0);
    return x;
#endif
}

/* kdRoundf: Round value to nearest integer. */
KD_API KDfloat32 KD_APIENTRY kdRoundf(KDfloat32 x)
{
    KDfloat32 t;
    KDuint32 hx;
    GET_FLOAT_WORD(hx, x);
    if((hx & KDUINT32_MAX) == 0x7f800000)
    {
        return (x + x);
    }
    if(!(hx & KDINT32_MIN))
    {
        t = kdFloorf(x);
        if(t - x <= -0.5f)
        {
            t += 1;
        }
        return (t);
    }
    else
    {
        t = kdFloorf(-x);
        if(t + x <= -0.5f)
        {
            t += 1;
        }
        return (-t);
    }
}

/* kdInvsqrtf: Inverse square root function. */
KD_API KDfloat32 KD_APIENTRY kdInvsqrtf(KDfloat32 x)
{
#ifdef __SSE__
    KDfloat32 result = 0.0f;
    _mm_store_ss(&result, _mm_rsqrt_ss(_mm_load_ss(&x)));
    result = (0.5f * (result + 1.0f / (x * result)));
    return result;
#else
    return 1.0f / kdSqrtf(x);
#endif
}

/* kdFmodf: Calculate floating point remainder. */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat32 KD_APIENTRY
kdFmodf(KDfloat32 x, KDfloat32 y)
{
    const KDfloat32 Zero[] = {
        0.0f,
        -0.0f,
    };
    KDint32 n, hx, hy, hz, ix, iy, sx, i;

    GET_FLOAT_WORD(hx, x);
    GET_FLOAT_WORD(hy, y);
    sx = hx & KDINT32_MIN; /* sign of x */
    hx ^= sx;              /* |x| */
    hy &= KDINT32_MAX;     /* |y| */
    /* purge off exception values */
    /* y=0,or x not finite */
    if(hy == 0 || (hx >= 0x7f800000) || (hy > 0x7f800000))
    { /* or y is NaN */
        return (x * y) / (x * y);
    }
    if(hx < hy)
    {
        return x;
    } /* |x|<|y| return x */
    if(hx == hy)
    {
        return Zero[(KDuint32)sx >> 31]; /* |x|=|y| return x*0*/
    }
    /* determine ix = ilogb(x) */
    if(hx < 0x00800000)
    { /* subnormal x */
        for(ix = -126, i = (hx << 8); i > 0; i <<= 1)
        {
            ix -= 1;
        }
    }
    else
    {
        ix = (hx >> 23) - 127;
    }
    /* determine iy = ilogb(y) */
    if(hy < 0x00800000)
    { /* subnormal y */
        for(iy = -126, i = (hy << 8); i >= 0; i <<= 1)
        {
            iy -= 1;
        }
    }
    else
    {
        iy = (hy >> 23) - 127;
    }
    /* set up {hx,lx}, {hy,ly} and align y to x */
    if(ix >= -126)
    {
        hx = 0x00800000 | (0x007fffff & hx);
    }
    else
    { /* subnormal x, shift x to normal */
        n = -126 - ix;
        hx = hx << n;
    }
    if(iy >= -126)
    {
        hy = 0x00800000 | (0x007fffff & hy);
    }
    else
    { /* subnormal y, shift y to normal */
        n = -126 - iy;
        hy = hy << n;
    }
    /* fix point fmod */
    n = ix - iy;
    while(n--)
    {
        hz = hx - hy;
        if(hz < 0)
        {
            hx = hx + hx;
        }
        else
        {
            if(hz == 0) /* return sign(x)*0 */
            {
                return Zero[(KDuint32)sx >> 31];
            }
            hx = hz + hz;
        }
    }
    hz = hx - hy;
    if(hz >= 0)
    {
        hx = hz;
    }
    /* convert back to floating value and restore the sign */
    if(hx == 0) /* return sign(x)*0 */
    {
        return Zero[(KDuint32)sx >> 31];
    }
    while(hx < 0x00800000)
    { /* normalize x */
        hx = hx + hx;
        iy -= 1;
    }
    if(iy >= -126)
    { /* normalize output */
        hx = ((hx - 0x00800000) | ((iy + 127) << 23));
        SET_FLOAT_WORD(x, hx | sx);
    }
    else
    { /* subnormal output */
        n = -126 - iy;
        hx >>= n;
        SET_FLOAT_WORD(x, hx | sx);
        x *= 1.0f; /* create necessary signal */
    }
    return x; /* exact output */
}

/* kdAcosKHR
 * Method :                  
 *  acos(x)  = pi/2 - asin(x)
 *  acos(-x) = pi/2 + asin(x)
 * For |x|<=0.5
 *  acos(x) = pi/2 - (x + x*x^2*R(x^2)) (see asin.c)
 * For x>0.5
 *  acos(x) = pi/2 - (pi/2 - 2asin(sqrt((1-x)/2)))
 *      = 2asin(sqrt((1-x)/2))  
 *      = 2s + 2s*z*R(z)    ...z=(1-x)/2, s=sqrt(z)
 *      = 2f + (2c + 2s*z*R(z))
 *     where f=hi part of s, and c = (z-f*f)/(s+f) is the correction term
 *     for f so that f+c ~ sqrt(z).
 * For x<-0.5
 *  acos(x) = pi - 2asin(sqrt((1-|x|)/2))
 *      = pi - 0.5*(s+s*z*R(z)), where z=(1-|x|)/2,s=sqrt(z)
 *
 * Special cases:
 *  if x is NaN, return x itself;
 *  if |x|>1, return NaN with invalid signal.
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdAcosKHR(KDfloat64KHR x)
{
    volatile KDfloat64KHR
        pio2_lo = 6.12323399573676603587e-17; /* 0x3C91A626, 0x33145C07 */
    const KDfloat64KHR
        pS0 = 1.66666666666666657415e-01,  /* 0x3FC55555, 0x55555555 */
        pS1 = -3.25565818622400915405e-01, /* 0xBFD4D612, 0x03EB6F7D */
        pS2 = 2.01212532134862925881e-01,  /* 0x3FC9C155, 0x0E884455 */
        pS3 = -4.00555345006794114027e-02, /* 0xBFA48228, 0xB5688F3B */
        pS4 = 7.91534994289814532176e-04,  /* 0x3F49EFE0, 0x7501B288 */
        pS5 = 3.47933107596021167570e-05,  /* 0x3F023DE1, 0x0DFDF709 */
        qS1 = -2.40339491173441421878e+00, /* 0xC0033A27, 0x1C8A2D4B */
        qS2 = 2.02094576023350569471e+00,  /* 0x40002AE5, 0x9C598AC8 */
        qS3 = -6.88283971605453293030e-01, /* 0xBFE6066C, 0x1B8D0159 */
        qS4 = 7.70381505559019352791e-02;  /* 0x3FB3B8C5, 0xB12E9282 */

    KDfloat64KHR z, p, q, r, w, s, c, df;
    KDint32 hx, ix;
    GET_HIGH_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix >= 0x3ff00000)
    { /* |x| >= 1 */
        KDint32 lx;
        GET_LOW_WORD(lx, x);
        if(((ix - 0x3ff00000) | lx) == 0)
        { /* |x|==1 */
            if(hx > 0)
            {
                return 0.0;
            } /* acos(1) = 0  */
            else
            {
                return KD_PI_KHR + 2.0 * pio2_lo;
            }
        }
        /* acos(|x|>1) is NaN */
        return KD_NAN;
    }
    if(ix < 0x3fe00000)
    { /* |x| < 0.5 */
        if(ix <= 0x3c600000)
        {
            return KD_PI_2_KHR + pio2_lo; /*if|x|<2**-57*/
        }
        z = x * x;
        p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
        q = 1.0 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
        r = p / q;
        return KD_PI_2_KHR - (x - (pio2_lo - x * r));
    }
    else if(hx < 0)
    { /* x < -0.5 */
        z = (1.0 + x) * 0.5;
        p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
        q = 1.0 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
        s = kdSqrtKHR(z);
        r = p / q;
        w = r * s - pio2_lo;
        return KD_PI_KHR - 2.0 * (s + w);
    }
    else
    { /* x > 0.5 */
        z = (1.0 - x) * 0.5;
        s = kdSqrtKHR(z);
        df = s;
        SET_LOW_WORD(df, 0);
        c = (z - df * df) / (s + df);
        p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
        q = 1.0 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
        r = p / q;
        w = r * s + c;
        return 2.0 * (df + w);
    }
}

/* kdAsinKHR
 * Method :                  
 *  Since  asin(x) = x + x^3/6 + x^5*3/40 + x^7*15/336 + ...
 *  we approximate asin(x) on [0,0.5] by
 *      asin(x) = x + x*x^2*R(x^2)
 *  where
 *      R(x^2) is a rational approximation of (asin(x)-x)/x^3 
 *  and its remez error is bounded by
 *      |(asin(x)-x)/x^3 - R(x^2)| < 2^(-58.75)
 *
 *  For x in [0.5,1]
 *      asin(x) = pi/2-2*asin(sqrt((1-x)/2))
 *  Let y = (1-x), z = y/2, s := sqrt(z), and pio2_hi+pio2_lo=pi/2;
 *  then for x>0.98
 *      asin(x) = pi/2 - 2*(s+s*z*R(z))
 *          = pio2_hi - (2*(s+s*z*R(z)) - pio2_lo)
 *  For x<=0.98, let pio4_hi = pio2_hi/2, then
 *      f = hi part of s;
 *      c = sqrt(z) - f = (z-f*f)/(s+f)     ...f+c=sqrt(z)
 *  and
 *      asin(x) = pi/2 - 2*(s+s*z*R(z))
 *          = pio4_hi+(pio4-2s)-(2s*z*R(z)-pio2_lo)
 *          = pio4_hi+(pio4-2f)-(2s*z*R(z)-(pio2_lo+2c))
 *
 * Special cases:
 *  if x is NaN, return x itself;
 *  if |x|>1, return NaN with invalid signal.
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdAsinKHR(KDfloat64KHR x)
{
    const KDfloat64KHR
        huge = 1.000e+300,
        pio2_lo = 6.12323399573676603587e-17, /* 0x3C91A626, 0x33145C07 */
        pio4_hi = 7.85398163397448278999e-01, /* 0x3FE921FB, 0x54442D18 */
        /* coefficient for R(x^2) */
        pS0 = 1.66666666666666657415e-01,  /* 0x3FC55555, 0x55555555 */
        pS1 = -3.25565818622400915405e-01, /* 0xBFD4D612, 0x03EB6F7D */
        pS2 = 2.01212532134862925881e-01,  /* 0x3FC9C155, 0x0E884455 */
        pS3 = -4.00555345006794114027e-02, /* 0xBFA48228, 0xB5688F3B */
        pS4 = 7.91534994289814532176e-04,  /* 0x3F49EFE0, 0x7501B288 */
        pS5 = 3.47933107596021167570e-05,  /* 0x3F023DE1, 0x0DFDF709 */
        qS1 = -2.40339491173441421878e+00, /* 0xC0033A27, 0x1C8A2D4B */
        qS2 = 2.02094576023350569471e+00,  /* 0x40002AE5, 0x9C598AC8 */
        qS3 = -6.88283971605453293030e-01, /* 0xBFE6066C, 0x1B8D0159 */
        qS4 = 7.70381505559019352791e-02;  /* 0x3FB3B8C5, 0xB12E9282 */

    KDfloat64KHR t = 0.0, w, p, q, s;
    KDint32 hx, ix;
    GET_HIGH_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix >= 0x3ff00000)
    { /* |x|>= 1 */
        KDuint32 lx;
        GET_LOW_WORD(lx, x);
        if(((ix - 0x3ff00000) | lx) == 0)
        {
            /* asin(1)=+-pi/2 with inexact */
            return x * KD_PI_2_KHR + x * pio2_lo;
        }
        /* asin(|x|>1) is NaN */
        return KD_NAN;
    }
    else if(ix < 0x3fe00000)
    { /* |x|<0.5 */
        if(ix < 0x3e500000)
        { /* if |x| < 2**-26 */
            if(huge + x > 1.0)
            {
                return x; /* return x with inexact if x!=0*/
            }
        }
        t = x * x;
        p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
        q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
        w = p / q;
        return x + x * w;
    }
    /* 1> |x|>= 0.5 */
    w = 1.0 - kdFabsKHR(x);
    t = w * 0.5;
    p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
    q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
    s = kdSqrtKHR(t);
    if(ix >= 0x3FEF3333)
    { /* if |x| > 0.975 */
        w = p / q;
        t = KD_PI_2_KHR - (2.0 * (s + s * w) - pio2_lo);
    }
    else
    {
        w = s;
        SET_LOW_WORD(w, 0);
        KDfloat64KHR c = (t - w * w) / (s + w);
        KDfloat64KHR r = p / q;
        p = 2.0 * s * r - (pio2_lo - 2.0 * c);
        q = pio4_hi - 2.0 * w;
        t = pio4_hi - (p - q);
    }
    if(hx > 0)
    {
        return t;
    }
    else
    {
        return -t;
    }
}

/* kdAtanKHR
 * Method:
 *   1. Reduce x to positive by atan(x) = -atan(-x).
 *   2. According to the integer k=4t+0.25 chopped, t=x, the argument
 *      is further reduced to one of the following intervals and the
 *      arctangent of t is evaluated by the corresponding formula:
 *
 *      [0,7/16]      atan(x) = t-t^3*(a1+t^2*(a2+...(a10+t^2*a11)...)
 *      [7/16,11/16]  atan(x) = atan(1/2) + atan( (t-0.5)/(1+t/2) )
 *      [11/16.19/16] atan(x) = atan( 1 ) + atan( (t-1)/(1+t) )
 *      [19/16,39/16] atan(x) = atan(3/2) + atan( (t-1.5)/(1+1.5t) )
 *      [39/16,INF]   atan(x) = atan(INF) + atan( -1/t )
 */
KD_API KDfloat64KHR KD_APIENTRY kdAtanKHR(KDfloat64KHR x)
{
    const KDfloat64KHR atanhi[] = {
        4.63647609000806093515e-01, /* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
        7.85398163397448278999e-01, /* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
        9.82793723247329054082e-01, /* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
        1.57079632679489655800e+00, /* atan(inf)hi 0x3FF921FB, 0x54442D18 */
    };

    const KDfloat64KHR atanlo[] = {
        2.26987774529616870924e-17, /* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
        3.06161699786838301793e-17, /* atan(1.0)lo 0x3C81A626, 0x33145C07 */
        1.39033110312309984516e-17, /* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
        6.12323399573676603587e-17, /* atan(inf)lo 0x3C91A626, 0x33145C07 */
    };

    const KDfloat64KHR aT[] = {
        3.33333333333329318027e-01,  /* 0x3FD55555, 0x5555550D */
        -1.99999999998764832476e-01, /* 0xBFC99999, 0x9998EBC4 */
        1.42857142725034663711e-01,  /* 0x3FC24924, 0x920083FF */
        -1.11111104054623557880e-01, /* 0xBFBC71C6, 0xFE231671 */
        9.09088713343650656196e-02,  /* 0x3FB745CD, 0xC54C206E */
        -7.69187620504482999495e-02, /* 0xBFB3B0F2, 0xAF749A6D */
        6.66107313738753120669e-02,  /* 0x3FB10D66, 0xA0D03D51 */
        -5.83357013379057348645e-02, /* 0xBFADDE2D, 0x52DEFD9A */
        4.97687799461593236017e-02,  /* 0x3FA97B4B, 0x24760DEB */
        -3.65315727442169155270e-02, /* 0xBFA2B444, 0x2C6A6C2F */
        1.62858201153657823623e-02,  /* 0x3F90AD3A, 0xE322DA11 */
    };

    const KDfloat64KHR huge = 1.000e+300;
    KDfloat64KHR w, s1, s2, z;
    KDint32 ix, hx, id;

    GET_HIGH_WORD(hx, x);
    ix = hx & KDINT32_MAX;
    if(ix >= 0x44100000)
    { /* if |x| >= 2^66 */
        KDint32 low;
        GET_LOW_WORD(low, x);
        if(ix > 0x7ff00000 || (ix == 0x7ff00000 && (low != 0)))
        {
            return x + x; /* NaN */
        }
        if(hx > 0)
        {
            return atanhi[3] + *(volatile KDfloat64KHR *)&atanlo[3];
        }
        else
        {
            return -atanhi[3] - *(volatile KDfloat64KHR *)&atanlo[3];
        }
    }
    if(ix < 0x3fdc0000)
    { /* |x| < 0.4375 */
        if(ix < 0x3e400000)
        { /* |x| < 2^-27 */
            if(huge + x > 1.0)
            {
                return x; /* raise inexact */
            }
        }
        id = -1;
    }
    else
    {
        x = kdFabsKHR(x);
        if(ix < 0x3ff30000)
        { /* |x| < 1.1875 */
            if(ix < 0x3fe60000)
            { /* 7/16 <=|x|<11/16 */
                id = 0;
                x = (2.0 * x - 1.0) / (2.0 + x);
            }
            else
            { /* 11/16<=|x|< 19/16 */
                id = 1;
                x = (x - 1.0) / (x + 1.0);
            }
        }
        else
        {
            if(ix < 0x40038000)
            { /* |x| < 2.4375 */
                id = 2;
                x = (x - 1.5) / (1.0 + 1.5 * x);
            }
            else
            { /* 2.4375 <= |x| < 2^66 */
                id = 3;
                x = -1.0 / x;
            }
        }
    }
    /* end of argument reduction */
    z = x * x;
    w = z * z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
    s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] + w * aT[10])))));
    s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
    if(id < 0)
    {
        return x - x * (s1 + s2);
    }
    else
    {
        z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
        return (hx < 0) ? -z : z;
    }
}


/* kdAtan2KHR
 * Method :
 *  1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *  2. Reduce x to positive by (if x and y are unexceptional): 
 *      ARG (x+iy) = arctan(y/x)       ... if x > 0,
 *      ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *  ATAN2((anything), NaN ) is NaN;
 *  ATAN2(NAN , (anything) ) is NaN;
 *  ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *  ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *  ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *  ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *  ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *  ATAN2(+-INF,+INF ) is +-pi/4 ;
 *  ATAN2(+-INF,-INF ) is +-3pi/4;
 *  ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("signed-integer-overflow")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdAtan2KHR(KDfloat64KHR y, KDfloat64KHR x)
{
    volatile KDfloat64KHR
        tiny = 1.0e-300,
        pi_lo = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */

    KDfloat64KHR z;
    KDint32 k, m, hx, hy, ix, iy;
    KDuint32 lx, ly;

    EXTRACT_WORDS(hx, lx, x);
    ix = hx & KDINT32_MAX;
    EXTRACT_WORDS(hy, ly, y);
    iy = hy & KDINT32_MAX;
    if(((ix | ((lx | -(KDint)lx) >> 31)) > 0x7ff00000) || ((iy | ((ly | -(KDint)ly) >> 31)) > 0x7ff00000)) /* x or y is NaN */
    {
        return x + y;
    }
    if(((hx - 0x3ff00000) | lx) == 0)
    {
        return kdAtanKHR(y); /* x=1.0 */
    }
    m = (((KDuint)hy >> 31) & 1) | ((hx >> 30) & 2); /* 2*sign(x)+sign(y) */

    /* when y = 0 */
    if((iy | ly) == 0)
    {
        switch(m)
        {
            case 0:
            case 1:
            {
                return y; /* atan(+-0,+anything)=+-0 */
            }
            case 2:
            {
                return KD_PI_KHR + tiny; /* atan(+0,-anything) = pi */
            }
            case 3:
            {
                return -KD_PI_KHR - tiny; /* atan(-0,-anything) =-pi */
            }
            default:
            {
            }
        }
    }
    /* when x = 0 */
    if((ix | lx) == 0)
    {
        return (hy < 0) ? -KD_PI_2_KHR - tiny : KD_PI_2_KHR + tiny;
    }

    /* when x is INF */
    if(ix == 0x7ff00000)
    {
        if(iy == 0x7ff00000)
        {
            switch(m)
            {
                case 0:
                {
                    return KD_PI_4_KHR + tiny; /* atan(+INF,+INF) */
                }
                case 1:
                {
                    return -KD_PI_4_KHR - tiny; /* atan(-INF,+INF) */
                }
                case 2:
                {
                    return 3.0 * KD_PI_4_KHR + tiny; /*atan(+INF,-INF)*/
                }
                case 3:
                {
                    return -3.0 * KD_PI_4_KHR - tiny; /*atan(-INF,-INF)*/
                }
                default:
                {
                }
            }
        }
        else
        {
            switch(m)
            {
                case 0:
                {
                    return 0.0; /* atan(+...,+INF) */
                }
                case 1:
                {
                    return -0.0; /* atan(-...,+INF) */
                }
                case 2:
                {
                    return KD_PI_KHR + tiny; /* atan(+...,-INF) */
                }
                case 3:
                {
                    return -KD_PI_KHR - tiny; /* atan(-...,-INF) */
                }
                default:
                {
                }
            }
        }
    }
    /* when y is INF */
    if(iy == 0x7ff00000)
    {
        return (hy < 0) ? -KD_PI_2_KHR - tiny : KD_PI_2_KHR + tiny;
    }

    /* compute y/x */
    k = (iy - ix) >> 20;
    if(k > 60)
    { /* |y/x| >  2**60 */
        z = KD_PI_2_KHR + 0.5 * pi_lo;
        m &= 1;
    }
    else if(hx < 0 && k < -60)
    {
        z = 0.0; /* 0 > |y|/x > -2**-60 */
    }
    else
    {
        z = kdAtanKHR(kdFabsKHR(y / x)); /* safe to do y/x */
    }
    switch(m)
    {
        case 0:
        {
            return z; /* atan(+,+) */
        }
        case 1:
        {
            return -z; /* atan(-,+) */
        }
        case 2:
        {
            return KD_PI_KHR - (z - pi_lo); /* atan(+,-) */
        }
        default: /* case 3 */
        {
            return (z - pi_lo) - KD_PI_KHR; /* atan(-,-) */
        }
    }
}

/* kdCosKHR
 * Method:
 *      Let S,C and T denote the sin, cos and tan respectively on
 *  [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *  in [-pi/4 , +pi/4], and let n = k mod 4.
 *  We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *      0          S       C         T
 *      1          C      -S        -1/T
 *      2         -S      -C         T
 *      3         -C       S        -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *  TRIG(x) returns trig(x) nearly rounded
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdCosKHR(KDfloat64KHR x)
{
    KDfloat64KHR y[2];
    KDint32 n, ix;

    /* High word of x. */
    GET_HIGH_WORD(ix, x);

    /* |x| ~< pi/4 */
    ix &= KDINT32_MAX;
    if(ix <= 0x3fe921fb)
    {
        if(ix < 0x3e46a09e) /* if x < 2**-27 * sqrt(2) */
        {
            if(((KDint)x) == 0)
            {
                return 1.0; /* generate inexact */
            }
        }
        return __kdCosKernel(x, 0.0);
    }

    /* cos(Inf or NaN) is NaN */
    else if(ix >= 0x7ff00000)
    {
        return KD_NAN;
    }

    /* argument reduction needed */
    else
    {
        n = __kdRemPio2(x, y);
        switch(n & 3)
        {
            case 0:
            {
                return __kdCosKernel(y[0], y[1]);
            }
            case 1:
            {
                return -__kdSinKernel(y[0], y[1], 1);
            }
            case 2:
            {
                return -__kdCosKernel(y[0], y[1]);
            }
            default:
            {
                return __kdSinKernel(y[0], y[1], 1);
            }
        }
    }
}

/* kdSinKHR
 * Method:
 *      Let S,C and T denote the sin, cos and tan respectively on
 *  [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *  in [-pi/4 , +pi/4], and let n = k mod 4.
 *  We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *      0          S       C         T
 *      1          C      -S        -1/T
 *      2         -S      -C         T
 *      3         -C       S        -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *  TRIG(x) returns trig(x) nearly rounded
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdSinKHR(KDfloat64KHR x)
{
    KDfloat64KHR y[2];
    KDint32 n, ix;

    /* High word of x. */
    GET_HIGH_WORD(ix, x);

    /* |x| ~< pi/4 */
    ix &= KDINT32_MAX;
    if(ix <= 0x3fe921fb)
    {
        if(ix < 0x3e500000) /* |x| < 2**-26 */
        {
            if((KDint)x == 0)
            {
                return x;
            }
        } /* generate inexact */
        return __kdSinKernel(x, 0.0, 0);
    }

    /* sin(Inf or NaN) is NaN */
    else if(ix >= 0x7ff00000)
    {
        return KD_NAN;
    }
    /* argument reduction needed */
    else
    {
        n = __kdRemPio2(x, y);
        switch(n & 3)
        {
            case 0:
            {
                return __kdSinKernel(y[0], y[1], 1);
            }
            case 1:
            {
                return __kdCosKernel(y[0], y[1]);
            }
            case 2:
            {
                return -__kdSinKernel(y[0], y[1], 1);
            }
            default:
            {
                return -__kdCosKernel(y[0], y[1]);
            }
        }
    }
}

/* kdTanKHR
 * Method:
 *      Let S,C and T denote the sin, cos and tan respectively on
 *  [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *  in [-pi/4 , +pi/4], and let n = k mod 4.
 *  We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *      0          S       C         T
 *      1          C      -S        -1/T
 *      2         -S      -C         T
 *      3         -C       S        -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *  TRIG(x) returns trig(x) nearly rounded
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdTanKHR(KDfloat64KHR x)
{
    KDfloat64KHR y[2];
    KDint32 n, ix;

    /* High word of x. */
    GET_HIGH_WORD(ix, x);

    /* |x| ~< pi/4 */
    ix &= KDINT32_MAX;
    if(ix <= 0x3fe921fb)
    {
        if(ix < 0x3e400000) /* x < 2**-27 */
        {
            if((KDint)x == 0)
            {
                return x; /* generate inexact */
            }
        }
        return __kdTanKernel(x, 0.0, 1);
    }

    /* tan(Inf or NaN) is NaN */
    else if(ix >= 0x7ff00000)
    {
        return KD_NAN;
    }

    /* argument reduction needed */
    else
    {
        n = __kdRemPio2(x, y);
        return __kdTanKernel(y[0], y[1], 1 - ((n & 1) << 1)); /* 1 -- n even -1 -- n odd */
    }
}

/* kdExpKHR
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *  Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.  
 *
 *      Here r will be represented as r = hi-lo for better 
 *  accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *  the interval [0,0.34658]:
 *  Write
 *      R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Remes algorithm on [0,0.34658] to generate 
 *  a polynomial of degree 5 to approximate R. The maximum error 
 *  of this polynomial approximation is bounded by 2**-59. In
 *  other words,
 *      R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *      (where z=r*r, and the values of P1 to P5 are listed below)
 *  and
 *      |                  5          |     -59
 *      | 2.0+P1*z+...+P5*z   -  R(z) | <= 2 
 *      |                             |
 *  The computation of exp(r) thus becomes
 *                             2*r
 *      exp(r) = 1 + -------
 *                    R - r
 *                                 r*R1(r)  
 *             = 1 + r + ----------- (for better accuracy)
 *                        2 - R1(r)
 *  where
 *                   2       4             10
 *      R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *  
 *   3. Scale back to obtain exp(x):
 *  From step 1, we have
 *     exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *  exp(INF) is INF, exp(NaN) is NaN;
 *  exp(-INF) is 0, and
 *  for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *  according to an error analysis, the error is always less than
 *  1 ulp (unit in the last place).
 *
 * Misc. info.
 *  For IEEE double 
 *      if x >  7.09782712893383973096e+02 then exp(x) overflow
 *      if x < -7.45133219101941108420e+02 then exp(x) underflow
 */
KD_API KDfloat64KHR KD_APIENTRY kdExpKHR(KDfloat64KHR x)
{
    const KDfloat64KHR
        halF[2] = {
            0.5,
            -0.5,
        },
        o_threshold = 7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
        u_threshold = -7.45133219101941108420e+02, /* 0xc0874910, 0xD52D3051 */
        ln2HI[2] = {
            6.93147180369123816490e-01, /* 0x3fe62e42, 0xfee00000 */
            -6.93147180369123816490e-01,
        }, /* 0xbfe62e42, 0xfee00000 */
        ln2LO[2] = {
            1.90821492927058770002e-10, /* 0x3dea39ef, 0x35793c76 */
            -1.90821492927058770002e-10,
        },                                   /* 0xbdea39ef, 0x35793c76 */
        invln2 = 1.44269504088896338700e+00, /* 0x3ff71547, 0x652b82fe */
        P1 = 1.66666666666666019037e-01,     /* 0x3FC55555, 0x5555553E */
        P2 = -2.77777777770155933842e-03,    /* 0xBF66C16C, 0x16BEBD93 */
        P3 = 6.61375632143793436117e-05,     /* 0x3F11566A, 0xAF25DE2C */
        P4 = -1.65339022054652515390e-06,    /* 0xBEBBBD41, 0xC5D26BF1 */
        P5 = 4.13813679705723846039e-08;     /* 0x3E663769, 0x72BEA4D0 */

    volatile KDfloat64KHR
        huge = 1.0e+300,
        twom1000 = 9.33263618503218878990e-302; /* 2**-1000=0x01700000,0*/

    KDfloat64KHR y, hi = 0.0, lo = 0.0, c, t, twopk;
    KDint32 k = 0, xsb;
    KDuint32 hx;

    GET_HIGH_WORD(hx, x);
    xsb = (hx >> 31) & 1; /* sign bit of x */
    hx &= KDINT32_MAX;    /* high word of |x| */

    /* filter out non-finite argument */
    if(hx >= 0x40862E42)
    { /* if |x|>=709.78... */
        if(hx >= 0x7ff00000)
        {
            KDuint32 lx;
            GET_LOW_WORD(lx, x);
            if(((hx & 0xfffff) | lx) != 0)
            {
                return x + x; /* NaN */
            }
            else
            {
                return (xsb == 0) ? x : 0.0; /* exp(+-inf)={inf,0} */
            }
        }
        if(x > o_threshold)
        {
            return huge * huge; /* overflow */
        }
        if(x < u_threshold)
        {
            return twom1000 * twom1000; /* underflow */
        }
    }

    /* argument reduction */
    if(hx > 0x3fd62e42)
    { /* if  |x| > 0.5 ln2 */
        if(hx < 0x3FF0A2B2)
        { /* and |x| < 1.5 ln2 */
            hi = x - ln2HI[xsb];
            lo = ln2LO[xsb];
            k = 1 - xsb - xsb;
        }
        else
        {
            k = (KDint)(invln2 * x + halF[xsb]);
            t = k;
            hi = x - t * ln2HI[0]; /* t*ln2HI is exact here */
            lo = t * ln2LO[0];
        }
        x = hi - lo;
    }
    else if(hx < 0x3e300000)
    { /* when |x|<2**-28 */
        if(huge + x > 1.0)
        {
            return 1.0 + x; /* trigger inexact */
        }
    }
    else
    {
        k = 0;
    }

    /* x is now in primary range */
    t = x * x;
    if(k >= -1021)
    {
        INSERT_WORDS(twopk, 0x3ff00000 + ((KDuint32)k << 20), 0);
    }
    else
    {
        INSERT_WORDS(twopk, 0x3ff00000 + ((k + 1000) << 20), 0);
    }
    c = x - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
    if(k == 0)
    {
        return 1.0 - ((x * c) / (c - 2.0) - x);
    }
    else
    {
        y = 1.0 - ((lo - (x * c) / (2.0 - c)) - hi);
    }
    if(k >= -1021)
    {
        if(k == 1024)
        {
            return y * 2.0 * 8.98847e+307;
        }
        return y * twopk;
    }
    else
    {
        return y * twopk * twom1000;
    }
}

/* kdLogKHR
 * Method :                  
 *   1. Argument Reduction: find k and f such that 
 *          x = 2^k * (1+f), 
 *     where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *   2. Approximation of log(1+f).
 *  Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *       = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *           = 2s + s*R
 *      We use a special Reme algorithm on [0,0.1716] to generate 
 *  a polynomial of degree 14 to approximate R The maximum error 
 *  of this polynomial approximation is bounded by 2**-58.45. In
 *  other words,
 *              2      4      6      8      10      12      14
 *      R(z) ~ Lg1*s +Lg2*s +Lg3*s +Lg4*s +Lg5*s  +Lg6*s  +Lg7*s
 *      (the values of Lg1 to Lg7 are listed in the program)
 *  and
 *      |      2          14          |     -58.45
 *      | Lg1*s +...+Lg7*s    -  R(z) | <= 2 
 *      |                             |
 *  Note that 2s = f - s*f = f - hfsq + s*hfsq, where hfsq = f*f/2.
 *  In order to guarantee error in log below 1ulp, we compute log
 *  by
 *      log(1+f) = f - s*(f - R)    (if f is not too large)
 *      log(1+f) = f - (hfsq - s*(hfsq+R)). (better accuracy)
 *  
 *  3. Finally,  log(x) = k*ln2 + log(1+f).  
 *              = k*ln2_hi+(f-(hfsq-(s*(hfsq+R)+k*ln2_lo)))
 *     Here ln2 is split into two floating point number: 
 *          ln2_hi + ln2_lo,
 *     where n*ln2_hi is always exact for |n| < 2000.
 *
 * Special cases:
 *  log(x) is NaN with signal if x < 0 (including -INF) ; 
 *  log(+INF) is +INF; log(0) is -INF with signal;
 *  log(NaN) is that NaN with no signal.
 *
 * Accuracy:
 *  according to an error analysis, the error is always less than
 *  1 ulp (unit in the last place).
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdLogKHR(KDfloat64KHR x)
{
    const KDfloat64KHR
        ln2_hi = 6.93147180369123816490e-01, /* 3fe62e42 fee00000 */
        ln2_lo = 1.90821492927058770002e-10, /* 3dea39ef 35793c76 */
        two54 = 1.80143985094819840000e+16,  /* 43500000 00000000 */
        Lg1 = 6.666666666666735130e-01,      /* 3FE55555 55555593 */
        Lg2 = 3.999999999940941908e-01,      /* 3FD99999 9997FA04 */
        Lg3 = 2.857142874366239149e-01,      /* 3FD24924 94229359 */
        Lg4 = 2.222219843214978396e-01,      /* 3FCC71C5 1D8E78AF */
        Lg5 = 1.818357216161805012e-01,      /* 3FC74664 96CB03DE */
        Lg6 = 1.531383769920937332e-01,      /* 3FC39A09 D078C69F */
        Lg7 = 1.479819860511658591e-01;      /* 3FC2F112 DF3E5244 */


    KDfloat64KHR f, s, z, R, w, t1, t2, dk;
    KDint32 k, hx, i, j;
    KDuint32 lx;

    EXTRACT_WORDS(hx, lx, x);

    k = 0;
    if(hx < 0x00100000)
    { /* x < 2**-1022  */
        if(((hx & KDINT32_MAX) | lx) == 0)
        {
            return -KD_HUGE_VAL_KHR; /* log(+-0)=-inf */
        }
        if(hx < 0)
        {
            return KD_NAN; /* log(-#) = NaN */
        }
        k -= 54;
        x *= two54; /* subnormal number, scale up x */
        GET_HIGH_WORD(hx, x);
    }
    if(hx >= 0x7ff00000)
    {
        return x + x;
    }
    k += (hx >> 20) - 1023;
    hx &= 0x000fffff;
    i = (hx + 0x95f64) & 0x100000;
    SET_HIGH_WORD(x, hx | (i ^ 0x3ff00000)); /* normalize x or x/2 */
    k += (i >> 20);
    f = x - 1.0;
    if((0x000fffff & (2 + hx)) < 3)
    { /* -2**-20 <= f < 2**-20 */
        if(f == 0.0)
        {
            if(k == 0)
            {
                return 0.0;
            }
            else
            {
                dk = (KDfloat64KHR)k;
                return dk * ln2_hi + dk * ln2_lo;
            }
        }
        R = f * f * (0.5 - 0.33333333333333333 * f);
        if(k == 0)
        {
            return f - R;
        }
        else
        {
            dk = (KDfloat64KHR)k;
            return dk * ln2_hi - ((R - dk * ln2_lo) - f);
        }
    }
    s = f / (2.0 + f);
    dk = (KDfloat64KHR)k;
    z = s * s;
    i = hx - 0x6147a;
    w = z * z;
    j = 0x6b851 - hx;
    t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
    t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
    i |= j;
    R = t2 + t1;
    if(i > 0)
    {
        KDfloat64KHR hfsq = 0.5 * f * f;
        if(k == 0)
        {
            return f - (hfsq - s * (hfsq + R));
        }
        else
        {
            return dk * ln2_hi - ((hfsq - (s * (hfsq + R) + dk * ln2_lo)) - f);
        }
    }
    else
    {
        if(k == 0)
        {
            return f - s * (f - R);
        }
        else
        {
            return dk * ln2_hi - ((s * (f - R) - dk * ln2_lo) - f);
        }
    }
}

KD_API KDfloat64KHR KD_APIENTRY kdFabsKHR(KDfloat64KHR x)
{
    __KDFloat64Shape shape;
    shape.f64 = x;
    shape.u64 &= KDINT64_MAX;
    return shape.f64;
}

/* kdPowKHR
 * Method:    n
 *  Let x =  2   * (1+f)
 *  1. Compute and return log2(x) in two pieces:
 *      log2(x) = w1 + w2,
 *     where w1 has 53-24 = 29 bit trailing zeros.
 *  2. Perform y*log2(x) = n+y' by simulating multi-precision 
 *     arithmetic, where |y'|<=0.5.
 *  3. Return x**y = 2**n*exp(y'*log2)
 *
 * Special cases:
 *  1.  (anything) ** 0  is 1
 *  2.  (anything) ** 1  is itself
 *  3.  (anything) ** NAN is NAN except 1 ** NAN = 1
 *  4.  NAN ** (anything except 0) is NAN
 *  5.  +-(|x| > 1) **  +INF is +INF
 *  6.  +-(|x| > 1) **  -INF is +0
 *  7.  +-(|x| < 1) **  +INF is +0
 *  8.  +-(|x| < 1) **  -INF is +INF
 *  9.  +-1         ** +-INF is 1
 *  10. +0 ** (+anything except 0, NAN)               is +0
 *  11. -0 ** (+anything except 0, NAN, odd integer)  is +0
 *  12. +0 ** (-anything except 0, NAN)               is +INF
 *  13. -0 ** (-anything except 0, NAN, odd integer)  is +INF
 *  14. -0 ** (odd integer) = -( +0 ** (odd integer) )
 *  15. +INF ** (+anything except 0,NAN) is +INF
 *  16. +INF ** (-anything except 0,NAN) is +0
 *  17. -INF ** (anything)  = -0 ** (-anything)
 *  18. (-anything) ** (integer) is (-1)**(integer)*(+anything**integer)
 *  19. (-anything except 0 and inf) ** (non-integer) is NAN
 *
 * Accuracy:
 *  pow(x,y) returns x**y nearly rounded. In particular
 *          pow(integer,integer)
 *  always returns the correct integer provided it is 
 *  representable.
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdPowKHR(KDfloat64KHR x, KDfloat64KHR y)
{
    const KDfloat64KHR
        bp[] = {
            1.0,
            1.5,
        },
        dp_h[] = {
            0.0,
            5.84962487220764160156e-01,
        }, /* 0x3FE2B803, 0x40000000 */
        dp_l[] = {
            0.0,
            1.35003920212974897128e-08,
        },                          /* 0x3E4CFDEB, 0x43CFD006 */
        two53 = 9007199254740992.0, /* 0x43400000, 0x00000000 */
        huge = 1.0e300, tiny = 1.0e-300,
        /* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
        L1 = 5.99999999999994648725e-01,      /* 0x3FE33333, 0x33333303 */
        L2 = 4.28571428578550184252e-01,      /* 0x3FDB6DB6, 0xDB6FABFF */
        L3 = 3.33333329818377432918e-01,      /* 0x3FD55555, 0x518F264D */
        L4 = 2.72728123808534006489e-01,      /* 0x3FD17460, 0xA91D4101 */
        L5 = 2.30660745775561754067e-01,      /* 0x3FCD864A, 0x93C9DB65 */
        L6 = 2.06975017800338417784e-01,      /* 0x3FCA7E28, 0x4A454EEF */
        P1 = 1.66666666666666019037e-01,      /* 0x3FC55555, 0x5555553E */
        P2 = -2.77777777770155933842e-03,     /* 0xBF66C16C, 0x16BEBD93 */
        P3 = 6.61375632143793436117e-05,      /* 0x3F11566A, 0xAF25DE2C */
        P4 = -1.65339022054652515390e-06,     /* 0xBEBBBD41, 0xC5D26BF1 */
        P5 = 4.13813679705723846039e-08,      /* 0x3E663769, 0x72BEA4D0 */
        lg2 = 6.93147180559945286227e-01,     /* 0x3FE62E42, 0xFEFA39EF */
        lg2_h = 6.93147182464599609375e-01,   /* 0x3FE62E43, 0x00000000 */
        lg2_l = -1.90465429995776804525e-09,  /* 0xBE205C61, 0x0CA86C39 */
        ovt = 8.0085662595372944372e-0017,    /* -(1024-log2(ovfl+.5ulp)) */
        cp = 9.61796693925975554329e-01,      /* 0x3FEEC709, 0xDC3A03FD =2/(3ln2) */
        cp_h = 9.61796700954437255859e-01,    /* 0x3FEEC709, 0xE0000000 =(float)cp */
        cp_l = -7.02846165095275826516e-09,   /* 0xBE3E2FE0, 0x145B01F5 =tail of cp_h*/
        ivln2 = 1.44269504088896338700e+00,   /* 0x3FF71547, 0x652B82FE =1/ln2 */
        ivln2_h = 1.44269502162933349609e+00, /* 0x3FF71547, 0x60000000 =24b 1/ln2*/
        ivln2_l = 1.92596299112661746887e-08; /* 0x3E54AE0B, 0xF85DDF44 =1/ln2 tail*/

    KDfloat64KHR z, ax, p_h, p_l;
    KDfloat64KHR y1, t1, t2, r, s, t, u, v, w;
    KDint32 i, j, k, yisint, n;
    KDint32 hx, hy, ix, iy;
    KDuint32 lx, ly;

    EXTRACT_WORDS(hx, lx, x);
    EXTRACT_WORDS(hy, ly, y);
    ix = hx & KDINT32_MAX;
    iy = hy & KDINT32_MAX;

    /* y==zero: x**0 = 1 */
    if((iy | ly) == 0)
    {
        return 1.0;
    }

    /* x==1: 1**y = 1, even if y is NaN */
    if(hx == 0x3ff00000 && lx == 0)
    {
        return 1.0;
    }

    /* y!=zero: result is NaN if either arg is NaN */
    if(ix > 0x7ff00000 || ((ix == 0x7ff00000) && (lx != 0)) ||
        iy > 0x7ff00000 || ((iy == 0x7ff00000) && (ly != 0)))
    {
        return (x + 0.0) + (y + 0.0);
    }
    /* determine if y is an odd int when x < 0
     * yisint = 0   ... y is not an integer
     * yisint = 1   ... y is an odd int
     * yisint = 2   ... y is an even int
     */
    yisint = 0;
    if(hx < 0)
    {
        if(iy >= 0x43400000)
        {
            yisint = 2; /* even integer y */
        }
        else if(iy >= 0x3ff00000)
        {
            k = (iy >> 20) - 0x3ff; /* exponent */
            if(k > 20)
            {
                j = ly >> (52 - k);
                if((j << (52 - k)) == (KDint32)ly)
                {
                    yisint = 2 - (j & 1);
                }
            }
            else if(ly == 0)
            {
                j = iy >> (20 - k);
                if((j << (20 - k)) == iy)
                {
                    yisint = 2 - (j & 1);
                }
            }
        }
    }

    /* special value of y */
    if(ly == 0)
    {
        if(iy == 0x7ff00000)
        { /* y is +-inf */
            if(((ix - 0x3ff00000) | lx) == 0)
            {
                return 1.0; /* (-1)**+-inf is 1 */
            }
            else if(ix >= 0x3ff00000) /* (|x|>1)**+-inf = inf,0 */
            {
                return (hy >= 0) ? y : 0.0;
            }
            else /* (|x|<1)**-,+inf = inf,0 */
            {
                return (hy < 0) ? -y : 0.0;
            }
        }
        if(iy == 0x3ff00000)
        { /* y is  +-1 */
            if(hy < 0)
            {
                return 1.0 / x;
            }
            else
            {
                return x;
            }
        }
        if(hy == 0x40000000)
        {
            return x * x; /* y is  2 */
        }
        if(hy == 0x3fe00000)
        {               /* y is  0.5 */
            if(hx >= 0) /* x >= +0 */
            {
                return kdSqrtKHR(x);
            }
        }
    }

    ax = kdFabsKHR(x);
    /* special value of x */
    if(lx == 0)
    {
        if(ix == 0x7ff00000 || ix == 0 || ix == 0x3ff00000)
        {
            z = ax; /*x is +-0,+-inf,+-1*/
            if(hy < 0)
            {
                z = 1.0 / z; /* z = (1/|x|) */
            }
            if(hx < 0)
            {
                if(((ix - 0x3ff00000) | yisint) == 0)
                {
                    z = KD_NAN; /* (-1)**non-int is NaN */
                }
                else if(yisint == 1)
                {
                    z = -z; /* (x<0)**odd = -(|x|**odd) */
                }
            }
            return z;
        }
    }

    n = ((KDuint32)hx >> 31) - 1;

    /* (x<0)**(non-int) is NaN */
    if((n | yisint) == 0)
    {
        return KD_NAN;
    }

    s = 1.0; /* s (sign of result -ve**odd) = -1 else = 1 */
    if((n | (yisint - 1)) == 0)
    {
        s = -1.0; /* (-ve)**(odd int) */
    }

    /* |y| is huge */
    if(iy > 0x41e00000)
    { /* if |y| > 2**31 */
        if(iy > 0x43f00000)
        { /* if |y| > 2**64, must o/uflow */
            if(ix <= 0x3fefffff)
            {
                return (hy < 0) ? huge * huge : tiny * tiny;
            }
            if(ix >= 0x3ff00000)
            {
                return (hy > 0) ? huge * huge : tiny * tiny;
            }
        }
        /* over/underflow if x is not close to one */
        if(ix < 0x3fefffff)
        {
            return (hy < 0) ? s * huge * huge : s * tiny * tiny;
        }
        if(ix > 0x3ff00000)
        {
            return (hy > 0) ? s * huge * huge : s * tiny * tiny;
        }
        /* now |1-x| is tiny <= 2**-20, suffice to compute 
       log(x) by x-x^2/2+x^3/3-x^4/4 */
        t = ax - 1.0; /* t has 20 trailing zeros */
        w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25));
        u = ivln2_h * t; /* ivln2_h has 21 sig. bits */
        v = t * ivln2_l - w * ivln2;
        t1 = u + v;
        SET_LOW_WORD(t1, 0);
        t2 = v - (t1 - u);
    }
    else
    {
        KDfloat64KHR ss, s2, s_h, s_l, t_h, t_l;
        n = 0;
        /* take care subnormal number */
        if(ix < 0x00100000)
        {
            ax *= two53;
            n -= 53;
            GET_HIGH_WORD(ix, ax);
        }
        n += ((ix) >> 20) - 0x3ff;
        j = ix & 0x000fffff;
        /* determine interval */
        ix = j | 0x3ff00000; /* normalize ix */
        if(j <= 0x3988E)
        {
            k = 0; /* |x|<sqrt(3/2) */
        }
        else if(j < 0xBB67A)
        {
            k = 1; /* |x|<sqrt(3)   */
        }
        else
        {
            k = 0;
            n += 1;
            ix -= 0x00100000;
        }
        SET_HIGH_WORD(ax, ix);

        /* compute ss = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
        u = ax - bp[k]; /* bp[0]=1.0, bp[1]=1.5 */
        v = 1.0 / (ax + bp[k]);
        ss = u * v;
        s_h = ss;
        SET_LOW_WORD(s_h, 0);
        /* t_h=ax+bp[k] High */
        t_h = 0.0;
        SET_HIGH_WORD(t_h, ((ix >> 1) | 0x20000000) + 0x00080000 + (k << 18));
        t_l = ax - (t_h - bp[k]);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);
        /* compute log(ax) */
        s2 = ss * ss;
        r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 + s2 * L6)))));
        r += s_l * (s_h + ss);
        s2 = s_h * s_h;
        t_h = 3.0 + s2 + r;
        SET_LOW_WORD(t_h, 0);
        t_l = r - ((t_h - 3.0) - s2);
        /* u+v = ss*(1+...) */
        u = s_h * t_h;
        v = s_l * t_h + t_l * ss;
        /* 2/(3log2)*(ss+...) */
        p_h = u + v;
        SET_LOW_WORD(p_h, 0);
        p_l = v - (p_h - u);
        KDfloat64KHR z_h = cp_h * p_h; /* cp_h+cp_l = 2/(3*log2) */
        KDfloat64KHR z_l = cp_l * p_h + p_l * cp + dp_l[k];
        /* log2(ax) = (ss+..)*2/(3*log2) = n + dp_h + z_h + z_l */
        t = (KDfloat64KHR)n;
        t1 = (((z_h + z_l) + dp_h[k]) + t);
        SET_LOW_WORD(t1, 0);
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }

    /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
    y1 = y;
    SET_LOW_WORD(y1, 0);
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    EXTRACT_WORDS(j, i, z);
    if(j >= 0x40900000)
    {                                   /* z >= 1024 */
        if(((j - 0x40900000) | i) != 0) /* if z > 1024 */
        {
            return s * huge * huge; /* overflow */
        }
        else
        {
            if(p_l + ovt > z - p_h)
            {
                return s * huge * huge; /* overflow */
            }
        }
    }
    else if((j & KDINT32_MAX) >= 0x4090cc00)
    {                                   /* z <= -1075 */
        if(((j - 0xc090cc00) | i) != 0) /* z < -1075 */
        {
            return s * tiny * tiny; /* underflow */
        }
        else
        {
            if(p_l <= z - p_h)
            {
                return s * tiny * tiny; /* underflow */
            }
        }
    }
    /*
     * compute 2**(p_h+p_l)
     */
    i = j & KDINT32_MAX;
    k = (i >> 20) - 0x3ff;
    n = 0;
    if(i > 0x3fe00000)
    { /* if |z| > 0.5, set n = [z+0.5] */
        n = j + (0x00100000 >> (k + 1));
        k = ((n & KDINT32_MAX) >> 20) - 0x3ff; /* new k for n */
        t = 0.0;
        SET_HIGH_WORD(t, n & ~(0x000fffff >> k));
        n = ((n & 0x000fffff) | 0x00100000) >> (20 - k);
        if(j < 0)
        {
            n = -n;
        }
        p_h -= t;
    }
    t = p_l + p_h;
    SET_LOW_WORD(t, 0);
    u = t * lg2_h;
    v = (p_l - (t - p_h)) * lg2 + t * lg2_l;
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
    r = (z * t1) / (t1 - 2.0) - (w + z * w);
    z = 1.0 - (r - z);
    GET_HIGH_WORD(j, z);
    j += ((KDuint32)n << 20);
    if((j >> 20) <= 0)
    {
        z = __kdScalbn(z, n); /* subnormal output */
    }
    else
    {
        SET_HIGH_WORD(z, j);
    }
    return s * z;
}

/* kdSqrtKHR
 * Method: 
 *   Bit by bit method using integer arithmetic. (Slow, but portable) 
 *   1. Normalization
 *  Scale x to y in [1,4) with even powers of 2: 
 *  find an integer k such that  1 <= (y=x*2^(2k)) < 4, then
 *      sqrt(x) = 2^k * sqrt(y)
 *   2. Bit by bit computation
 *  Let q  = sqrt(y) truncated to i bit after binary point (q = 1),
 *       i                           0
 *                                     i+1         2
 *      s  = 2*q , and  y  =  2   * ( y - q  ).     (1)
 *       i      i            i                 i
 *                                                        
 *  To compute q    from q , one checks whether 
 *          i+1       i                       
 *
 *                -(i+1) 2
 *          (q + 2      ) <= y.         (2)
 *                i
 *                                -(i+1)
 *  If (2) is false, then q   = q ; otherwise q   = q  + 2      .
 *                 i+1   i             i+1   i
 *
 *  With some algebric manipulation, it is not difficult to see
 *  that (2) is equivalent to 
 *                             -(i+1)
 *          s  +  2       <= y          (3)
 *           i                i
 *
 *  The advantage of (3) is that s  and y  can be computed by 
 *                    i      i
 *  the following recurrence formula:
 *      if (3) is false
 *
 *      s     =  s  ,   y    = y   ;            (4)
 *       i+1      i      i+1    i
 *
 *      otherwise,
 *                         -i                     -(i+1)
 *      s     =  s  + 2  ,  y    = y  -  s  - 2         (5)
 *           i+1      i          i+1    i     i
 *              
 *  One may easily use induction to prove (4) and (5). 
 *  Note. Since the left hand side of (3) contain only i+2 bits,
 *        it does not necessary to do a full (53-bit) comparison 
 *        in (3).
 *   3. Final rounding
 *  After generating the 53 bits result, we compute one more bit.
 *  Together with the remainder, we can decide whether the
 *  result is exact, bigger than 1/2ulp, or less than 1/2ulp
 *  (it will never equal to 1/2ulp).
 *  The rounding mode can be detected by checking whether
 *  huge + tiny is equal to huge, and whether huge - tiny is
 *  equal to huge for some floating point number "huge" and "tiny".
 *      
 * Special cases:
 *  sqrt(+-0) = +-0     ... exact
 *  sqrt(inf) = inf
 *  sqrt(-ve) = NaN     ... with invalid signal
 *  sqrt(NaN) = NaN     ... with invalid signal for signaling NaN
 */
#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KD_API KDfloat64KHR KD_APIENTRY
kdSqrtKHR(KDfloat64KHR x)
{
#ifdef __SSE2__
    KDfloat64KHR result = 0.0;
    _mm_store_sd(&result, _mm_sqrt_sd(_mm_load_sd(&result), _mm_load_sd(&x)));
    return result;
#else
    const KDfloat64KHR tiny = 1.0e-300;
    KDfloat64KHR z;
    KDint32 sign = (KDint)KDINT32_MIN;
    KDint32 ix0, s0, q, m, t, i;
    KDuint32 r, s1, ix1, q1;

    EXTRACT_WORDS(ix0, ix1, x);

    /* take care of Inf and NaN */
    if((ix0 & 0x7ff00000) == 0x7ff00000)
    {
        return x * x + x; /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                       sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if(ix0 <= 0)
    {
        if(((ix0 & (~sign)) | ix1) == 0)
        {
            return x; /* sqrt(+-0) = +-0 */
        }
        else if(ix0 < 0)
        {
            return KD_NAN; /* sqrt(-ve) = sNaN */
        }
    }
    /* normalize x */
    m = (ix0 >> 20);
    if(m == 0)
    { /* subnormal x */
        while(ix0 == 0)
        {
            m -= 21;
            ix0 |= (ix1 >> 11);
            ix1 <<= 21;
        }
        for(i = 0; (ix0 & 0x00100000) == 0; i++)
        {
            ix0 <<= 1;
        }
        /* Bit shifting by 32 is undefined behaviour*/
        if(i > 0)
        {
            m -= i - 1;
            ix0 |= (ix1 >> (32 - i));
            ix1 <<= i;
        }
    }
    m -= 1023; /* unbias exponent */
    ix0 = (ix0 & 0x000fffff) | 0x00100000;
    if(m & 1)
    { /* odd m, double x to make it even */
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
    }
    m >>= 1; /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0; /* [q,q1] = sqrt(x) */
    r = 0x00200000;       /* r = moving bit from right to left */

    while(r != 0)
    {
        t = s0 + r;
        if(t <= ix0)
        {
            s0 = t + r;
            ix0 -= t;
            q += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    r = sign;
    while(r != 0)
    {
        KDuint32 t1 = s1 + r;
        t = s0;
        if((t < ix0) || ((t == ix0) && (t1 <= ix1)))
        {
            s1 = t1 + r;
            if(((t1 & sign) == (KDuint32)sign) && (s1 & sign) == 0)
            {
                s0 += 1;
            }
            ix0 -= t;
            if(ix1 < t1)
            {
                ix0 -= 1;
            }
            ix1 -= t1;
            q1 += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    /* use floating add to find out rounding direction */
    if((ix0 | ix1) != 0)
    {
        z = 1.0 - tiny; /* trigger inexact flag */
        if(z >= 1.0)
        {
            z = 1.0 + tiny;
            if(q1 == (KDuint32)KDUINT_MAX)
            {
                q1 = 0;
                q += 1;
            }
            else if(z > 1.0)
            {
                if(q1 == (KDuint32)0xfffffffe)
                {
                    q += 1;
                }
                q1 += 2;
            }
            else
            {
                q1 += (q1 & 1);
            }
        }
    }
    ix0 = (q >> 1) + 0x3fe00000;
    ix1 = q1 >> 1;
    if((q & 1) == 1)
    {
        ix1 |= sign;
    }
    ix0 += (m << 20);
    INSERT_WORDS(z, ix0, ix1);
    return z;
#endif
}

/* kdCeilKHR
 * Return x rounded toward -inf to integral value
 * Method:
 *  Bit twiddling.
 * Exception:
 *  Inexact flag raised if x not equal to ceil(x).
 */
KD_API KDfloat64KHR KD_APIENTRY kdCeilKHR(KDfloat64KHR x)
{
#ifdef __SSE4_1__
    KDfloat64KHR result = 0.0;
    _mm_store_sd(&result, _mm_ceil_sd(_mm_load_sd(&result), _mm_load_sd(&x)));
    return result;
#else
    const KDfloat64KHR huge = 1.000e+300;
    KDint32 i0, i1, j0;
    KDuint32 i, j;

    EXTRACT_WORDS(i0, i1, x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if(j0 < 20)
    {
        if(j0 < 0)
        { /* raise inexact if x != 0 */
            if(huge + x > 0.0)
            { /* return 0*sign(x) if |x|<1 */
                if(i0 < 0)
                {
                    i0 = KDINT32_MIN;
                    i1 = 0;
                }
                else if((i0 | i1) != 0)
                {
                    i0 = 0x3ff00000;
                    i1 = 0;
                }
            }
        }
        else
        {
            i = (0x000fffff) >> j0;
            if(((i0 & i) | i1) == 0)
            {
                return x; /* x is integral */
            }
            if(huge + x > 0.0)
            { /* raise inexact flag */
                if(i0 > 0)
                {
                    i0 += (0x00100000) >> j0;
                }
                i0 &= (~i);
                i1 = 0;
            }
        }
    }
    else if(j0 > 51)
    {
        if(j0 == 0x400)
        {
            return x + x; /* inf or NaN */
        }
        else
        {
            return x; /* x is integral */
        }
    }
    else
    {
        i = ((KDuint32)(0xffffffff)) >> (j0 - 20);
        if((i1 & i) == 0)
        {
            return x; /* x is integral */
        }
        if(huge + x > 0.0)
        { /* raise inexact flag */
            if(i0 > 0)
            {
                if(j0 == 20)
                {
                    i0 += 1;
                }
                else
                {
                    j = i1 + (1 << (52 - j0));
                    if((KDint32)j < i1)
                    {
                        i0 += 1; /* got a carry */
                    }
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    INSERT_WORDS(x, i0, i1);
    return x;
#endif
}

/* kdFloorKHR
 * Return x rounded toward -inf to integral value
 * Method:
 *  Bit twiddling.
 * Exception:
 *  Inexact flag raised if x not equal to floor(x).
 */
KD_API KDfloat64KHR KD_APIENTRY kdFloorKHR(KDfloat64KHR x)
{
#ifdef __SSE4_1__
    KDfloat64KHR result = 0.0;
    _mm_store_sd(&result, _mm_floor_sd(_mm_load_sd(&result), _mm_load_sd(&x)));
    return result;
#else
    const KDfloat64KHR huge = 1.000e+300;
    KDint32 i0, i1, j0;
    KDuint32 i, j;

    EXTRACT_WORDS(i0, i1, x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if(j0 < 20)
    {
        if(j0 < 0)
        { /* raise inexact if x != 0 */
            if(huge + x > 0.0)
            { /* return 0*sign(x) if |x|<1 */
                if(i0 >= 0)
                {
                    i0 = i1 = 0;
                }
                else if(((i0 & KDINT32_MAX) | i1) != 0)
                {
                    i0 = 0xbff00000;
                    i1 = 0;
                }
            }
        }
        else
        {
            i = (0x000fffff) >> j0;
            if(((i0 & i) | i1) == 0)
            {
                return x;
            } /* x is integral */
            if(huge + x > 0.0)
            { /* raise inexact flag */
                if(i0 < 0)
                {
                    i0 += (0x00100000) >> j0;
                }
                i0 &= (~i);
                i1 = 0;
            }
        }
    }
    else if(j0 > 51)
    {
        if(j0 == 0x400)
        {
            return x + x; /* inf or NaN */
        }
        else
        {
            return x;
        } /* x is integral */
    }
    else
    {
        i = KDUINT_MAX >> (j0 - 20);
        if((i1 & i) == 0)
        {
            return x;
        } /* x is integral */
        if(huge + x > 0.0)
        { /* raise inexact flag */
            if(i0 < 0)
            {
                if(j0 == 20)
                {
                    i0 += 1;
                }
                else
                {
                    j = i1 + (1 << (52 - j0));
                    if((KDint32)j < i1)
                    {
                        i0 += 1; /* got a carry */
                    }
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    INSERT_WORDS(x, i0, i1);
    return x;
#endif
}

KD_API KDfloat64KHR KD_APIENTRY kdRoundKHR(KDfloat64KHR x)
{
    KDfloat64KHR t;
    KDuint32 hx;

    GET_HIGH_WORD(hx, x);
    if((hx & KDUINT32_MAX) == 0x7ff00000)
    {
        return (x + x);
    }

    if(!(hx & KDINT32_MIN))
    {
        t = kdFloorKHR(x);
        if(t - x <= -0.5)
        {
            t += 1;
        }
        return (t);
    }
    else
    {
        t = kdFloorKHR(-x);
        if(t + x <= -0.5)
        {
            t += 1;
        }
        return (-t);
    }
}

KD_API KDfloat64KHR KD_APIENTRY kdInvsqrtKHR(KDfloat64KHR x)
{
    return 1.0 / kdSqrtKHR(x);
}

/* kdFmodKHR
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */
KD_API KDfloat64KHR KD_APIENTRY kdFmodKHR(KDfloat64KHR x, KDfloat64KHR y)
{
    const KDfloat64KHR one = 1.0;
    const KDfloat64KHR Zero[] = {
        0.0,
        -0.0,
    };
    KDint32 n, hx, hy, hz, ix, iy, sx, i;
    KDuint32 lx, ly, lz;

    EXTRACT_WORDS(hx, lx, x);
    EXTRACT_WORDS(hy, ly, y);
    sx = hx & KDINT32_MIN; /* sign of x */
    hx ^= sx;              /* |x| */
    hy &= KDINT32_MAX;     /* |y| */

    /* purge off exception values */
    if((hy | ly) == 0 || (hx >= 0x7ff00000) ||           /* y=0,or x not finite */
        ((hy | ((ly | -(KDint)ly) >> 31)) > 0x7ff00000)) /* or y is NaN */
    {
        return (x * y) / (x * y);
    }
    if(hx <= hy)
    {
        if((hx < hy) || (lx < ly))
        {
            return x; /* |x|<|y| return x */
        }
        if(lx == ly)
        {
            return Zero[(KDuint32)sx >> 31]; /* |x|=|y| return x*0*/
        }
    }

    /* determine ix = ilogb(x) */
    if(hx < 0x00100000)
    { /* subnormal x */
        if(hx == 0)
        {
            for(ix = -1043, i = lx; i > 0; i <<= 1)
            {
                ix -= 1;
            }
        }
        else
        {
            for(ix = -1022, i = (hx << 11); i > 0; i <<= 1)
            {
                ix -= 1;
            }
        }
    }
    else
    {
        ix = (hx >> 20) - 1023;
    }

    /* determine iy = ilogb(y) */
    if(hy < 0x00100000)
    { /* subnormal y */
        if(hy == 0)
        {
            for(iy = -1043, i = ly; i > 0; i <<= 1)
            {
                iy -= 1;
            }
        }
        else
        {
            for(iy = -1022, i = (hy << 11); i > 0; i <<= 1)
            {
                iy -= 1;
            }
        }
    }
    else
    {
        iy = (hy >> 20) - 1023;
    }

    /* set up {hx,lx}, {hy,ly} and align y to x */
    if(ix >= -1022)
    {
        hx = 0x00100000 | (0x000fffff & hx);
    }
    else
    { /* subnormal x, shift x to normal */
        n = -1022 - ix;
        if(n <= 31)
        {
            hx = (hx << n) | (lx >> (32 - n));
            lx <<= n;
        }
        else
        {
            hx = lx << (n - 32);
            lx = 0;
        }
    }
    if(iy >= -1022)
    {
        hy = 0x00100000 | (0x000fffff & hy);
    }
    else
    { /* subnormal y, shift y to normal */
        n = -1022 - iy;
        if(n <= 31)
        {
            hy = (hy << n) | (ly >> (32 - n));
            ly <<= n;
        }
        else
        {
            hy = ly << (n - 32);
            ly = 0;
        }
    }

    /* fix point fmod */
    n = ix - iy;
    while(n--)
    {
        hz = hx - hy;
        lz = lx - ly;
        if(lx < ly)
        {
            hz -= 1;
        }
        if(hz < 0)
        {
            hx = hx + hx + (lx >> 31);
            lx = lx + lx;
        }
        else
        {
            if((hz | lz) == 0) /* return sign(x)*0 */
            {
                return Zero[(KDuint32)sx >> 31];
            }
            hx = hz + hz + (lz >> 31);
            lx = lz + lz;
        }
    }
    hz = hx - hy;
    lz = lx - ly;
    if(lx < ly)
    {
        hz -= 1;
    }
    if(hz >= 0)
    {
        hx = hz;
        lx = lz;
    }

    /* convert back to floating value and restore the sign */
    if((hx | lx) == 0) /* return sign(x)*0 */
    {
        return Zero[(KDuint32)sx >> 31];
    }
    while(hx < 0x00100000)
    { /* normalize x */
        hx = hx + hx + (lx >> 31);
        lx = lx + lx;
        iy -= 1;
    }
    if(iy >= -1022)
    { /* normalize output */
        hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
        INSERT_WORDS(x, hx | sx, lx);
    }
    else
    { /* subnormal output */
        n = -1022 - iy;
        if(n <= 20)
        {
            lx = (lx >> n) | ((KDuint32)hx << (32 - n));
            hx >>= n;
        }
        else if(n <= 31)
        {
            lx = (hx << (32 - n)) | (lx >> n);
            hx = sx;
        }
        else
        {
            lx = hx >> (n - 32);
            hx = sx;
        }
        INSERT_WORDS(x, hx | sx, lx);
        x *= one; /* create necessary signal */
    }
    return x; /* exact output */
}

/* kdBitsToFloatNV
 * Returns bit equivalent float
 */
KD_API KDfloat32 KD_APIENTRY kdBitsToFloatNV(KDuint32 x)
{
    KDfloat32 f = 0.0f;
    kdAssert(sizeof(KDuint32) == sizeof(KDfloat32));
    kdMemcpy(&f, &x, sizeof(x));
    return f;
}
