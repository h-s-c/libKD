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
    KDchar string[20];
    KDssize size = 0;
    size = kdLtostr(string, 20, 1234567890);
    TEST_STREQ(string, "1234567890");
    TEST_EQ(size, 10);
    size = kdLtostr(string, 20, -1234567890);
    TEST_STREQ(string, "-1234567890");
    TEST_EQ(size, 11);
    size = kdUltostr(string, 20, 1234567890, 10);
    TEST_STREQ(string, "1234567890");
    TEST_EQ(size, 10);
    return 0;
}
