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
    TEST_APPROXF(kdPowf(0.0f, 6.0f), 0.0f);
    TEST_APPROXF(kdPowf(-0.0f, 6.0f), -0.0f);
    TEST_APPROXF(kdPowf(0.0f, 5.0f), 0.0f);
    TEST_APPROXF(kdPowf(-0.0f, 5.0f), -0.0f);
    TEST_APPROXF(kdPowf(6.0f, 0.0f), 1.0f);
    TEST_APPROXF(kdPowf(2.0f, -0.5f), KD_SQRT1_2_F);
    TEST_APPROXF(kdPowf(3.0f, 4.0f), 81.0f);
    TEST_APPROXF(kdPowf(1.0f, 6.0f), 1.0f);
    TEST_EXPR(kdIsNan(kdPowf(-1.0f, 1.5f)));

    TEST_APPROX(kdPowKHR(-2.5, 2.0), 6.25);
    TEST_APPROX(kdPowKHR(-2.0, -3.0), -0.125);
    TEST_APPROX(kdPowKHR(0.0, 6.0), 0.0);
    TEST_APPROX(kdPowKHR(-0.0, 6.0), -0.0);
    TEST_APPROX(kdPowKHR(0.0, 5.0), 0.0);
    TEST_APPROX(kdPowKHR(-0.0, 5.0), -0.0);
    TEST_APPROX(kdPowKHR(6.0, 0.0), 1.0);
    TEST_APPROX(kdPowKHR(2.0, -0.5), KD_SQRT1_2_KHR);
    TEST_APPROX(kdPowKHR(3.0, 4.0), 81.0);
    TEST_APPROX(kdPowKHR(1.0, 6.0), 1.0);
    TEST_EXPR(kdIsNan(kdPowKHR(-1.0, 1.5)));

#if !defined(_MSC_VER)
    TEST_EXPR(kdPowf(-1.0f, KD_INFINITY) == 1.0f);
    TEST_EXPR(kdPowf(-1.0f, -KD_INFINITY) == 1.0f);
    TEST_EXPR(kdPowf(0.5f, -KD_INFINITY) == KD_INFINITY);
    TEST_EXPR(kdPowf(1.5f, -KD_INFINITY) == 0.0f);
    TEST_EXPR(kdPowf(0.5f, KD_INFINITY) == 0.0f);
    TEST_EXPR(kdPowf(1.5f, KD_INFINITY) == KD_INFINITY);
    TEST_EXPR(kdPowf(-KD_INFINITY, -1.0f) == -0.0f);
    TEST_EXPR(kdPowf(-KD_INFINITY, -2.0f) == 0.0f);
    TEST_EXPR(kdPowf(-KD_INFINITY, 1.0f) == -KD_INFINITY);
    TEST_EXPR(kdPowf(-KD_INFINITY, 2.0f) == KD_INFINITY);
    TEST_EXPR(kdPowf(KD_INFINITY, -1.0f) == 0.0f);
    TEST_EXPR(kdPowf(KD_INFINITY, 1.0f) == KD_INFINITY);
    TEST_EXPR(kdPowf(0.0f, -1.0f) == KD_INFINITY);
    TEST_EXPR(kdPowf(-0.0f, -1.0f) == -KD_INFINITY);
    TEST_EXPR(kdPowf(0.0f, -2.0f) == KD_INFINITY);
    TEST_EXPR(kdPowf(-0.0f, -2.0f) == KD_INFINITY);

    TEST_EXPR(kdPowKHR(-1.0, KD_HUGE_VAL_KHR) == 1.0);
    TEST_EXPR(kdPowKHR(-1.0, -KD_HUGE_VAL_KHR) == 1.0);
    TEST_EXPR(kdPowKHR(0.5, -KD_HUGE_VAL_KHR) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(1.5, -KD_HUGE_VAL_KHR) == 0.0);
    TEST_EXPR(kdPowKHR(0.5, KD_HUGE_VAL_KHR) == 0.0);
    TEST_EXPR(kdPowKHR(1.5, KD_HUGE_VAL_KHR) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(-KD_HUGE_VAL_KHR, -1.0) == -0.0);
    TEST_EXPR(kdPowKHR(-KD_HUGE_VAL_KHR, -2.0) == 0.0);
    TEST_EXPR(kdPowKHR(-KD_HUGE_VAL_KHR, 1.0) == -KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(-KD_HUGE_VAL_KHR, 2.0) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(KD_HUGE_VAL_KHR, -1.0) == 0.0);
    TEST_EXPR(kdPowKHR(KD_HUGE_VAL_KHR, 1.0) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(0.0, -1.0) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(-0.0, -1.0) == -KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(0.0, -2.0) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdPowKHR(-0.0, -2.0) == KD_HUGE_VAL_KHR);

#define KD_NANF ((1.0f - 1.0f) / (1.0f - 1.0f))
#define KD_NAN ((1.0 - 1.0) / (1.0 - 1.0))
    TEST_EXPR(kdIsNan(kdPowf(KD_NANF, 3.0f)));
    TEST_EXPR(kdIsNan(kdPowf(3.0f, KD_NANF)));
    TEST_EXPR(kdPowf(1.0f, KD_NANF) == 1.0f);
    TEST_EXPR(kdPowf(KD_NANF, 0.0f) == 1.0f);
    TEST_EXPR(kdPowf(KD_NANF, -0.0f) == 1.0f);

    TEST_EXPR(kdIsNan(kdPowKHR(KD_NAN, 3.0)));
    TEST_EXPR(kdIsNan(kdPowKHR(3.0, KD_NAN)));
    TEST_EXPR(kdPowKHR(1.0, KD_NAN) == 1.0);
    TEST_EXPR(kdPowKHR(KD_NAN, 0.0) == 1.0);
    TEST_EXPR(kdPowKHR(KD_NAN, -0.0) == 1.0);
#endif  
    return 0;
}
