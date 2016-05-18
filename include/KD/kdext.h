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

#ifndef __kdext_h_
#define __kdext_h_

#include <KD/KHR_float64.h>
#include <KD/KHR_formatted.h>
#include <KD/KHR_perfcounter.h>
#include <KD/KHR_thread_storage.h>

#include <KD/VEN_atomic.h>
#include <KD/VEN_keyboard.h>
#include <KD/VEN_queue.h>

/******************************************************************************
 * String and memory functions (extensions)
 ******************************************************************************/

/* kdStrstr: Locate substring. */
KD_API KDchar* KD_APIENTRY kdStrstrVEN(const KDchar *str1, const KDchar *str2);

#endif /* __kdext_h_ */
