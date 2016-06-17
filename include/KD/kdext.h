/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2016 Kevin Schmidt
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

#include <KD/KHR_float64.h>
#include <KD/KHR_formatted.h>
#include <KD/KHR_perfcounter.h>
#include <KD/KHR_thread_storage.h>

/******************************************************************************
 * Threads and synchronization (extensions)
 ******************************************************************************/

/* kdThreadAttrSetDebugNameVEN: Set debugname attribute. */
KD_API KDint KD_APIENTRY kdThreadAttrSetDebugNameVEN(KDThreadAttr *attr, const char * debugname);
 
/* kdThreadSleepVEN: Blocks the current thread for nanoseconds. */
KD_API KDint KD_APIENTRY kdThreadSleepVEN(KDust timeout);

/******************************************************************************
 * String and memory functions (extensions)
 ******************************************************************************/

/* kdStrstrVEN: Locate substring. */
KD_API KDchar* KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2);

/*******************************************************
 * Input/output (extensions)
 *******************************************************/

typedef struct KDEventInputKeyVEN {
    KDuint32 flags;
    KDint32 keycode;
} KDEventInputKeyVEN;

typedef struct KDEventInputKeyCharVEN {
    KDuint32 flags;
    KDint32 character;
} KDEventInputKeyCharVEN;

#define KD_EVENT_INPUT_KEY_VEN      101
#define KD_EVENT_INPUT_KEYCHAR_VEN  102

#define KD_KEY_PRESS_VEN    0x300000
#define KD_KEY_UP_VEN       0x300001
#define KD_KEY_DOWN_VEN     0x300002
#define KD_KEY_LEFT_VEN     0x300003
#define KD_KEY_RIGHT_VEN    0x300004

/******************************************************************************
 * Atomics
 ******************************************************************************/

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

/******************************************************************************
 * Dispatcher
 ******************************************************************************/

typedef struct KDDispatchVEN KDDispatchVEN;
typedef KDboolean(KD_APIENTRY KDDispatchFuncVEN)(void* optimalinfo, void* candidateinfo);

KD_API KDDispatchVEN* KD_APIENTRY kdDispatchCreateVEN(KDDispatchFuncVEN* filterfunc);
KD_API KDint KD_APIENTRY kdDispatchFreeVEN(KDDispatchVEN* disp);

KD_API KDint KD_APIENTRY kdDispatchInstallCandidateVEN(KDDispatchVEN* disp, void* candidateinfo, KDuintptr candidatefunc);
KD_API KDuintptr KD_APIENTRY kdDispatchGetOptimalVEN(KDDispatchVEN* disp);

/******************************************************************************
 * Queue (threadsafe)
 ******************************************************************************/

typedef struct KDQueueVEN KDQueueVEN;

KD_API KDQueueVEN* KD_APIENTRY kdQueueCreateVEN(KDsize maxsize);
KD_API KDint KD_APIENTRY kdQueueFreeVEN(KDQueueVEN* queue);

KD_API KDsize KD_APIENTRY kdQueueSizeVEN(KDQueueVEN *queue);

KD_API void KD_APIENTRY kdQueuePushHeadVEN(KDQueueVEN *queue, void *value);
KD_API void KD_APIENTRY kdQueuePushTailVEN(KDQueueVEN *queue, void *value);

KD_API void* KD_APIENTRY kdQueuePopHeadVEN(KDQueueVEN *queue);
KD_API void* KD_APIENTRY kdQueuePopTailVEN(KDQueueVEN *queue);

#endif /* __kdext_h_ */
