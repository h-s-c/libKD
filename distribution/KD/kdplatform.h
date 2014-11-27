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

#ifndef __kdplatform_h_
#define __kdplatform_h_

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include <KHR/khrplatform.h>

#define KD_WINDOW_SUPPORTED
#define MESA_EGL_NO_X11_HEADERS

#define KD_API KHRONOS_APICALL
#define KD_APIENTRY KHRONOS_APIENTRY
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
typedef khronos_ssize_t KDssize;
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

