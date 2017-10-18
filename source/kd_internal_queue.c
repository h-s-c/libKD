// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

/******************************************************************************
 * KD includes
 ******************************************************************************/

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wpadded"
#   if __has_warning("-Wreserved-id-macro")
#       pragma clang diagnostic ignored "-Wreserved-id-macro"
#   endif
#endif
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#include "kd_internal.h"

/******************************************************************************
 * MPMC, FIFO queue
 *
 * Notes:
 * - Based on http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 ******************************************************************************/

typedef struct _kdQueueCell _kdQueueCell;
struct _kdQueueCell {
    KDAtomicIntVEN *sequence;
    void *data;
};

struct _KDQueue {
    KDsize buffer_mask;
    _kdQueueCell *buffer;
    KDAtomicIntVEN *tail;
    KDAtomicIntVEN *head;
};

_KDQueue * __kdQueueCreate(KDsize size)
{
    kdAssert((size >= 2) && ((size & (size - 1)) == 0));

    _KDQueue *queue = (_KDQueue *)kdMalloc(sizeof(_KDQueue));
    if(queue == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    queue->buffer_mask = size - 1;
    queue->buffer = kdMalloc(sizeof(_kdQueueCell) * size);
    if(queue->buffer == KD_NULL)
    {
        kdFree(queue);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }

    for(KDsize i = 0; i != size; i += 1)
    {
        queue->buffer[i].sequence = kdAtomicIntCreateVEN((KDint)i);
    }

    queue->tail = kdAtomicIntCreateVEN(0);
    queue->head = kdAtomicIntCreateVEN(0);
    return queue;
}

KDint __kdQueueFree(_KDQueue *queue)
{
    kdAtomicIntFreeVEN(queue->head);
    kdAtomicIntFreeVEN(queue->tail);

    for(KDsize i = 0; i != queue->buffer_mask + 1; i += 1)
    {
        kdAtomicIntFreeVEN(queue->buffer[i].sequence);
    }

    kdFree(queue->buffer);
    kdFree(queue);
    return 0;
}

KDsize __kdQueueSize(_KDQueue *queue)
{
    return (KDsize)(kdAtomicIntLoadVEN(queue->tail) - kdAtomicIntLoadVEN(queue->head));
}

KDint __kdQueuePush(_KDQueue *queue, void *value)
{
    _kdQueueCell *cell;
    KDsize pos = (KDsize)kdAtomicIntLoadVEN(queue->tail);
    for(;;)
    {
        cell = &queue->buffer[pos & queue->buffer_mask];
        KDsize seq = (KDsize)kdAtomicIntLoadVEN(cell->sequence);
        KDssize dif = (KDssize)seq - (KDssize)pos;
        if(dif == 0)
        {
            if(kdAtomicIntCompareExchangeVEN(queue->tail, (KDint)pos, (KDint)pos + 1))
            {
                break;
            }
        }
        else if(dif < 0)
        {
            kdSetError(KD_EAGAIN);
            return -1;
        }
        else
        {
            pos = (KDsize)kdAtomicIntLoadVEN(queue->tail);
        }
    }

    cell->data = value;
    kdAtomicIntStoreVEN(cell->sequence, (KDint)pos + 1);

    return 0;
}

void * __kdQueuePull(_KDQueue *queue)
{
    _kdQueueCell *cell;
    KDsize pos = (KDsize)kdAtomicIntLoadVEN(queue->head);
    for(;;)
    {
        cell = &queue->buffer[pos & queue->buffer_mask];
        KDsize seq = (KDsize)kdAtomicIntLoadVEN(cell->sequence);
        KDssize dif = (KDssize)seq - (KDssize)(pos + 1);
        if(dif == 0)
        {
            if(kdAtomicIntCompareExchangeVEN(queue->head, (KDint)pos, (KDint)pos + 1))
            {
                break;
            }
        }
        else if(dif < 0)
        {
            kdSetError(KD_EAGAIN);
            return KD_NULL;
        }
        else
        {
            pos = (KDsize)kdAtomicIntLoadVEN(queue->head);
        }
    }

    void *value = cell->data;
    kdAtomicIntStoreVEN(cell->sequence, (KDint)(pos + queue->buffer_mask) + 1);
    return value;
}
