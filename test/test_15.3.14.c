/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2018 Kevin Schmidt
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
#include <KD/KHR_float64.h>
#include "test.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    TEST_APPROXF(kdFloorf(-0.0f), -0.0f);
    TEST_APPROXF(kdFloorf(-5.1f), -6.0f);
    TEST_APPROXF(kdFloorf(-5.0f), -5.0f);
    TEST_APPROXF(kdFloorf(-4.9f), -5.0f);
    TEST_APPROXF(kdFloorf(0.0f), 0.0f);
    TEST_APPROXF(kdFloorf(4.9f), 4.0f);
    TEST_APPROXF(kdFloorf(5.0f), 5.0f);
    TEST_APPROXF(kdFloorf(5.1f), 5.0f);

    TEST_APPROXF(kdFloorKHR(-0.0), -0.0);
    TEST_APPROXF(kdFloorKHR(-5.1), -6.0);
    TEST_APPROXF(kdFloorKHR(-5.0), -5.0);
    TEST_APPROXF(kdFloorKHR(-4.9), -5.0);
    TEST_APPROXF(kdFloorKHR(0.0), 0.0);
    TEST_APPROXF(kdFloorKHR(4.9), 4.0);
    TEST_APPROXF(kdFloorKHR(5.0), 5.0);
    TEST_APPROXF(kdFloorKHR(5.1), 5.0);

#if !defined(_MSC_VER)
    TEST_EXPR(kdFloorf(KD_INFINITY) == KD_INFINITY);
    TEST_EXPR(kdFloorf(-KD_INFINITY) == -KD_INFINITY);
    TEST_EXPR(kdFloorKHR(KD_HUGE_VAL_KHR) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdFloorKHR(-KD_HUGE_VAL_KHR) == -KD_HUGE_VAL_KHR);
   
#define KD_NANF ((1.0f - 1.0f) / (1.0f - 1.0f))
#define KD_NAN ((1.0 - 1.0) / (1.0 - 1.0))
    TEST_EXPR(kdIsNan(kdFloorf(KD_NANF)));
    TEST_EXPR(kdIsNan(kdFloorKHR(KD_NAN)));
#endif
    return 0;
}
