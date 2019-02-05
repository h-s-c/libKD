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
#include <KD/ATX_imgdec.h>
#include "test.h"

#define PNG_IMAGE_COUNT 162
#if defined(_WIN32)
#define PNG_IMAGE_PATH "data\\PngSuite\\"
#else
#define PNG_IMAGE_PATH "data/PngSuite/"
#endif

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
   KDDir *dir = kdOpenDir(PNG_IMAGE_PATH);
   TEST_EXPR(dir != KD_NULL);
   KDDirent *dirent = KD_NULL;
   KDint num = 0;
   while((dirent = kdReadDir(dir)))
   {
      if((kdStrcmp(dirent->d_name, ".") != 0) && (kdStrcmp(dirent->d_name, "..") != 0))
      {
         KDchar path[32] = PNG_IMAGE_PATH;
         kdStrncat_s(path, 32, dirent->d_name, 16);
         KDImageATX image = kdGetImageATX(path, KD_IMAGE_FORMAT_RGBA8888_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         kdFreeImageATX(image);
         image = kdGetImageATX(path, KD_IMAGE_FORMAT_RGB888_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         kdFreeImageATX(image);
         image = kdGetImageATX(path, KD_IMAGE_FORMAT_LUMALPHA88_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         kdFreeImageATX(image);
         image = kdGetImageATX(path, KD_IMAGE_FORMAT_ALPHA8_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         kdFreeImageATX(image);
         num++;
      }
   }
   kdCloseDir(dir);
   TEST_EXPR(num == PNG_IMAGE_COUNT);
   return 0;
}
