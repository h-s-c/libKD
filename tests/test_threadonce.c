/******************************************************************************
* Copyright (c) 2014 Kevin Schmidt
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
#include <stdatomic.h> /* _Atomic */
#include <stdlib.h> /* EXIT_FAILURE */

#include <stdio.h>

/* Test if we can call test_func more than once. */
#define THREAD_COUNT 10
_Atomic (KDint) test_once_count = ATOMIC_VAR_INIT(0);
static KDThreadOnce test_once = KD_THREAD_ONCE_INIT;
static void test_once_func()
{
    atomic_fetch_add(&test_once_count, 1);
}

void* test_func( void *arg)
{
    for(KDint i = 0 ; i < THREAD_COUNT ;i++)
    {
        kdThreadOnce(&test_once, test_once_func);
    }
    kdThreadJoin(kdThreadSelf(), KD_NULL);
    if(kdGetError() != KD_EDEADLK)
    {
        kdExit(EXIT_FAILURE);
    }
    kdThreadExit(KD_NULL);
    kdExit(EXIT_FAILURE);
}

KDint kdMain(KDint argc, const KDchar *const *argv)
{
    static KDThread* threads[THREAD_COUNT] = {KD_NULL};
    for(KDint i = 0 ; i < THREAD_COUNT ; i++)
    {
        threads[i] = kdThreadCreate(KD_NULL, test_func, KD_NULL);
        if (threads[i] == KD_NULL)
        {
            kdExit(EXIT_FAILURE);
        }
    }
    for(KDint k = 0 ; k < THREAD_COUNT ; k++)
    {
        if(kdThreadJoin(threads[k], KD_NULL) == -1)
        {
            kdExit(EXIT_FAILURE);
        }
        threads[k] = KD_NULL;
    }

    KDint test = atomic_load(&test_once_count);
    if (test != 1)
    {
        kdExit(EXIT_FAILURE);
    }
    return 0;
}