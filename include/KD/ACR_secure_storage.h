
/*******************************************************
 * OpenKODE Core extension: ACR_secure_storage
 *******************************************************/

#ifndef __kd_ACR_secure_storage_h_
#define __kd_ACR_secure_storage_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

KD_API KDint KD_APIENTRY kdSecureStorageSetValueACR(const KDchar *key, const KDchar *data, KDsize size);
KD_API KDint KD_APIENTRY kdSecureStorageGetValueACR(const KDchar *key, KDchar *buf, KDsize maxSize);

#ifdef __cplusplus
}
#endif

#endif /* __kd_ACR_secure_storage_h_ */
