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

#include <KD/kd.h>
#include <KD/kdext.h>
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDchar *ptr = KD_NULL;
    KDchar *ptr2 = KD_NULL;
    static const KDchar abc[] = "abcdefghijklmnopqrstuvwxyz";
    KDsize size = kdStrlen(abc);

    TEST_EXPR((ptr = kdMalloc(size)) != KD_NULL);
    kdStrncpy_s(ptr, size, abc, size);
    TEST_EXPR(ptr[size - 1] == 'z');
    TEST_EXPR((ptr = kdRealloc(ptr, (size * 2) + 1)) != KD_NULL);
    kdStrncat_s(ptr, (size * 2) + 1, abc, size);
    TEST_EXPR(ptr[(size * 2) - 1] == 'z');
    kdFree(ptr);

    TEST_EXPR((ptr2 = kdCallocVEN(sizeof(abc), 1)) != KD_NULL);
    TEST_EXPR(ptr2[0] == '\0');
    kdFree(ptr2);
    return 0;
}
