/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2020 Kevin Schmidt
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
    KDchar stringf[KD_FTOSTR_MAXLEN];
    KDssize sizef = kdFtostr(stringf, KD_FTOSTR_MAXLEN, 1.23456789f);
    KDfloat32 f = kdStrtof(stringf, KD_NULL);
    TEST_APPROXF(f, 1.23456789f);
    TEST_EXPR(sizef == 10);
    sizef = kdFtostr(stringf, KD_FTOSTR_MAXLEN, -1.23456789f);
    f = kdStrtof(stringf, KD_NULL);
    TEST_APPROXF(f, -1.23456789f);
    TEST_EXPR(sizef == 11);

    KDchar string[KD_DTOSTR_MAXLEN_KHR];
    KDssize size = kdDtostrKHR(string, KD_DTOSTR_MAXLEN_KHR, 1.2345678910111213);
    KDfloat64KHR d = kdStrtodKHR(string, KD_NULL);
    TEST_APPROX(d, 1.2345678910111213);
    TEST_EXPR(size == 18);
    size = kdDtostrKHR(string, KD_DTOSTR_MAXLEN_KHR, -1.2345678910111213);
    d = kdStrtodKHR(string, KD_NULL);
    TEST_APPROX(d, -1.2345678910111213);
    TEST_EXPR(size == 19);
    return 0;
}
