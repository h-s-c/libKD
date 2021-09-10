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
#include <KD/ATX_dxtcomp.h>
#include <KD/ATX_imgdec.h>
#include "test.h"

#define IMAGE_PATH "data/images/lenna.png"

static KDuint32 magic = 0x20534444;

#define FOURCC(a,b,c,d) ( (KDuint32) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a)) )

typedef struct{
   KDint32 dwSize;
   KDint32 dwFlags;
   KDint32 dwFourCC;
   KDint32 dwRGBBitCount;
   KDint32 dwRBitMask;
   KDint32 dwGBitMask;
   KDint32 dwBBitMask;
   KDint32 dwABitMask;
} DDS_PIXELFORMAT;

#define DDPF_FOURCC        0x4

typedef struct{
   KDuint32           dwSize;
   KDuint32           dwFlags;
   KDuint32           dwHeight;
   KDuint32           dwWidth;
   KDuint32           dwPitchOrLinearSize;
   KDuint32           dwDepth;
   KDuint32           dwMipMapCount;
   KDuint32           dwReserved1[11];
   DDS_PIXELFORMAT    ddspf;
   KDuint32           dwCaps;
   KDuint32           dwCaps2;
   KDuint32           dwCaps3;
   KDuint32           dwCaps4;
   KDuint32           dwReserved2;
} DDS_HEADER;

#define DDSD_CAPS          0x1
#define DDSD_HEIGHT        0x2
#define DDSD_WIDTH         0x4
#define DDSD_PIXELFORMAT   0x1000
#define DDSD_LINEARSIZE    0x80000

#define DDSCAPS_TEXTURE    0x1000

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
   KDImageATX image = kdGetImageATX(IMAGE_PATH, KD_IMAGE_FORMAT_RGBA8888_ATX, 0);
   TEST_EXPR(image != KD_NULL);
   KDImageATX compimage = kdDXTCompressImageATX(image, KD_DXTCOMP_TYPE_DXT1_ATX);
   TEST_EXPR(compimage != KD_NULL);
   KDint width = kdGetImageIntATX(compimage, KD_IMAGE_WIDTH_ATX);
   KDint height = kdGetImageIntATX(compimage, KD_IMAGE_HEIGHT_ATX);
   TEST_EXPR(width != 0 && height != 0);
   KDint size = kdGetImageIntATX(compimage, KD_IMAGE_DATASIZE_ATX);

   KDchar *data = kdGetImagePointerATX(compimage, KD_IMAGE_POINTER_BUFFER_ATX);
   TEST_EXPR(data != KD_NULL);

   KDFile* file = kdFopen("debug.dds", "wb");
   TEST_EXPR(file != KD_NULL);

   kdFwrite(&magic, 4, 1, file);

   DDS_PIXELFORMAT pixelformat;
   kdMemset(&pixelformat, 0, sizeof(pixelformat));

   pixelformat.dwSize = 32;
   pixelformat.dwFlags = DDPF_FOURCC;
   pixelformat.dwFourCC = FOURCC('D','X','T','1');

   DDS_HEADER header;
   kdMemset(&header, 0, sizeof(header));

   header.dwSize = size;
   header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
   header.dwHeight = height;
   header.dwWidth = width;

   KDint blocksize = 8;
   header.dwPitchOrLinearSize = kdMaxVEN( 1, ((width+3)/4) ) * blocksize;
   header.ddspf = pixelformat;
   header.dwCaps = DDSCAPS_TEXTURE;

   kdFwrite(&header, sizeof(header), 1, file);
   kdFwrite(data, size, 1, file);

   kdFclose(file);
   kdFreeImageATX(compimage);
   kdFreeImageATX(image);
   return 0;
}
