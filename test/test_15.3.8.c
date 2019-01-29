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

#if defined(__clang__)
#if defined(__has_attribute)
#if __has_attribute(__no_sanitize__)
__attribute__((__no_sanitize__("float-divide-by-zero")))
#endif
#endif
#endif
KDint KD_APIENTRY
kdMain(KDint argc, const KDchar *const *argv)
{
    TEST_APPROXF(kdExpf(-1.0f), 1.0f / KD_E_F);
    TEST_APPROXF(kdExpf(0.0f), 1.0f);
    TEST_APPROXF(kdExpf(KD_LN2_F), 2.0f);
    TEST_APPROXF(kdExpf(1.0f), KD_E_F);
    TEST_APPROXF(kdExpf(3.0f), KD_E_F * KD_E_F * KD_E_F);

    TEST_APPROX(kdExpKHR(-1.0), 1.0 / KD_E_KHR);
    TEST_APPROX(kdExpKHR(0.0), 1.0);
    TEST_APPROX(kdExpKHR(KD_LN2_KHR), 2.0);
    TEST_APPROX(kdExpKHR(1.0), KD_E_KHR);
    TEST_APPROX(kdExpKHR(3.0), KD_E_KHR * KD_E_KHR * KD_E_KHR);

#if !defined(_MSC_VER)
    TEST_EXPR(kdExpf(-KD_INFINITY) == 0.0f);
    TEST_EXPR(kdExpf(KD_INFINITY) == KD_INFINITY);
    TEST_EXPR(kdExpKHR(-KD_HUGE_VAL_KHR) == 0.0);
    TEST_EXPR(kdExpKHR(KD_HUGE_VAL_KHR) == KD_HUGE_VAL_KHR);

#define KD_NANF ((1.0f - 1.0f) / (1.0f - 1.0f))
#define KD_NAN ((1.0 - 1.0) / (1.0 - 1.0))
    TEST_EXPR(kdIsNan(kdExpf(KD_NANF)));
    TEST_EXPR(kdIsNan(kdExpKHR(KD_NAN)));
#endif
    return 0;
}
