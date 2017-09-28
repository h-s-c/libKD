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

#include <KD/ATX_dxtcomp.h>
#include <KD/ATX_imgdec.h>
#include <KD/ATX_imgdec_jpeg.h>
#include <KD/ATX_imgdec_png.h>
#include <KD/ATX_imgdec_pvr.h>
#include <KD/ATX_keyboard.h>
#include <KD/KHR_float64.h>
#include <KD/KHR_formatted.h>
#include <KD/KHR_thread_storage.h>
#include <KD/VEN_atomic_ops.h>
#include <KD/VEN_queue.h>

#define KD_ATX_dxtcomp 1
#define KD_ATX_imgdec 1
#define KD_ATX_imgdec_jpeg 1
#define KD_ATX_imgdec_png 1
#define KD_ATX_imgdec_pvr 1
#define KD_ATX_keyboard 1
#define KD_KHR_float64 1
#define KD_KHR_formatted 1
#define KD_KHR_perfcounter 1
#define KD_KHR_thread_storage 1
#define KD_QNX_input 1
#define KD_QNX_window 1
#define KD_VEN_atomic_ops 1
#define KD_VEN_queue 1

/*******************************************************
 * Errors (extensions)
 *******************************************************/

KD_API void KD_APIENTRY kdSetErrorPlatformVEN(KDint error, KDint allowed);

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

/* kdIsalphaVEN: Check if character is alphabetic.*/
KD_API KDint KD_APIENTRY kdIsalphaVEN(KDint c);

/* kdIsdigitVEN: Check if character is decimal digit. */
KD_API KDint KD_APIENTRY kdIsdigitVEN(KDint c);

/* kdIsspaceVEN: Check if character is a white-space. */
KD_API KDint KD_APIENTRY kdIsspaceVEN(KDint c);

/* kdIsupperVEN: Check if character is uppercase letter. */
KD_API KDint KD_APIENTRY kdIsupperVEN(KDint c);

/* kdMinVEN: Returns the smaller of the given values. */
KD_API KDint KD_APIENTRY kdMinVEN(KDint a, KDint b);

/* kdGetEnvVEN: Get an environment variable. */
KD_API KDchar *KD_APIENTRY kdGetEnvVEN(const KDchar *env);

/*******************************************************
 * String and memory functions (extensions)
 *******************************************************/

/* kdStrstrVEN: Locate substring. */
KD_API KDchar* KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2);

/* kdStrcspnVEN:  Get span until character in string. */
KD_API KDsize KD_APIENTRY kdStrcspnVEN(const KDchar *str1, const KDchar *str2);

/*******************************************************
 * Windowing (extensions)
 *******************************************************/
#ifdef KD_WINDOW_SUPPORTED
/* kdGetPlatformDisplayVEN: Wayland only. */
KD_API NativeDisplayType KD_APIENTRY kdGetDisplayVEN(void);
#endif

#endif /* __kdext_h_ */
