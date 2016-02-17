/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2016 Kevin Schmidt
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

#ifndef __kd_VEN_keyboard_h
#define __kd_VEN_keyboard_h
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KDEventInputKeyVEN {
    KDuint32 flags;
    KDint32 keycode;
} KDEventInputKeyVEN;

typedef struct KDEventInputKeyCharVEN {
    KDuint32 flags;
    KDint32 character;
} KDEventInputKeyCharVEN;

#define KD_EVENT_INPUT_KEY_VEN      101
#define KD_EVENT_INPUT_KEYCHAR_VEN  102

#define KD_KEY_PRESS_VEN    0x300000
#define KD_KEY_UP_VEN       0x300001
#define KD_KEY_DOWN_VEN     0x300002
#define KD_KEY_LEFT_VEN     0x300003
#define KD_KEY_RIGHT_VEN    0x300004

#ifdef __cplusplus
}
#endif

#endif /* __kd_VEN_keyboard_h */
