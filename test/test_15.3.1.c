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
    TEST_APPROXF(kdAcosf(-1.0f), 4.0f * KD_PI_4_F);
    TEST_APPROXF(kdAcosf(-KD_SQRT1_2_F), 3.0f * KD_PI_4_F);
    TEST_APPROXF(kdAcosf(0.0f), 2.0f * KD_PI_4_F);
    TEST_APPROXF(kdAcosf(KD_SQRT1_2_F), KD_PI_4_F);
    TEST_APPROXF(kdAcosf(1.0f), 0.0f);
    TEST_EXPR(kdIsNan(kdAcosf(-1.1f)));
    TEST_EXPR(kdIsNan(kdAcosf(1.1f)));

    TEST_APPROX(kdAcosKHR(-1.0), 4.0 * KD_PI_4_KHR);
    TEST_APPROX(kdAcosKHR(-KD_SQRT1_2_KHR), 3.0 * KD_PI_4_KHR);
    TEST_APPROX(kdAcosKHR(0.0), 2.0 * KD_PI_4_KHR);
    TEST_APPROX(kdAcosKHR(KD_SQRT1_2_KHR), KD_PI_4_KHR);
    TEST_APPROX(kdAcosKHR(1.0), 0.0);
    TEST_EXPR(kdIsNan(kdAcosKHR(-1.1)));
    TEST_EXPR(kdIsNan(kdAcosKHR(1.1)));

    TEST_EXPR(kdIsNan(kdAcosf(KD_NANF)));
    TEST_EXPR(kdIsNan(kdAcosKHR(KD_NAN)));
    return 0;
}
