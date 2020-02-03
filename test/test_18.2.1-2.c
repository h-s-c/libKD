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

#include <KD/kd.h>
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDTimer *timer = kdSetTimer(100000, KD_TIMER_ONESHOT, KD_NULL);
    if(timer == KD_NULL)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            return 0;
        }
        TEST_FAIL();
    }
    for(;;)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_TIMER)
            {
                break;
            }
            kdDefaultEvent(event);
        }
    }
    kdCancelTimer(timer);


    timer = kdSetTimer(100000, KD_TIMER_PERIODIC_AVERAGE, KD_NULL);
    if(timer == KD_NULL)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            return 0;
        }
        TEST_FAIL();
    }
    KDint counter = 0;
    while(counter != 10)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_TIMER)
            {
                counter++;
            }
            kdDefaultEvent(event);
        }
    }
    kdCancelTimer(timer);

    timer = kdSetTimer(100000, KD_TIMER_PERIODIC_MINIMUM, KD_NULL);
    if(timer == KD_NULL)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            return 0;
        }
        TEST_FAIL();
    }
    counter = 0;
    while(counter != 10)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_TIMER)
            {
                counter++;
            }
            kdDefaultEvent(event);
        }
    }
    kdCancelTimer(timer);

    return 0;
}
