
// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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
#include "kdplatform.h"
#include <KD/kd.h>
#include <KD/ACR_system_font.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * Thirdparty includes
 ******************************************************************************/

#define STBTT_ifloor(x) ((KDint)kdFloorf(x))
#define STBTT_iceil(x) ((KDint)kdCeilf(x))
#define STBTT_sqrt(x) kdSqrtf(x)
#define STBTT_pow(x, y) kdPowf(x, y)
#define STBTT_fmod(x, y) kdFmodf(x, y)
#define STBTT_cos(x) kdCosf(x)
#define STBTT_acos(x) kdAcosf(x)
#define STBTT_fabs(x) kdFabsf(x)
#define STBTT_malloc(x, u) ((void)(u), kdMalloc(x))
#define STBTT_free(x, u) ((void)(u), kdFree(x))
#define STBTT_assert(x) kdAssert(x)
#define STBTT_strlen(x) kdStrlen(x)
#define STBTT_memcpy kdMemcpy
#define STBTT_memset kdMemset
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wbad-function-cast"
#if __has_warning("-Wimplicit-float-conversion")
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#if __has_warning("-Wextra-semi-stmt")
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#endif
#if __has_warning("-Wimplicit-fallthrough")
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-conversion"
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include "stb_truetype.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/******************************************************************************
 * OpenKODE Core extension: KD_ACR_system_font
 ******************************************************************************/

static const KDchar __kd_sans_regular[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSans-Regular.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\arial.ttf",
    /* MacOS */
    "/Library/Fonts/Arial.ttf",
    /* Android */
    "/system/fonts/Roboto-Regular.ttf"};

static const KDchar __kd_sans_bold[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSans-Bold.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSans-Bold.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\arialbd.ttf",
    /* MacOS */
    "/Library/Fonts/Arial Bold.ttf",
    /* Android */
    "/system/fonts/Roboto-Bold.ttf"};

static const KDchar __kd_sans_bolditalic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSans-BoldItalic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSans-BoldItalic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSans-BoldItalic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSans-BoldItalic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\arialbd.ttf",
    /* MacOS */
    "/Library/Fonts/Arial Bold Italic.ttf",
    /* Android */
    "/system/fonts/Roboto-BoldItalic.ttf"};

static const KDchar __kd_sans_italic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSans-Italic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSans-Italic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSans-Italic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSans-Italic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\ariali.ttf",
    /* MacOS */
    "/Library/Fonts/Arial Italic.ttf",
    /* Android */
    "/system/fonts/Roboto-Italic.ttf"};

static const KDchar __kd_serif_regular[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSerif-Regular.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSerif-Regular.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSerif-Regular.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\times.ttf",
    /* MacOS */
    "/Library/Fonts/Times New Roman.ttf",
    /* Android */
    "/system/fonts/NotoSerif-Regular.ttf"};

static const KDchar __kd_serif_bold[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSerif-Bold.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSerif-Bold.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSerif-Bold.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSerif-Bold.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\timesbd.ttf",
    /* MacOS */
    "/Library/Fonts/Times New Roman Bold.ttf",
    /* Android */
    "/system/fonts/NotoSerif-Bold.ttf"};

static const KDchar __kd_serif_bolditalic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSerif-BoldItalic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSerif-BoldItalic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSerif-BoldItalic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSerif-BoldItalic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\timesbi.ttf",
    /* MacOS */
    "/Library/Fonts/Times New Roman Bold Italic.ttf",
    /* Android */
    "/system/fonts/NotoSerif-BoldItalic.ttf"};

static const KDchar __kd_serif_italic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationSerif-Italic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationSerif-Italic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationSerif-Italic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\timesi.ttf",
    /* MacOS */
    "/Library/Fonts/Times New Roman Italic.ttf",
    /* Android */
    "/system/fonts/NotoSerif-Italic.ttf"};

static const KDchar __kd_mono_regular[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationMono-Regular.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationMono-Regular.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\cour.ttf",
    /* MacOS */
    "/Library/Fonts/Courier New.ttf",
    /* Android */
    "/system/fonts/DroidSansMono.ttf"};

static const KDchar __kd_mono_bold[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationMono-Bold.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationMono-Bold.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationMono-Bold.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\courbd.ttf",
    /* MacOS */
    "/Library/Fonts/Courier New Bold.ttf",
    /* Android */
    ""};

static const KDchar __kd_mono_bolditalic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationMono-BoldItalic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationMono-BoldItalic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationMono-BoldItalic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationMono-BoldItalic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\courbi.ttf",
    /* MacOS */
    "/Library/Fonts/Courier New Bold Italic.ttf",
    /* Android */
    ""};

static const KDchar __kd_mono_italic[7][128] = {
    /* Ubuntu */
    "/usr/share/fonts/truetype/liberation/LiberationMono-Italic.ttf",
    /* Arch */
    "/usr/share/fonts/TTF/LiberationMono-Italic.ttf",
    /* Fedora/RHEL/CentOS */
    "/usr/share/fonts/liberation/LiberationMono-Italic.ttf",
    /* Opensuse */
    "/usr/share/fonts/truetype/LiberationMono-Italic.ttf",
    /* Windows */
    "C:\\Windows\\Fonts\\couri.ttf",
    /* MacOS */
    "/Library/Fonts/Courier New Italic.ttf",
    /* Android */
    ""};

