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
#define _GNU_SOURCE /* O_CLOEXEC */
#endif
#include "kdplatform.h"
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#ifdef __EMSCRIPTEN__
#include <stdlib.h>
#endif

/******************************************************************************
 * Thirdparty includes
 ******************************************************************************/

#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
#include "rpmalloc.h"
#endif

/******************************************************************************
 * Memory allocation
 *
 * Notes:
 * - Based on rpmalloc by Mattias Jansson
 ******************************************************************************/

void __kdMallocInit(void)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    rpmalloc_initialize();
#endif
}

void __kdMallocFinal(void)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    rpmalloc_finalize();
#endif
}

void __kdMallocThreadInit(void)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    rpmalloc_thread_initialize();
#endif
}

void __kdMallocThreadFinal(void)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    rpmalloc_thread_finalize();
#endif
}


/* kdMalloc: Allocate memory. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdMalloc(KDsize size)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    return rpmalloc(size);
#else
    return malloc(size);
#endif
}

/* kdFree: Free allocated memory block. */
KD_API void KD_APIENTRY kdFree(void *ptr)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    rpfree(ptr);
#else
    free(ptr);
#endif
}

/* kdRealloc: Resize memory block. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdRealloc(void *ptr, KDsize size)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    return rprealloc(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

/* kdCallocVEN: Allocate and zero-initialize memory. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdCallocVEN(KDsize num, KDsize size)
{
#if !defined(__EMSCRIPTEN__) && !defined(__PGIC__)
    return rpcalloc(num, size);
#else
    return calloc(num, size);
#endif
}
