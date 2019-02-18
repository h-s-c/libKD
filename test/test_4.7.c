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

/* Test if the stack is big enough according to the OpenKODE Core spec */
struct recurse {
    struct recurse *next;
    KDuint64 value;
};
static KDuint64 testrecurse(KDuint64 count, struct recurse *lastrecurse)
{
    if(count != 625)
    {
        struct recurse thisrecurse;
        thisrecurse.value = ++count;
        thisrecurse.next = lastrecurse;
        return testrecurse(count, &thisrecurse);
    }
    else
    {
        KDuint64 product = 1;
        while(lastrecurse)
        {
            product = product * lastrecurse->value | 1;
            lastrecurse = lastrecurse->next;
        }
        return product;
    }
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    testrecurse(0, 0);
    return 0;
}