/* ***************************************************************************
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Kevin Schmidt
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#ifndef __kdplatform_h_
#define __kdplatform_h_

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <math.h>

#include <unistd.h>

#define KD_WINDOW_SUPPORTED
#define MESA_EGL_NO_X11_HEADERS

#define KD_API
#define KD_APIENTRY
#define KD_NORETURN noreturn

#ifdef NDEBUG
#define KD_NDEBUG
#endif

#ifdef KD_NDEBUG
#define kdAssert(c)
#else
#define kdAssert(c) ((void)( (c) ? 0 : (kdHandleAssertion(#c, __FILE__, __LINE__), 0)))
#endif

typedef int32_t KDint32;
typedef uint32_t KDuint32;
typedef int64_t KDint64;
typedef uint64_t KDuint64;
typedef int16_t KDint16;
typedef uint16_t KDuint16;
typedef uintptr_t KDuintptr;
typedef size_t KDsize;
typedef ssize_t KDssize;
#define KDINT_MIN INT32_MIN
#define KDINT_MAX INT32_MAX
#define KDUINT_MAX UINT32_MAX
#define KDINT64_MIN INT64_MIN
#define KDINT64_MAX INT64_MAX
#define KDUINT64_MAX UINT64_MAX
#define KDSSIZE_MIN SSIZE_MIN
#define KDSSIZE_MAX SSIZE_MAX
#define KDSIZE_MAX SIZE_MAX
#define KDUINTPTR_MAX UINTPTR_MAX
#define KD_INFINITY INFINITY

#endif /* __kdplatform_h_ */

