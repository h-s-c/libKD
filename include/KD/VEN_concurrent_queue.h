
/*******************************************************
 * OpenKODE Core extension: VEN_concurrent_queue
 *******************************************************/

#ifndef __kd_VEN_concurrent_queue_h_
#define __kd_VEN_concurrent_queue_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KDQueueVEN KDQueueVEN;

KD_API KDQueueVEN* KD_APIENTRY kdQueueCreateVEN(KDsize maxsize);
KD_API KDint KD_APIENTRY kdQueueFreeVEN(KDQueueVEN* queue);

KD_API KDsize KD_APIENTRY kdQueueSizeVEN(KDQueueVEN *queue);

KD_API void KD_APIENTRY kdQueuePushHeadVEN(KDQueueVEN *queue, void *value);
KD_API void KD_APIENTRY kdQueuePushTailVEN(KDQueueVEN *queue, void *value);

KD_API void* KD_APIENTRY kdQueuePopHeadVEN(KDQueueVEN *queue);
KD_API void* KD_APIENTRY kdQueuePopTailVEN(KDQueueVEN *queue);

#ifdef __cplusplus
}
#endif

#endif /* __kd_VEN_concurrent_queue_h_ */
