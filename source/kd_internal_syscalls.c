// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2017 Kevin Schmidt
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
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wpadded"
#   pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#include "kd_internal.h"

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__linux__)
#   include <unistd.h>
#endif

/******************************************************************************
 * Syscalls
 ******************************************************************************/

KDssize __kdWrite(KDint fd, const void *buf, KDsize count)
{
    KDssize retval;
#if defined(__linux__) && defined(__x86_64__)
    __asm__ volatile
    (
        "syscall"
        : "=a" (retval)
        : "0"(1), "D"(fd), "S"(buf), "d"(count)
        : "cc", "rcx", "r11", "memory"
    );
#elif defined(__linux__) && defined(__i386__)
    __asm__ volatile
    (
        "int $0x80"
        : "=a" (retval)
        : "0"(4), "b"(fd), "c"(buf), "d"(count)
        : "cc", "edi", "esi", "memory"
    );
#else
    retval = write(fd, buf, count);
#endif
    return retval;
}
