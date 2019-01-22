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
    TEST_EXPR(kdLogf(1.0f) == 0.0f);
    TEST_APPROXF(kdLogf(KD_E_F) , 1.0f);
    TEST_APPROXF(kdLogf(KD_E_F * KD_E_F * KD_E_F) , 3.0f);

    TEST_EXPR(kdLogKHR(1.0) == 0.0);
    TEST_APPROX(kdLogKHR(KD_E_KHR) , 1.0);
    TEST_APPROX(kdLogKHR(KD_E_KHR * KD_E_KHR * KD_E_KHR) , 3.0);   

#if !defined(_MSC_VER)
    TEST_EXPR(kdIsNan(kdLogf(-1.0f)));
    TEST_EXPR(kdIsNan(kdLogf(-KD_INFINITY)));
    TEST_EXPR(kdLogf(0.0f) == -KD_INFINITY);
    TEST_EXPR(kdLogf(-0.0f) == -KD_INFINITY);

    TEST_EXPR(kdIsNan(kdLogKHR(-1.0)));
    TEST_EXPR(kdIsNan(kdLogKHR(-KD_HUGE_VAL_KHR))); 
    TEST_EXPR(kdLogKHR(0.0f) == -KD_HUGE_VAL_KHR);
    TEST_EXPR(kdLogKHR(-0.0f) == -KD_HUGE_VAL_KHR);
   
#define KD_NANF ((1.0f - 1.0f) / (1.0f - 1.0f))
#define KD_NAN ((1.0 - 1.0) / (1.0 - 1.0))
    TEST_EXPR(kdIsNan(kdLogf(KD_NANF)));
    TEST_EXPR(kdIsNan(kdLogKHR(KD_NAN)));
#endif
    return 0;
}
