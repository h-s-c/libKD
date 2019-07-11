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
    const KDchar *mode[] = {"r", "r+", "w", "w+", "a", "a+"};
#if defined(_WIN32)
    const KDchar *path = "NUL";
#else
    const KDchar *path = "/dev/null";
#endif

    KDsize i;
    KDFile *f;

    for(i = 0; i < (sizeof(mode) / sizeof(mode[0])); i++)
    {
        f = kdFopen(path, mode[i]);
        if(f == KD_NULL)
        {
            TEST_FAIL();
        }
        else
        {
            TEST_EXPR(kdFclose(f) == 0);
        }
    }

    return 0;
}
