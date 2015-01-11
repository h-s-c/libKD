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

#include <stdlib.h> /* EXIT_FAILURE */
#include <KD/kd.h>

/* Test if we can call test_func more than once. */
static KDint test_once_count = 0;
static KDThreadOnce test_once = KD_THREAD_ONCE_INIT;
static void test_once_func()
{
    ++test_once_count;
}

void* test_func( void *arg)
{
    for(KDint i = 0 ; i < 999 ;i++)
    {
        kdThreadOnce(&test_once, test_once_func);
    }
    return 0;
}

KDint kdMain(KDint argc, const KDchar *const *argv)
{
    KDThread* threads[9] = {KD_NULL};
    for(KDint thread = 0 ; thread < 9 ;thread++)
    {
        threads[thread] = kdThreadCreate(KD_NULL, test_func, KD_NULL);
    }
    for(KDint thread = 0 ; thread < 9 ;thread++)
    {
        kdThreadJoin(threads[thread], KD_NULL);
    }

    if (test_once_count != 1)
    {
        kdExit(EXIT_FAILURE);
    }
    return 0;
}