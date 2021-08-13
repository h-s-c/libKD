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

#define EXAMPLE_COMMON_IMPLEMENTATION
#include "example_common.h"

/* Example with events, timers and random numbers */
KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    Example *example = exampleInit();
    
    KDTimer *kd_timer = kdSetTimer(1000000000, KD_TIMER_PERIODIC_AVERAGE, KD_NULL);

    KDfloat32 r = 0.0f;
    KDfloat32 g = 1.0f;
    KDfloat32 b = 0.0f;

    while(example->run)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_TIMER):
                {
                    KDuint8 buffer[3] = {0};
                    kdCryptoRandom(buffer, 3);
                    r = buffer[0] % 256 / 255.0f;
                    g = buffer[1] % 256 / 255.0f;
                    b = buffer[2] % 256 / 255.0f;
                    break;
                }
                case(KD_EVENT_QUIT):
                case(KD_EVENT_WINDOW_CLOSE):
                {
                    example->run = KD_FALSE;
                }
                default:
                {
                    kdDefaultEvent(event);
                }
            }
        }

        glClearColor(r, g, b, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        exampleRun(example);
    }

    kdCancelTimer(kd_timer);
    return exampleDestroy(example);
}