static KDuint8 *__kdLoadFont(KDint32 type, KDint32 flag)
{
    KDFile *fontfile = KD_NULL;
    KDchar fontpaths[7][128];

    switch(type)
    {
        case(KD_SYSTEM_FONT_TYPE_SANSSERIF_ACR):
        {
            if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR && flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_sans_bolditalic, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_sans_bold, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_sans_italic, sizeof(fontpaths));
            }
            else
            {
                kdMemcpy(&fontpaths, &__kd_sans_regular, sizeof(fontpaths));
            }
            break;
        }
        case(KD_SYSTEM_FONT_TYPE_SERIF_ACR):
        {
            if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR && flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_serif_bolditalic, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_serif_bold, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_serif_italic, sizeof(fontpaths));
            }
            else
            {
                kdMemcpy(&fontpaths, &__kd_serif_regular, sizeof(fontpaths));
            }
            break;
        }
        case(KD_SYSTEM_FONT_TYPE_MONOSPACE_ACR):
        {
            if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR && flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_mono_bolditalic, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_BOLD_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_mono_bold, sizeof(fontpaths));
            }
            else if(flag & KD_SYSTEM_FONT_FLAG_ITALIC_ACR)
            {
                kdMemcpy(&fontpaths, &__kd_mono_italic, sizeof(fontpaths));
            }
            else
            {
                kdMemcpy(&fontpaths, &__kd_mono_regular, sizeof(fontpaths));
            }
            break;
        }
        default:
        {
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
    }

    for(KDuint64 i = 0; i < sizeof(fontpaths) / sizeof(fontpaths[0]); i++)
    {
        fontfile = kdFopen(fontpaths[i], "rb");
        if(fontfile)
        {
            break;
        }
    }

    if(!fontfile)
    {
        kdSetError(KD_EIO);
        return KD_NULL;
    }

    kdFseek(fontfile, 0, KD_SEEK_END);
    KDsize fontsize = (KDsize)kdFtell(fontfile);
    kdFseek(fontfile, 0, KD_SEEK_SET);

    KDuint8 *fontbuffer = kdMalloc(fontsize);

    kdFread(fontbuffer, fontsize, 1, fontfile);
    kdFclose(fontfile);

    return fontbuffer;
}

/* kdSystemFontGetTextSizeACR: Gets system font width and height. */
KD_API KDint KD_APIENTRY kdSystemFontGetTextSizeACR(KDint32 size, KDint32 locale, KDint32 type, KDint32 flag, const KDchar *utf8string, KD_UNUSED KDint32 w, KDint32 *result_w, KDint32 *result_h)
{
    if(locale)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }

    KDuint8 *font = __kdLoadFont(type, flag);

    stbtt_fontinfo info;
    kdMemset(&info, 0, sizeof(info));

    stbtt_InitFont(&info, font, 0);

    /* calculate font scaling */
    KDfloat32 scale = stbtt_ScaleForPixelHeight(&info, (KDfloat32)size);

    KDuint x = 0;
    for(KDsize i = 0; i < kdStrlen(utf8string); ++i)
    {
        /* how wide is this character */
        KDint ax;
        stbtt_GetCodepointHMetrics(&info, utf8string[i], &ax, 0);
        x += (KDuint)((KDfloat32)ax * scale);

        /* add kerning */
        KDint kern;
        kern = stbtt_GetCodepointKernAdvance(&info, utf8string[i], utf8string[i + 1]);
        x += (KDuint)((KDfloat32)kern * scale);
    }

    *result_w = (KDint)x;
    *result_h = size;

    kdFree(font);
    return 0;
}

/* kdSystemFontRenderTextACR: Stores a system font image to a buffer. */
KD_API KDint KD_APIENTRY kdSystemFontRenderTextACR(KDint32 size, KDint32 locale, KDint32 type, KDint32 flag, const KDchar *utf8string, KDint32 w, KD_UNUSED KDint32 h, KD_UNUSED KDint32 pitch, void *buffer)
{
    if(locale)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }

    KDuint8 *font = __kdLoadFont(type, flag);

    stbtt_fontinfo info;
    kdMemset(&info, 0, sizeof(info));

    stbtt_InitFont(&info, font, 0);

    /* calculate font scaling */
    KDfloat32 scale = stbtt_ScaleForPixelHeight(&info, (KDfloat32)size);

    KDuint x = 0;

    KDint ascent = 0;
    KDint descent = 0;
    KDint lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    ascent *= (KDint)scale;
    descent *= (KDint)scale;

    for(KDsize i = 0; i < kdStrlen(utf8string); ++i)
    {
        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        KDint c_x1 = 0;
        KDint c_y1 = 0;
        KDint c_x2 = 0;
        KDint c_y2 = 0;
        stbtt_GetCodepointBitmapBox(&info, utf8string[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        /* compute y (different characters have different heights */
        KDint y = ascent + c_y1;

        /* render character (stride and offset is important here) */
        KDint byteOffset = (KDint)x + (y * w);
        stbtt_MakeCodepointBitmap(&info, (KDuint8 *)buffer + byteOffset, c_x2 - c_x1, c_y2 - c_y1, w, scale, scale, utf8string[i]);

        /* how wide is this character */
        KDint ax;
        stbtt_GetCodepointHMetrics(&info, utf8string[i], &ax, 0);
        x += (KDuint)((KDfloat32)ax * scale);

        /* add kerning */
        KDint kern;
        kern = stbtt_GetCodepointKernAdvance(&info, utf8string[i], utf8string[i + 1]);
        x += (KDuint)((KDfloat32)kern * scale);
    }

    kdFree(font);
    return 0;
}
