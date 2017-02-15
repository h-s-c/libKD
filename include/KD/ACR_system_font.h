
/*******************************************************
 * OpenKODE Core extension: ACR_system_font
 *******************************************************/

#ifndef __kd_ACR_system_font_h_
#define __kd_ACR_system_font_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KD_SYSTEM_FONT_TYPE_SANSSERIF_ACR   0x00000000
#define KD_SYSTEM_FONT_TYPE_MONOSPACE_ACR   0x00000001
#define KD_SYSTEM_FONT_TYPE_SERIF_ACR       0x00000002

#define KD_SYSTEM_FONT_FLAG_BOLD_ACR                0x00000001
#define KD_SYSTEM_FONT_FLAG_ITALIC_ACR              0x00000002
#define KD_SYSTEM_FONT_FLAG_OVERFLOW_WRAP_ACR       0x00000004
#define KD_SYSTEM_FONT_FLAG_OVERFLOW_ELLIPSIS_ACR   0x00000008

KD_API KDint KD_APIENTRY kdSystemFontGetTextSizeACR(KDint32 size, KDint32 locale, KDint32 type, KDint32 flag, const KDchar * utf8string, KDint32 w, KDint32 * result_w, KDint32 * result_h);
KD_API KDint KD_APIENTRY kdSystemFontRenderTextACR(KDint32 size, KDint32 locale, KDint32 type, KDint32 flag, const KDchar * utf8string, KDint32 w, KDint32 h, KDint32 pitch, void * buffer);

#ifdef __cplusplus
}
#endif

#endif /* __kd_ACR_system_font_h_ */
