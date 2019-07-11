/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2019 Kevin Schmidt
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

#define THREAD_COUNT 10
KDint done = 0;
KDThreadSem *sem = KD_NULL;

void *test_func(void *arg)
{
    kdThreadSemWait(sem);
    kdThreadSemPost(sem);
    return 0;
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    sem = kdThreadSemCreate(1);

    KDThread *threads[THREAD_COUNT] = {KD_NULL};
    for(KDint i = 0; i < THREAD_COUNT; i++)
    {
        threads[i] = kdThreadCreate(KD_NULL, test_func, KD_NULL);
        if(threads[i] == KD_NULL)
        {
            if(kdGetError() == KD_ENOSYS)
            {
                return 0;
            }
            TEST_FAIL();
        }
    }
    for(KDint k = 0; k < THREAD_COUNT; k++)
    {
        kdThreadJoin(threads[k], KD_NULL);
    }

    kdThreadSemFree(sem);
    return 0;
}
