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
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{

    KDchar buf1[10] = "xxx";
    KDchar buf2[10] = "xxy";

    TEST_EXPR(kdStrcmp(buf1, buf1) == 0);
    TEST_EXPR(kdStrcmp(buf2, buf2) == 0);

    TEST_EXPR(kdStrcmp("xxx", "xxxyyy") < 0);
    TEST_EXPR(kdStrcmp("xxxyyy", "xxx") > 0);

    TEST_EXPR(kdStrcmp(buf1 + 0, buf2 + 0) < 0);
    TEST_EXPR(kdStrcmp(buf1 + 1, buf2 + 1) < 0);
    TEST_EXPR(kdStrcmp(buf1 + 2, buf2 + 2) < 0);
    TEST_EXPR(kdStrcmp(buf1 + 3, buf2 + 3) == 0);

    TEST_EXPR(kdStrcmp(buf2 + 0, buf1 + 0) > 0);
    TEST_EXPR(kdStrcmp(buf2 + 1, buf1 + 1) > 0);
    TEST_EXPR(kdStrcmp(buf2 + 2, buf1 + 2) > 0);
    TEST_EXPR(kdStrcmp(buf2 + 3, buf1 + 3) == 0);

    return 0;
}
