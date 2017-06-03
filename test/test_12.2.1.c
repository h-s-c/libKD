/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2017 Kevin Schmidt
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
#include <KD/kdext.h>

#ifdef KD_NDEBUG
#error "Dont run tests with NDEBUG defined."
#endif

/* "" is a valid return if no locale info can be gathered but this shouldn't happen */
KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    const KDchar *locale = kdGetLocale();
    if(kdStrcmp(locale, "en") == 0 || kdStrcmp(locale, "en_") == 0 || kdStrcmp(locale, "en_US") == 0)
    {
        return 0;
    }
    else if(kdStrlen(locale) == 2 || kdStrlen(locale) == 3 || kdStrlen(locale) == 5)
    {
        return 0;
    }
    else
    {
        kdAssert(0);
    }
    return 0;
}
