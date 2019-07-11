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
#include <KD/kdext.h>
#include <KD/ACR_system_font.h>
#include "test.h"

#define STBIW_ASSERT kdAssert
#define STBIW_MALLOC kdMalloc
#define STBIW_REALLOC kdRealloc
#define STBIW_FREE kdFree
#define STBIW_MEMMOVE kdMemmove
#define STBIW_MEMCPY kdMemcpy
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static void stbi_stdio_write(void *context, void *data, int size)
{
   kdFwrite(data, 1, size, (KDFile *)context);
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
#if 1
    /* Skip test */
    return 0;
#endif

    KDint size = 32;
    const KDchar* utf8string = "This is a test. This is only a test.";

    KDint w = 0;
    KDint h = 0;
    kdSystemFontGetTextSizeACR(size, 0,  KD_SYSTEM_FONT_TYPE_SANSSERIF_ACR, KD_SYSTEM_FONT_FLAG_BOLD_ACR | KD_SYSTEM_FONT_FLAG_ITALIC_ACR, utf8string, 0, &w, &h);
    
    void *buffer = kdMalloc(w * h);
    KDint result = kdSystemFontRenderTextACR(size, 0, KD_SYSTEM_FONT_TYPE_SANSSERIF_ACR, KD_SYSTEM_FONT_FLAG_BOLD_ACR | KD_SYSTEM_FONT_FLAG_ITALIC_ACR, utf8string, w, h, 0, buffer);
    if(result == -1)
    {
        KDint errorcode = kdGetError();
        if(errorcode == KD_EIO)
        {
            /* No fonts found */
            return 0;
        }
        TEST_FAIL();
    }

#if 0
    KDFile *file = kdFopen("test_sysfont.png", "wb");
    stbi_write_png_to_func(stbi_stdio_write, file, w, h, 1, buffer, 0);
    kdFclose(file);
#endif

    kdFree(buffer);
    return 0;
}
