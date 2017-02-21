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

#ifndef __kdext_h_
#define __kdext_h_

#include <KD/ACR_system_font.h>
#include <KD/ATX_dxtcomp.h>
#include <KD/ATX_imgdec.h>
#include <KD/ATX_imgdec_jpeg.h>
#include <KD/ATX_imgdec_png.h>
#include <KD/ATX_imgdec_pvr.h>
#include <KD/ATX_keyboard.h>
#include <KD/KHR_float64.h>
#include <KD/KHR_formatted.h>
#include <KD/KHR_perfcounter.h>
#include <KD/KHR_thread_storage.h>

#define KD_ACR_system_font
#define KD_ATX_dxtcomp
#define KD_ATX_imgdec
#define KD_ATX_imgdec_jpeg
#define KD_ATX_imgdec_png
#define KD_ATX_imgdec_pvr
#define KD_ATX_keyboard
#define KD_KHR_float64
#define KD_KHR_formatted
#define KD_KHR_perfcounter
#define KD_KHR_staticdata
#define KD_KHR_thread_storage

/*******************************************************
 * Errors (extensions)
 *******************************************************/

#if defined(_MSC_VER) || defined(__MINGW32__)
typedef KDuint KDPlatformErrorVEN;
#else
typedef KDint KDPlatformErrorVEN;
#endif

KD_API void KD_APIENTRY kdSetErrorPlatformVEN(KDPlatformErrorVEN error, KDint allowed);

/*******************************************************
 * Threads and synchronization (extensions)
 *******************************************************/

/* kdThreadAttrSetDebugNameVEN: Set debugname attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetDebugNameVEN(KDThreadAttr *attr, const char * debugname);
 
/* kdThreadSleepVEN: Blocks the current thread for nanoseconds. */
KD_API KDint KD_APIENTRY kdThreadSleepVEN(KDust timeout);

/*******************************************************
 * Utility library functions (extensions)
 *******************************************************/

/* kdGetEnvVEN: Get an environment variable. */
KD_API KDsize KD_APIENTRY kdGetEnvVEN(const KDchar *env, KDchar *buf, KDsize buflen);

/*******************************************************
 * String and memory functions (extensions)
 *******************************************************/

/* kdStrstrVEN: Locate substring. */
KD_API KDchar* KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2);

/*******************************************************
 * Windowing (extensions)
 *******************************************************/
#ifdef KD_WINDOW_SUPPORTED
KD_API KDint KD_APIENTRY kdRealizePlatformWindowVEN(KDWindow *window, void **nativewindow);
#endif

/*******************************************************
 * Atomics
 *******************************************************/

typedef struct KDAtomicIntVEN KDAtomicIntVEN;
typedef struct KDAtomicPtrVEN KDAtomicPtrVEN;

KD_API KDAtomicIntVEN* KD_APIENTRY kdAtomicIntCreateVEN(KDint value);
KD_API KDAtomicPtrVEN* KD_APIENTRY kdAtomicPtrCreateVEN(void* value);

KD_API KDint KD_APIENTRY kdAtomicIntFreeVEN(KDAtomicIntVEN *object);
KD_API KDint KD_APIENTRY kdAtomicPtrFreeVEN(KDAtomicPtrVEN *object);

KD_API KDint KD_APIENTRY kdAtomicIntLoadVEN(KDAtomicIntVEN *object);
KD_API void* KD_APIENTRY kdAtomicPtrLoadVEN(KDAtomicPtrVEN *object);

KD_API void KD_APIENTRY kdAtomicIntStoreVEN(KDAtomicIntVEN *object, KDint value);
KD_API void KD_APIENTRY kdAtomicPtrStoreVEN(KDAtomicPtrVEN *object, void* value);

KD_API KDint KD_APIENTRY kdAtomicIntFetchAddVEN(KDAtomicIntVEN *object, KDint value);
KD_API KDint KD_APIENTRY kdAtomicIntFetchSubVEN(KDAtomicIntVEN *object, KDint value);

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN *object, KDint expected, KDint desired);
KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN *object, void* expected, void* desired);

/*******************************************************
 * Queue (threadsafe)
 *******************************************************/

typedef struct KDQueueVEN KDQueueVEN;

KD_API KDQueueVEN* KD_APIENTRY kdQueueCreateVEN(KDsize maxsize);
KD_API KDint KD_APIENTRY kdQueueFreeVEN(KDQueueVEN* queue);

KD_API KDsize KD_APIENTRY kdQueueSizeVEN(KDQueueVEN *queue);

KD_API void KD_APIENTRY kdQueuePushHeadVEN(KDQueueVEN *queue, void *value);
KD_API void KD_APIENTRY kdQueuePushTailVEN(KDQueueVEN *queue, void *value);

KD_API void* KD_APIENTRY kdQueuePopHeadVEN(KDQueueVEN *queue);
KD_API void* KD_APIENTRY kdQueuePopTailVEN(KDQueueVEN *queue);

#endif /* __kdext_h_ */
