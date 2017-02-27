
/*******************************************************
 * OpenKODE Core extension: VEN_atomic_ops
 *******************************************************/

#ifndef __kd_VEN_atomic_ops_h_
#define __kd_VEN_atomic_ops_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* __kd_VEN_atomic_ops_h_ */
