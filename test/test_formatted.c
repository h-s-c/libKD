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
#include <KD/KHR_formatted.h>
#include "test.h"
#include <math.h>

void test(const KDchar *str, const KDchar *format, ...)
{
    KDchar buf[1024];
    KDVaListKHR ap;
    KD_VA_START_KHR(ap, format);
    kdVsprintfKHR(buf, format, ap);
    KD_VA_END_KHR(ap);
    TEST_STREQ(str, buf);
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    /* integers */
    test("a b     1", "%c %s     %d", 'a', "b", 1);
    test("abc     ", "%-8.3s", "abcdefgh");
    test("+5", "%+2d", 5);
    test("  6", "% 3i", 6);
    test("-7  ", "%-4d", -7);
    test("+0", "%+d", 0);
    test("     00003:     00004", "%10.5d:%10.5d", 3, 4);
    test("-100006789", "%d", -100006789);
    test("20 0020", "%u %04u", 20u, 20u);
    test("12 1e 3C", "%o %x %X", 10u, 30u, 60u);
    test(" 12 1e 3C ", "%3o %2x %-3X", 10u, 30u, 60u);
    test("012 0x1e 0X3C", "%#o %#x %#X", 10u, 30u, 60u);
    test("", "%.0x", 0);
    test("0", "%.0d", 0);
    test("33 555", "%hi %ld", (KDint16)33, 555l);
    test("9888777666", "%llu", 9888777666llu);
    test("-1 2 -3", "%ji %zi %ti", (KDint64)-1, (KDssize)2, (KDuintptr)-3);

    /* floating-point numbers */
    test("-3.000000", "%f", -3.0);
    test("-8.8888888800", "%.10f", -8.88888888);
    test("880.0888888800", "%.10f", 880.08888888);
    test("4.1", "%.1f", 4.1);
    test(" 0", "% .0f", 0.1);
    test("0.00", "%.2f", 1e-4);
    test("-5.20", "%+4.2f", -5.2);
    test("0.0       ", "%-10.1f", 0.);
    test("0.000001", "%f", 9.09834e-07);

    const KDfloat64KHR pow_2_85 = 38685626227668133590597632.0;
    test("38685626227668133600000000.0", "%.1f", pow_2_85);
    test("0.000000499999999999999978", "%.24f", 5e-7);
    test("0.000000000000000020000000", "%.24f", 2e-17);
    test("0.0000000100 100000000", "%.10f %.0f", 1e-8, 1e+8);
    test("100056789.0", "%.1f", 100056789.0);
    test(" 1.23 %", "%*.*f %%", 5, 2, 1.23);
    test("-3.000000e+00", "%e", -3.0);
    test("4.1E+00", "%.1E", 4.1);
    test("-5.20e+00", "%+4.2e", -5.2);
    test("+0.3 -3", "%+g %+g", 0.3, -3.0);
    test("4", "%.1G", 4.1);
    test("-5.2", "%+4.2g", -5.2);
    test("3e-300", "%g", 3e-300);
    test("1", "%.0g", 1.2);
    test(" 3.7 3.71", "% .3g %.3g", 3.704, 3.706);
    test("2e-315:1e+308", "%g:%g", 2e-315, 1e+308);

    //test("Inf Inf NaN", "%g %G %f", KD_INFINITY, KD_INFINITY, KD_NAN);
    //test("N", "%.1g", KD_NAN);

    /* %n */
    KDint n = 0;
    test("aaa ", "%.3s %n", "aaaaaaaaaaaaa", &n);
    kdAssert(n == 4);

    /* hex floats */
#if !defined(_MSC_VER) || _MSC_VER >= 1912
    test("0x1.fedcbap+98", "%a", 0x1.fedcbap+98);
#endif
    test("0x1.999999999999a0p-4", "%.14a", 0.1);
    test("0x1.0p-1022", "%.1a", 0x1.ffp-1023);
    test("0x1.009117p-1022", "%a", 2.23e-308);
    test("-0x1.AB0P-5", "%.3A", -0x1.abp-5);

    /* %p */
    test("0000000000000000", "%p", KD_NULL);

    /* ' modifier. Non-standard, but supported by glibc. */
    test("1,200,000", "%'d", 1200000);
    test("-100,006,789", "%'d", -100006789);
    test("9,888,777,666", "%'lld", 9888777666ll);
    test("200,000,000.000000", "%'18f", 2e8);
    test("100,056,789", "%'.0f", 100056789.0);
    test("100,056,789.0", "%'.1f", 100056789.0);
    test("000,001,200,000", "%'015d", 1200000);

    /* things not supported by glibc */
    test("null", "%s", (KDchar*) KD_NULL);
    test("123,4abc:", "%'x:", 0x1234ABC);
    test("100000000", "%b", 256);
    test("0b10 0B11", "%#b %#B", 2, 3);
    test("2 3 4", "%I64d %I32d %Id", 2ll, 3, 4ll);
    test("1k 2.54 M", "%$_d %$.2d", 1000, 2536000);
    test("2.42 Mi 2.4 M", "%$$.2d %$$$d", 2536000, 2536000);
    return 0;
}