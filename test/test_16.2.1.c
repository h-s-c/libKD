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
    static const KDchar buf[] = "abcdefg";
    KDuint8 i = 7;

    TEST_EXPR(kdMemchr(buf, 'a', 0) == KD_NULL);
    TEST_EXPR(kdMemchr(buf, 'g', 0) == KD_NULL);
    TEST_EXPR(kdMemchr(buf, 'x', 7) == KD_NULL);

    TEST_EXPR(kdMemchr("\0", 'x', 0) == KD_NULL);
    TEST_EXPR(kdMemchr("\0", 'x', 1) == KD_NULL);

    while(i <= 14)
    {

        TEST_EQ(kdMemchr(buf, 'a', i), buf + 0);
        TEST_EQ(kdMemchr(buf, 'b', i), buf + 1);
        TEST_EQ(kdMemchr(buf, 'c', i), buf + 2);
        TEST_EQ(kdMemchr(buf, 'd', i), buf + 3);
        TEST_EQ(kdMemchr(buf, 'e', i), buf + 4);
        TEST_EQ(kdMemchr(buf, 'f', i), buf + 5);
        TEST_EQ(kdMemchr(buf, 'g', i), buf + 6);

        i *= 2;
    }

    return 0;
}
