/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2018 Kevin Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arikdSinfg from the use of this software.
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
#include <KD/KHR_float64.h>
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    TEST_APPROXF(kdPowf(-2.5f, 2.0f), 6.25f);
    TEST_APPROXF(kdPowf(-2.0f, -3.0f), -0.125f);
    TEST_EXPR(kdPowf(0.0f, 6.0f) == 0.0f);
    TEST_APPROXF(kdPowf(2.0f, -0.5f), KD_SQRT1_2_F);
    TEST_APPROXF(kdPowf(3.0f, 4.0f), 81.0f);

    TEST_APPROX(kdPowKHR(-2.5, 2.0), 6.25);
    TEST_APPROX(kdPowKHR(-2.0, -3.0), -0.125);
    TEST_EXPR(kdPowKHR(0.0, 6.0) == 0.0);
    TEST_APPROX(kdPowKHR(2.0, -0.5), KD_SQRT1_2_KHR);
    TEST_APPROX(kdPowKHR(3.0, 4.0), 81.0);
    return 0;
}
