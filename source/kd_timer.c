// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2021 Kevin Schmidt
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
#include "kdplatform.h"  // for KD_API, KD_APIENTRY, KDint64
#include <KD/kd.h>       // for kdFree, KDTimer, kdSetError, KD_NULL, kdThre...
#include <KD/kdext.h>    // for kdThreadSleepVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * Timer functions
 ******************************************************************************/

/* kdSetTimer: Set timer. */
typedef struct
{
    KDint64 interval;
    void *eventuserptr;
    KDThread *destination;
    KDint periodic;
    KDint8 padding[4];
} _KDTimerPayload;
static void *__kdTimerHandler(void *arg)
{
    _KDTimerPayload *payload = (_KDTimerPayload *)arg;
    for(;;)
    {
        kdThreadSleepVEN(payload->interval);

        /* Post event to the original thread */
        KDEvent *timerevent = kdCreateEvent();
        timerevent->type = KD_EVENT_TIMER;
        timerevent->userptr = payload->eventuserptr;
        kdPostThreadEvent(timerevent, payload->destination);

        /* Abort if this is a oneshot timer*/
        if(payload->periodic == KD_TIMER_ONESHOT)
        {
            break;
        }

        /* Check for quit event send by kdCancelTimer */
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_QUIT)
            {
                break;
            }
            kdDefaultEvent(event);
        }
    }
    return 0;
}
struct KDTimer
{
    KDThread *thread;
    KDThread *originthr;
    _KDTimerPayload *payload;
};
KD_API KDTimer *KD_APIENTRY kdSetTimer(KDint64 interval, KDint periodic, void *eventuserptr)
{
    if(periodic != KD_TIMER_ONESHOT && periodic != KD_TIMER_PERIODIC_AVERAGE && periodic != KD_TIMER_PERIODIC_MINIMUM)
    {
        kdLogMessage("kdSetTimer() encountered unknown periodic value.");
        return KD_NULL;
    }

    _KDTimerPayload *payload = (_KDTimerPayload *)kdMalloc(sizeof(_KDTimerPayload));
    if(payload == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    payload->interval = interval;
    payload->periodic = periodic;
    payload->eventuserptr = eventuserptr;
    payload->destination = kdThreadSelf();

    KDTimer *timer = (KDTimer *)kdMalloc(sizeof(KDTimer));
    if(timer == KD_NULL)
    {
        kdFree(payload);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    timer->thread = kdThreadCreate(KD_NULL, __kdTimerHandler, payload);
    if(timer->thread == KD_NULL)
    {
        kdFree(timer);
        kdFree(payload);
        if(kdGetError() == KD_ENOSYS)
        {
            kdLogMessage("kdSetTimer() needs a threading implementation.");
            return KD_NULL;
        }
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    timer->originthr = kdThreadSelf();
    timer->payload = payload;
    return timer;
}

/* kdCancelTimer: Cancel and free a timer. */
KD_API KDint KD_APIENTRY kdCancelTimer(KDTimer *timer)
{
    if(timer->originthr != kdThreadSelf())
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
    /* Post quit event to the timer thread */
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_QUIT;
    kdPostThreadEvent(event, timer->thread);
    kdThreadJoin(timer->thread, KD_NULL);
    kdFree(timer->payload);
    kdFree(timer);
    return 0;
}
