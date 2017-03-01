
/*******************************************************
 * OpenKODE Core extension: KD_ATX_imgdec
 *******************************************************/

#ifndef __kd_ATX_imgdec_h_
#define __kd_ATX_imgdec_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void *KDImageATX;


/* kdGetImageInfoATX, kdGetImageInfoFromStreamATX: Construct an informational image object based on an image in a file or stream. */
KD_API KDImageATX KD_APIENTRY kdGetImageInfoATX(const KDchar *pathname);
KD_API KDImageATX KD_APIENTRY kdGetImageInfoFromStreamATX(KDFile *file);

/* kdGetImageATX, kdGetImageFromStreamATX: Read and decode an image from a file or stream, returning a decoded image object. */
KD_API KDImageATX KD_APIENTRY kdGetImageATX(const KDchar *pathname, KDint format, KDint flags);
KD_API KDImageATX KD_APIENTRY kdGetImageFromStreamATX(KDFile *file, KDint format, KDint flags);
#define KD_IMAGE_FORMAT_RGBA8888_ATX 121
#define KD_IMAGE_FORMAT_RGB888_ATX 128
#define KD_IMAGE_FORMAT_RGB565_ATX 129
#define KD_IMAGE_FORMAT_RGB5551_ATX 130
#define KD_IMAGE_FORMAT_RGBA4444_ATX 131
#define KD_IMAGE_FORMAT_BGRA8888_ATX 132
#define KD_IMAGE_FORMAT_ALPHA8_ATX 133
#define KD_IMAGE_FORMAT_LUM8_ATX 134
#define KD_IMAGE_FORMAT_LUMALPHA88_ATX 135
#define KD_IMAGE_FORMAT_COMPRESSED_ATX 122
#define KD_IMAGE_FLAG_FLIP_X_ATX 1
#define KD_IMAGE_FLAG_FLIP_Y_ATX 2

/* kdFreeImageATX: Free image object. */
KD_API void KD_APIENTRY kdFreeImageATX(KDImageATX image);

/* kdGetImagePointerATX: Get the value of an image pointer attribute. */
KD_API void *KD_APIENTRY kdGetImagePointerATX(KDImageATX image, KDint attr);
#define KD_IMAGE_POINTER_BUFFER_ATX 112

/* kdGetImageIntATX, kdGetImageLevelIntATX: Get the value of an image integer attribute. */
KD_API KDint KD_APIENTRY kdGetImageIntATX(KDImageATX image, KDint attr);
KD_API KDint KD_APIENTRY kdGetImageLevelIntATX(KDImageATX image, KDint attr, KDint level);
#define KD_IMAGE_WIDTH_ATX 113
#define KD_IMAGE_HEIGHT_ATX 114
#define KD_IMAGE_FORMAT_ATX 115
#define KD_IMAGE_FORMAT_RGB_ATX 136
#define KD_IMAGE_FORMAT_RGBA_ATX 137
#define KD_IMAGE_FORMAT_PALETTED_ATX 138
#define KD_IMAGE_FORMAT_LUMINANCE_ATX 139
#define KD_IMAGE_FORMAT_LUMALPHA_ATX 140
#define KD_IMAGE_STRIDE_ATX 116
#define KD_IMAGE_BITSPERPIXEL_ATX 117
#define KD_IMAGE_LEVELS_ATX 118
#define KD_IMAGE_DATASIZE_ATX 119
#define KD_IMAGE_BUFFEROFFSET_ATX 120
#define KD_IMAGE_ALPHA_ATX 141

#ifdef __cplusplus
}
#endif

#endif /* __kd_ATX_imgdec_h_ */

