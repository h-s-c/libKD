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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#include "kdplatform.h"  // for KDssize, KDsize
#include <KD/kd.h>       // for KDint, KDchar, KDuint
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // IWYU pragma: keep

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(KD_FREESTANDING)
#include <errno.h>  // for errno
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
#include <syscall.h>  // for SYS_open, SYS_read, SYS_write
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#endif

/******************************************************************************
 * Syscalls
 ******************************************************************************/

#if !defined(_WIN32)
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
inline static long __kdSyscall3(KDint nr, long arga, long argb, long argc)
{
    long result = 0;
    __asm__ __volatile__(
        "syscall"
        : "=a"(result)
        : "0"(nr), "D"(arga), "S"(argb), "d"(argc)
        : "cc", "rcx", "r11", "memory");
    return result;
}

inline static long __kdSyscallRes(long result)
{
    if(result >= -4095 && result <= -1)
    {
        errno = (KDint)-result;
        return -1;
    }
    else
    {
        return result;
    }
}
#endif

KDssize __kdWrite(KDint fd, const void *buf, KDsize count)
{
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
    long result = __kdSyscall3(SYS_write, (long)fd, (long)buf, (long)count);
    return (KDssize)__kdSyscallRes(result);
#else
    return write(fd, buf, count);
#endif
}

KDssize __kdRead(KDint fd, void *buf, KDsize count)
{
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
    long result = __kdSyscall3(SYS_read, (long)fd, (long)buf, (long)count);
    return (KDssize)__kdSyscallRes(result);
#else
    return read(fd, buf, count);
#endif
}

KDint __kdOpen(const KDchar *pathname, KDint flags, KDuint mode)
{
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
    long result = __kdSyscall3(SYS_open, (long)pathname, (long)flags, (long)mode);
    return (KDint)__kdSyscallRes(result);
#else
    return open(pathname, flags, mode);
#endif
}
#endif
