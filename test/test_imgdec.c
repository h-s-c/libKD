/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2019 Kevin Schmidt
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

#define IMAGE_COUNT 38
#define IMAGE_PATH "data/images/"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
   KDDir *dir = kdOpenDir(IMAGE_PATH);
   TEST_EXPR(dir != KD_NULL);
   KDDirent *dirent = KD_NULL;
   KDint num = 0;
   while((dirent = kdReadDir(dir)))
   {
      if((kdStrcmp(dirent->d_name, ".") != 0) && (kdStrcmp(dirent->d_name, "..") != 0))
      {
         KDchar path[64] = IMAGE_PATH;
         kdStrncat_s(path, 64, dirent->d_name, 32);

         KDImageATX image = kdGetImageATX(path, KD_IMAGE_FORMAT_RGBA8888_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         KDint w = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
         KDint h = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
         TEST_EXPR(w != 0 && h != 0);
         void *ptr = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
         TEST_EXPR(ptr != KD_NULL);
         kdFreeImageATX(image);

         image = kdGetImageATX(path, KD_IMAGE_FORMAT_RGB888_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         w = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
         h = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
         TEST_EXPR(w != 0 && h != 0);
         ptr = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
         TEST_EXPR(ptr != KD_NULL);
         kdFreeImageATX(image);

         image = kdGetImageATX(path, KD_IMAGE_FORMAT_LUMALPHA88_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         w = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
         h = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
         TEST_EXPR(w != 0 && h != 0);
         ptr = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
         TEST_EXPR(ptr != KD_NULL);
         kdFreeImageATX(image);

         image = kdGetImageATX(path, KD_IMAGE_FORMAT_LUM8_ATX, 0);
         TEST_EXPR(image != KD_NULL);
         w = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
         h = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
         TEST_EXPR(w != 0 && h != 0);
         ptr = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
         TEST_EXPR(ptr != KD_NULL);
         kdFreeImageATX(image);
         
         num++;
      }
   }
   kdCloseDir(dir);
   TEST_EXPR(num == IMAGE_COUNT);
   return 0;
}
