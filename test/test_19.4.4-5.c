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

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    const KDchar *path = "fread";
    const KDchar *str = "1234567890x";
    KDchar buf[10];
    KDFile *f;

    kdMemset(buf, 'x', sizeof(buf));

    f = kdFopen(path, "w+");
    TEST_EXPR(f != KD_NULL);

    TEST_EXPR(kdFwrite(str, 1, 10, f) == 10);
    TEST_EXPR(kdFclose(f) == 0);

    f = kdFopen(path, "r");
    TEST_EXPR(f != KD_NULL);

    TEST_EXPR(kdFread(buf, 1, 10, f) == 10);
    TEST_EXPR(kdStrncmp(buf, str, 10) == 0);
    TEST_EXPR(kdFclose(f) == 0);
    TEST_EXPR(kdRemove(path) == 0);

    return 0;
}
