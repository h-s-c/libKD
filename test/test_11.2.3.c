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
#include <KD/KHR_formatted.h>
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    static const KDsize n = 1024 * 1000;

    for(KDsize i = 1; i < n; i = i + 1024)
    {
        KDchar buf[512];
        kdSnprintfKHR(buf, sizeof(buf), "%u", i);
        KDuint ul = kdStrtoul(buf, KD_NULL, 10);
        TEST_EXPR(ul == i);

        kdSnprintfKHR(buf, sizeof(buf), "%d", -(KDint)i);
        KDint l = kdStrtol(buf, KD_NULL, 10);
        TEST_EXPR(l == -(KDint)i);
    }

    return 0;
}
