// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#if defined(__clang__)
#if __has_warning("-Wreserved-identifier")
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#endif

/******************************************************************************
 * KD includes
 ******************************************************************************/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#include "kdplatform.h"         // for kdAssert, KDint32, KDsize, KD_API
#include <KD/kd.h>              // for KDuint8, kdFree, kdSetError, kdMemcpy
#include <KD/ATX_dxtcomp.h>     // for KD_DXTCOMP_TYPE_DXT1A_ATX, KD_DXTCOMP...
#include <KD/ATX_imgdec.h>      // for KDImageATX, KD_IMAGE_FORMAT_RGBA8888_ATX
#include <KD/ATX_imgdec_pvr.h>  // for KD_IMAGE_FORMAT_DXT1_ATX, KD_IMAGE_FO...
#include <KD/KHR_float64.h>     // for kdFabsKHR, kdCeilKHR, kdFloorKHR, kdP...
#include <KD/kdext.h>           // for kdMinVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // for _KDImageATX

/******************************************************************************
 * Thirdparty includes
 ******************************************************************************/

#define STBD_ABS kdAbs
#define STBD_FABS kdFabsKHR
#define STBD_MEMCPY kdMemcpy
#define STB_DXT_STATIC
#define STB_DXT_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-function"
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_dxt.h"  // for stb_compress_dxt_block, STB_DXT_NORMAL
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define STBIR_ASSERT kdAssert
#define STBIR_MEMSET kdMemset
#define STBIR_MALLOC(s, c) kdMalloc(s)
#define STBIR_FREE(p, c) kdFree(p)
#define STBIR_CEIL kdCeilKHR
#define STBIR_FABS kdFabsKHR
#define STBIR_FLOOR kdFloorKHR
#define STBIR_POW kdPowKHR
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#if defined __INTEL_COMPILER
#pragma warning push
#pragma warning disable 279
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#if __has_warning("-Wimplicit-int-float-conversion")
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wstring-conversion"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "stb_image_resize.h"  // for stbir_resize_uint8
#if defined __INTEL_COMPILER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/******************************************************************************
 * OpenKODE Core extension: KD_ATX_dxtcomp
 ******************************************************************************/

/* kdDXTCompressImageATX, kdDXTCompressBufferATX: Compresses an image into a DXT format. */
KD_API KDImageATX KD_APIENTRY kdDXTCompressImageATX(KDImageATX image, KDint32 comptype)
{
    _KDImageATX *_image = image;
    if(_image->format != KD_IMAGE_FORMAT_RGBA8888_ATX)
    {
        kdSetError(KD_EINVAL);
        return KD_NULL;
    }
    return kdDXTCompressBufferATX(_image->buffer, _image->width, _image->height, comptype, _image->levels);
}

/* TODO: Generate Mipmaps */
KD_API KDImageATX KD_APIENTRY kdDXTCompressBufferATX(const void *buffer, KDint32 width, KDint32 height, KDint32 comptype, KDint32 levels)
{
    _KDImageATX *image = (_KDImageATX *)kdMalloc(sizeof(_KDImageATX));
    if(image == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    image->levels = levels;
    image->width = width;
    image->height = height;

    switch(comptype)
    {
        case(KD_DXTCOMP_TYPE_DXT1_ATX):
        {
            image->format = KD_IMAGE_FORMAT_DXT1_ATX;
            image->alpha = KD_FALSE;
            break;
        }
        case(KD_DXTCOMP_TYPE_DXT1A_ATX):
        {
            kdFree(image);
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
        case(KD_DXTCOMP_TYPE_DXT3_ATX):
        {
            kdFree(image);
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
        case(KD_DXTCOMP_TYPE_DXT5_ATX):
        {
            kdFree(image);
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
        default:
        {
            kdFree(image);
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
    }
    image->bpp = image->alpha ? 16 : 8;
    image->size = image->width * image->height * image->bpp;

    image->buffer = kdMalloc((KDsize)image->size);
    if(image->buffer == KD_NULL)
    {
        kdFree(image);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }

    KDint position = 0;
    const KDuint8 *input = buffer;
    for(KDint y = 0; y < height; y += 4) 
    {
        for(KDint x = 0; x < width; x += 4) 
        {
            KDuint8 block[64];
            for(KDint by = 0; by < 4; ++by) 
            {
                for(KDint bx = 0; bx < 4; ++bx) 
                {
                    KDint ai = (by * 4) + bx;
                    KDint bi = ai * 4;
                    KDint xx = kdMinVEN(x + bx, width - 1);
                    KDint yy = kdMinVEN(y + by, height - 1);
                    KDint i = ((yy * width) + xx) * 4;
                    block[bi + 0] = input[i + 0];
                    block[bi + 1] = input[i + 1];
                    block[bi + 2] = input[i + 2];
                    block[bi + 3] = 0xFF;
                }
            }

            KDuint8 chunk[8];
            stb_compress_dxt_block(chunk, block, image->alpha, STB_DXT_HIGHQUAL);
            kdMemcpy(image->buffer + position, chunk, 8);
            position += 8;
        }
    }

    return image;
}
