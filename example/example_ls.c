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
#include <KD/kdext.h>

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDDir *dir = kdOpenDir(argc > 1 ? argv[1] : ".");
    KDDirent *dirent = KD_NULL;
    while(1)
    {
        dirent = kdReadDir(dir);
        if(dirent == KD_NULL)
        {
            break;
        }
        kdLogMessagefKHR("%s\n", dirent->d_name);
    }
    kdCloseDir(dir);
    return 0;
}
