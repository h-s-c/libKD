/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2015 Kevin Schmidt
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

#ifndef __kd_VEN_atomic_h_
#define __kd_VEN_atomic_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Atomics
 ******************************************************************************/

typedef struct KDAtomicIntVEN KDAtomicIntVEN;
typedef struct KDAtomicPtrVEN KDAtomicPtrVEN;

inline void KD_APIENTRY kdAtomicFenceAcquireVEN(void);
inline void KD_APIENTRY kdAtomicFenceReleaseVEN(void);

KD_API KDAtomicIntVEN* KD_APIENTRY kdAtomicIntCreateVEN(KDint value);
KD_API KDAtomicPtrVEN* KD_APIENTRY kdAtomicPtrCreateVEN(void* value);

KD_API void KD_APIENTRY kdAtomicIntFreeVEN(KDAtomicIntVEN *object);
KD_API void KD_APIENTRY kdAtomicPtrFreeVEN(KDAtomicPtrVEN *object);

KD_API KDint KD_APIENTRY kdAtomicIntLoadVEN(KDAtomicIntVEN *object);
KD_API void* KD_APIENTRY kdAtomicPtrLoadVEN(KDAtomicPtrVEN *object);

KD_API void KD_APIENTRY kdAtomicIntStoreVEN(KDAtomicIntVEN *object, KDint value);
KD_API void KD_APIENTRY kdAtomicPtrStoreVEN(KDAtomicPtrVEN *object, void* value);

KD_API KDint KD_APIENTRY kdAtomicIntFetchAddVEN(KDAtomicIntVEN *object, KDint value);
KD_API KDint KD_APIENTRY kdAtomicIntFetchSubVEN(KDAtomicIntVEN *object, KDint value);

KD_API KDboolean KD_APIENTRY kdAtomicIntCompareExchangeVEN(KDAtomicIntVEN *object, KDint expected, KDint desired);
KD_API KDboolean KD_APIENTRY kdAtomicPtrCompareExchangeVEN(KDAtomicPtrVEN *object, void* expected, void* desired);

#ifdef __cplusplus
}
#endif

#endif /* __kd_VEN_atomic_h_ */

