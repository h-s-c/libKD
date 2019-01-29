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
    TEST_EXPR(kdRoundf(-0.0f) == -0.0f);
    TEST_EXPR(kdRoundf(-5.5f) == -6.0f);
    TEST_EXPR(kdRoundf(-5.0f) == -5.0f);
    TEST_EXPR(kdRoundf(-4.5f) == -5.0f);
    TEST_EXPR(kdRoundf(0.0f) == 0.0f);
    TEST_EXPR(kdRoundf(4.5f) == 5.0f);
    TEST_EXPR(kdRoundf(5.0f) == 5.0f);
    TEST_EXPR(kdRoundf(5.5f) == 6.0f);

    TEST_EXPR(kdRoundKHR(-0.0) == -0.0);
    TEST_EXPR(kdRoundKHR(-5.5) == -6.0);
    TEST_EXPR(kdRoundKHR(-5.0) == -5.0);
    TEST_EXPR(kdRoundKHR(-4.5) == -5.0);
    TEST_EXPR(kdRoundKHR(0.0) == 0.0);
    TEST_EXPR(kdRoundKHR(4.5) == 5.0);
    TEST_EXPR(kdRoundKHR(5.0) == 5.0);
    TEST_EXPR(kdRoundKHR(5.5) == 6.0);

#if !defined(_MSC_VER)
    TEST_EXPR(kdRoundf(KD_INFINITY) == KD_INFINITY);
    TEST_EXPR(kdRoundf(-KD_INFINITY) == -KD_INFINITY);
    TEST_EXPR(kdRoundKHR(KD_HUGE_VAL_KHR) == KD_HUGE_VAL_KHR);
    TEST_EXPR(kdRoundKHR(-KD_HUGE_VAL_KHR) == -KD_HUGE_VAL_KHR);

#define KD_NANF ((1.0f - 1.0f) / (1.0f - 1.0f))
#define KD_NAN ((1.0 - 1.0) / (1.0 - 1.0))
    TEST_EXPR(kdIsNan(kdRoundf(KD_NANF)));
    TEST_EXPR(kdIsNan(kdRoundKHR(KD_NAN)));
#endif
    return 0;
}
