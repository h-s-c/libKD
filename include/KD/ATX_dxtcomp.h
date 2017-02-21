
/*******************************************************
 * OpenKODE Core extension: KD_ATX_dxtcomp
 *******************************************************/
/* Sample KD/ATX_dxtcomp.h for OpenKODE Core */
#ifndef __kd_ATX_dxtcomp_h_
#define __kd_ATX_dxtcomp_h_
#include <KD/ATX_imgdec_pvr.h>

#ifdef __cplusplus
extern "C" {
#endif



/* kdDXTCompressImageATX, kdDXTCompressBufferATX: Compresses an image into a DXT format. */
KD_API KDImageATX KD_APIENTRY kdDXTCompressImageATX(KDImageATX image, KDint32 comptype);
KD_API KDImageATX KD_APIENTRY kdDXTCompressBufferATX(const void *buffer, KDint32 width, KDint32 height, KDint32 compType, KDint32 levels);
#define KD_DXTCOMP_TYPE_DXT1_ATX 144
#define KD_DXTCOMP_TYPE_DXT1A_ATX 145
#define KD_DXTCOMP_TYPE_DXT3_ATX 146
#define KD_DXTCOMP_TYPE_DXT5_ATX 147

#ifdef __cplusplus
}
#endif

#endif /* __kd_ATX_dxtcomp_h_ */

