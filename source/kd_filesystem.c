// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#define _GNU_SOURCE /* O_CLOEXEC */
#endif
#include "kdplatform.h"  // for KD_API, KD_APIENTRY, KDsize
#include <KD/kd.h>       // for KDint, KDFile, kdFree, KDchar
#include <KD/kdext.h>    // for kdSetErrorPlatformVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // for KDFile

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32)
#include <errno.h>  // for errno, EINTR, ENOTDIR, ENOTE...
#include <stdio.h>  // for remove, rename
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
// IWYU pragma: no_include <bits/types/struct_timespec.h>
// IWYU pragma: no_include <time.h>
#include <unistd.h>    // for lseek, access, close, fsync
#include <fcntl.h>     // for O_CREAT, O_WRONLY, SEEK_CUR
#include <dirent.h>    // for closedir, opendir, readdir, DIR
#include <sys/stat.h>  // for stat, mkdir, S_IRUSR, S_IWUSR
#if defined(__APPLE__) || defined(BSD)
#include <sys/mount.h>  // for statfs
#else
#include <sys/statfs.h>  // for statfs
#endif
#undef st_mtime /* OpenKODE uses this */
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h> /* R_OK etc. */
#endif

/******************************************************************************
 * File system
 ******************************************************************************/

/* kdFopen: Open a file from the file system. */
KD_API KDFile *KD_APIENTRY kdFopen(const KDchar *pathname, const KDchar *mode)
{
    KDFile *file = (KDFile *)kdMalloc(sizeof(KDFile));
    if(file == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    file->pathname = pathname;
#if defined(_WIN32)
    DWORD access = 0;
    DWORD create = 0;
    KDboolean append = 0;
#else
    KDint access = 0;
    mode_t create = 0;
#endif

    switch(mode[0])
    {
        case 'w':
        {
#if defined(_WIN32)
            access = GENERIC_WRITE;
            create = CREATE_ALWAYS;
#else
            access = O_WRONLY | O_CREAT;
            create = S_IRUSR | S_IWUSR;
#endif
            break;
        }
        case 'r':
        {
#if defined(_WIN32)
            access = GENERIC_READ;
            create = OPEN_EXISTING;
#else
            access = O_RDONLY;
#endif
            break;
        }
        case 'a':
        {
#if defined(_WIN32)
            access = GENERIC_READ | GENERIC_WRITE;
            create = OPEN_ALWAYS;
            append = 1;
#else
            access = O_WRONLY | O_CREAT | O_APPEND;
#endif
            break;
        }
        default:
        {
            kdSetError(KD_EINVAL);
            return KD_NULL;
        }
    }

    for(KDint i = 1; mode[i] != (KDchar)'\0'; i++)
    {
        switch(mode[i])
        {
            case 'b':
            {
                break;
            }
            case '+':
            {
#if defined(_WIN32)
                access = GENERIC_READ | GENERIC_WRITE;
#else
                access = O_RDWR | O_CREAT;
                create = S_IRUSR | S_IWUSR;
#endif
                break;
            }
            default:
            {
                kdSetError(KD_EINVAL);
                return KD_NULL;
            }
        }
    }

#if defined(_WIN32)
    file->nativefile = CreateFileA(pathname, access, FILE_SHARE_READ | FILE_SHARE_WRITE, KD_NULL, create, 0, KD_NULL);
    if(file->nativefile != INVALID_HANDLE_VALUE)
    {
        if(append)
        {
            SetFilePointer(file->nativefile, 0, KD_NULL, FILE_END);
        }
    }
    else
    {
        KDint error = GetLastError();
#else
    file->nativefile = open(pathname, access | O_CLOEXEC, create);
    if(file->nativefile == -1)
    {
        KDint error = errno;
#endif
        kdFree(file);
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EISDIR | KD_EMFILE | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        return KD_NULL;
    }
    file->eof = KD_FALSE;
    return file;
}

/* kdFclose: Close an open file. */
KD_API KDint KD_APIENTRY kdFclose(KDFile *file)
{
    KDint retval = 0;
#if defined(_WIN32)
    retval = CloseHandle(file->nativefile);
    if(retval == 0)
    {
        KDint error = GetLastError();
#else
    retval = close(file->nativefile);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        kdFree(file);
        return KD_EOF;
    }
    kdFree(file);
    return 0;
}

/* kdFflush: Flush an open file. */
KD_API KDint KD_APIENTRY kdFflush(KD_UNUSED KDFile *file)
{
#if !defined(_WIN32)
    KDint retval = fsync(file->nativefile);
    if(retval == -1)
    {
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(errno, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
#endif
    return 0;
}

/* kdFread: Read from a file. */
KD_API KDsize KD_APIENTRY kdFread(void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDssize retval = 0;
    KDsize length = count * size;
#if defined(_WIN32)
    BOOL success = ReadFile(file->nativefile, buffer, (DWORD)length, (LPDWORD)&retval, KD_NULL);
    if(success == TRUE)
    {
        length -= (KDsize)retval;
    }
    else
    {
        KDint error = GetLastError();
#else
    KDchar *temp = buffer;
    while(length != 0 && (retval = read(file->nativefile, temp, size)) != 0)
    {
        if(retval == -1)
        {
            if(errno != EINTR)
            {
                break;
            }
        }
        length -= (KDsize)retval;
        temp += retval;
    }
    if(retval == -1)
    {
        KDint error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
    }
    if(length == 0)
    {
        file->eof = KD_TRUE;
    }
    return (KDsize)(count - (length / size));
}

/* kdFwrite: Write to a file. */
KD_API KDsize KD_APIENTRY kdFwrite(const void *buffer, KDsize size, KDsize count, KDFile *file)
{
    KDssize retval = 0;
    KDsize length = count * size;
#if defined(_WIN32)
    BOOL success = WriteFile(file->nativefile, buffer, (DWORD)length, (LPDWORD)&retval, KD_NULL);
    if(success == TRUE)
    {
        length -= (KDsize)retval;
    }
    else
    {
        KDint error = GetLastError();
#else
    KDchar *temp = kdMalloc(length);
    KDchar *_temp = temp;
    kdMemcpy(temp, buffer, length);
    while(length != 0 && (retval = write(file->nativefile, temp, size)) != 0)
    {
        if(retval == -1)
        {
            if(errno != EINTR)
            {
                break;
            }
        }
        length -= (KDsize)retval;
        temp += retval;
    }
    kdFree(_temp);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EBADF | KD_EFBIG | KD_ENOMEM | KD_ENOSPC);
    }
    if(length == 0)
    {
        file->eof = KD_TRUE;
    }
    return (KDsize)(count - (length / size));
}

/* kdGetc: Read next byte from an open file. */
KD_API KDint KD_APIENTRY kdGetc(KDFile *file)
{
    KDuint8 byte = 0;
#if defined(_WIN32)
    DWORD byteswritten = 0;
    BOOL success = ReadFile(file->nativefile, &byte, 1, &byteswritten, KD_NULL);
    if(success == TRUE && byteswritten == 0)
    {
        file->eof = KD_TRUE;
        return KD_EOF;
    }
    else if(success == FALSE)
    {
        KDint error = GetLastError();
#else
    KDint success = (KDint)read(file->nativefile, &byte, 1);
    if(success == 0)
    {
        file->eof = KD_TRUE;
        return KD_EOF;
    }
    else if(success == -1)
    {
        KDint error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EIO | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
    return (KDint)byte;
}

/* kdPutc: Write a byte to an open file. */
KD_API KDint KD_APIENTRY kdPutc(KDint c, KDFile *file)
{
    KDuint8 byte = c & 0xFF;
#if defined(_WIN32)
    BOOL success = WriteFile(file->nativefile, &byte, 1, (DWORD[]) {0}, KD_NULL);
    if(success == FALSE)
    {
        KDint error = GetLastError();
#else
    KDint success = (KDint)write(file->nativefile, &byte, 1);
    if(success == -1)
    {
        KDint error = errno;
#endif
        file->error = KD_TRUE;
        kdSetErrorPlatformVEN(error, KD_EBADF | KD_EFBIG | KD_ENOMEM | KD_ENOSPC);
        return KD_EOF;
    }
    return (KDint)byte;
}

/* kdFgets: Read a line of text from an open file. */
KD_API KDchar *KD_APIENTRY kdFgets(KDchar *buffer, KDsize buflen, KDFile *file)
{
    KDchar *line = buffer;
    for(KDsize i = buflen; i > 1; --i)
    {
        KDint character = kdGetc(file);
        if(character == KD_EOF)
        {
            break;
        }
        *line++ = (KDchar)character;
        if(character == '\n')
        {
            break;
        }
    }
    *line++ = (KDchar)'\0';
    return line;
}

/* kdFEOF: Check for end of file. */
KD_API KDint KD_APIENTRY kdFEOF(KDFile *file)
{
    if(file->eof == KD_TRUE)
    {
        return KD_EOF;
    }
    return 0;
}

/* kdFerror: Check for an error condition on an open file. */
KD_API KDint KD_APIENTRY kdFerror(KDFile *file)
{
    if(file->error == KD_TRUE)
    {
        return KD_EOF;
    }
    return 0;
}

/* kdClearerr: Clear a file's error and end-of-file indicators. */
KD_API void KD_APIENTRY kdClearerr(KDFile *file)
{
    file->error = KD_FALSE;
    file->eof = KD_FALSE;
}

/* TODO: Cleanup */
typedef struct {
#if defined(_MSC_VER)
    KDint seekorigin_kd;
#else
    KDuint seekorigin_kd;
#endif
#if defined(_WIN32)
    DWORD seekorigin;
#else
    KDint seekorigin;
#endif
} _KDSeekOrigin;

#if defined(_WIN32)
static _KDSeekOrigin seekorigins[] = {{KD_SEEK_SET, FILE_BEGIN}, {KD_SEEK_CUR, FILE_CURRENT}, {KD_SEEK_END, FILE_END}};
#else
static _KDSeekOrigin seekorigins[] = {{KD_SEEK_SET, SEEK_SET}, {KD_SEEK_CUR, SEEK_CUR}, {KD_SEEK_END, SEEK_END}};
#endif

/* kdFseek: Reposition the file position indicator in a file. */
KD_API KDint KD_APIENTRY kdFseek(KDFile *file, KDoff offset, KDfileSeekOrigin origin)
{
    KDint error = 0;
    for(KDuint64 i = 0; i < sizeof(seekorigins) / sizeof(seekorigins[0]); i++)
    {
        if(seekorigins[i].seekorigin_kd == origin)
        {
#if defined(_WIN32)
            DWORD retval = SetFilePointer(file->nativefile, (LONG)offset, KD_NULL, seekorigins[i].seekorigin);
            if(retval == INVALID_SET_FILE_POINTER)
            {
                error = GetLastError();
#else
            KDoff retval = (KDint)lseek(file->nativefile, (KDint32)offset, seekorigins[i].seekorigin);
            if(retval == (KDoff)-1)
            {
                error = errno;
#endif
                kdSetErrorPlatformVEN(error, KD_EFBIG | KD_EINVAL | KD_EIO | KD_ENOMEM | KD_ENOSPC | KD_EOVERFLOW);
                return -1;
            }
            break;
        }
    }
    return 0;
}

/* kdFtell: Get the file position of an open file. */
KD_API KDoff KD_APIENTRY kdFtell(KDFile *file)
{
    KDoff position = 0;
#if defined(_WIN32)
    position = (KDoff)SetFilePointer(file->nativefile, 0, KD_NULL, FILE_CURRENT);
    if(position == INVALID_SET_FILE_POINTER)
    {
        KDint error = GetLastError();
#else
    position = lseek(file->nativefile, 0, SEEK_CUR);
    if(position == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EOVERFLOW);
        return -1;
    }
    return position;
}

/* kdMkdir: Create new directory. */
KD_API KDint KD_APIENTRY kdMkdir(const KDchar *pathname)
{
    KDint retval = 0;
#if defined(_WIN32)
    retval = CreateDirectoryA(pathname, KD_NULL);
    if(retval == 0)
    {
        KDint error = GetLastError();
#else
    retval = mkdir(pathname, 0777);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EEXIST | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        return -1;
    }
    return 0;
}

/* kdRmdir: Delete a directory. */
KD_API KDint KD_APIENTRY kdRmdir(const KDchar *pathname)
{
    KDint retval = 0;
#if defined(_WIN32)
    retval = RemoveDirectoryA(pathname);
    if(retval == 0)
    {
        KDint error = GetLastError();
#else
    retval = rmdir(pathname);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EEXIST | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdRename: Rename a file. */
KD_API KDint KD_APIENTRY kdRename(const KDchar *src, const KDchar *dest)
{
    KDint retval = 0;
#if defined(_WIN32)
    retval = MoveFileA(src, dest);
    if(retval == 0)
    {
        KDint error = GetLastError();
#if defined(__MINGW32__)
        if(error == ERROR_ALREADY_EXISTS || error == ERROR_SEEK || error == ERROR_SHARING_VIOLATION || error == ERROR_ACCESS_DENIED)
#else
        if(error == ERROR_ALREADY_EXISTS || error == ERROR_SEEK || error == ERROR_SHARING_VIOLATION)
#endif
        {
            kdSetError(KD_EINVAL);
        }
        else
#else
    retval = rename(src, dest);
    if(retval == -1)
    {
        KDint error = errno;
        if(error == ENOTDIR || error == ENOTEMPTY || error == EISDIR)
        {
            kdSetError(KD_EINVAL);
        }
        else
#endif
        {
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSPC);
        }
        return -1;
    }
    return 0;
}

/* kdRemove: Delete a file. */
KD_API KDint KD_APIENTRY kdRemove(const KDchar *pathname)
{
    KDint retval = 0;
#if defined(_WIN32)
    retval = DeleteFileA(pathname);
    if(retval == 0)
    {
        KDint error = GetLastError();
#else
    retval = remove(pathname);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EBUSY | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdTruncate: Truncate or extend a file. */
KD_API KDint KD_APIENTRY kdTruncate(KD_UNUSED const KDchar *pathname, KDoff length)
{
    KDint retval = 0;
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    kdMemset(&data, 0, sizeof(data));
    HANDLE file = FindFirstFileA(pathname, &data);
    retval = (KDint)SetFileValidData(file, (LONGLONG)length);
    FindClose(file);
    if(retval == 0)
    {
        KDint error = GetLastError();
#else
    retval = truncate(pathname, (off_t)length);
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdStat, kdFstat: Return information about a file. */
KD_API KDint KD_APIENTRY kdStat(const KDchar *pathname, struct KDStat *buf)
{
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    kdMemset(&data, 0, sizeof(data));
    if(FindFirstFileA(pathname, &data) != INVALID_HANDLE_VALUE)
    {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            buf->st_mode = 0x4000;
        }
        else if(data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL || data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
        {
            buf->st_mode = 0x8000;
        }
        else
        {
            kdSetError(KD_EACCES);
            return -1;
        }
        LARGE_INTEGER size;
        size.LowPart = data.nFileSizeLow;
        size.HighPart = data.nFileSizeHigh;
        buf->st_size = size.QuadPart;

        ULARGE_INTEGER time;
        time.LowPart = data.ftLastWriteTime.dwLowDateTime;
        time.HighPart = data.ftLastWriteTime.dwHighDateTime;
        /* See RtlTimeToSecondsSince1970 */
        buf->st_mtime = (KDtime)((time.QuadPart / 10000000) - 11644473600LL);
    }
    else
    {
        KDint error = GetLastError();
#else
    struct stat posixstat;
    kdMemset(&posixstat, 0, sizeof(posixstat));
    if(stat(pathname, &posixstat) == 0)
    {
        if(posixstat.st_mode & S_IFDIR)
        {
            buf->st_mode = 0x4000;
        }
        else if(posixstat.st_mode & S_IFREG)
        {
            buf->st_mode = 0x8000;
        }
        else
        {
            kdSetError(KD_EACCES);
            return -1;
        }
        buf->st_size = posixstat.st_size;
#if defined(__APPLE__)
        buf->st_mtime = posixstat.st_mtimespec.tv_sec;
#else
        /* We undef the st_mtime macro*/
        buf->st_mtime = posixstat.st_mtim.tv_sec;
#endif
    }
    else
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_EOVERFLOW);
        return -1;
    }
    return 0;
}

KD_API KDint KD_APIENTRY kdFstat(KDFile *file, struct KDStat *buf)
{
    return kdStat(file->pathname, buf);
}

/* kdAccess: Determine whether the application can access a file or directory. */
KD_API KDint KD_APIENTRY kdAccess(const KDchar *pathname, KDint amode)
{
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    kdMemset(&data, 0, sizeof(data));
    if(FindFirstFileA(pathname, &data) != INVALID_HANDLE_VALUE)
    {
        if(data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if(amode & KD_X_OK || amode & KD_R_OK)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        KDint error = GetLastError();
#else
    typedef struct _KDAccessMode {
        KDint accessmode_kd;
        KDint accessmode_posix;
    } _KDAccessMode;
    _KDAccessMode accessmodes[] = {{KD_R_OK, R_OK}, {KD_W_OK, W_OK}, {KD_X_OK, X_OK}};
    KDint accessmode = 0;
    for(KDuint64 i = 0; i < sizeof(accessmodes) / sizeof(accessmodes[0]); i++)
    {
        if(accessmodes[i].accessmode_kd & amode)
        {
            accessmode |= accessmodes[i].accessmode_posix;
        }
    }
    if(access(pathname, accessmode) == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdOpenDir: Open a directory ready for listing. */
struct KDDir {
#if defined(_WIN32)
    HANDLE nativedir;
#else
    DIR *nativedir;
#endif
    KDDirent *dirent;
    void *dirent_d_name;
};
KD_API KDDir *KD_APIENTRY kdOpenDir(const KDchar *pathname)
{
    if(kdStrcmp(pathname, "/") == 0)
    {
        kdSetError(KD_EACCES);
        return KD_NULL;
    }
    KDDir *dir = (KDDir *)kdMalloc(sizeof(KDDir));
    if(dir == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    dir->dirent = (KDDirent *)kdMalloc(sizeof(KDDirent));
    if(dir->dirent == KD_NULL)
    {
        kdFree(dir);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    dir->dirent_d_name = kdMalloc(sizeof(KDchar) * 256);
    if(dir->dirent_d_name == KD_NULL)
    {
        kdFree(dir->dirent);
        kdFree(dir);
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    dir->dirent->d_name = dir->dirent_d_name;
#if defined(_WIN32)
    KDchar dirpath[MAX_PATH];
    WIN32_FIND_DATA data;
    kdMemset(&data, 0, sizeof(data));
    if(kdStrcmp(pathname, ".") == 0)
    {
        GetCurrentDirectoryA(MAX_PATH, dirpath);
    }
    else
    {
        kdStrcpy_s(dirpath, MAX_PATH, pathname);
    }
    kdStrncat_s(dirpath, MAX_PATH, "/*", 2);
#if defined(_MSC_VER)
#pragma warning(suppress : 6102)
#endif
    dir->nativedir = FindFirstFileA((const KDchar *)dirpath, &data);
    if(dir->nativedir == INVALID_HANDLE_VALUE)
    {
        KDint error = GetLastError();
#else
    dir->nativedir = opendir(pathname);
    if(dir->nativedir == KD_NULL)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM);
        kdFree(dir->dirent_d_name);
        kdFree(dir->dirent);
        kdFree(dir);
        return KD_NULL;
    }
    return dir;
}

/* kdReadDir: Return the next file in a directory. */
KD_API KDDirent *KD_APIENTRY kdReadDir(KDDir *dir)
{
#if defined(_WIN32)
    WIN32_FIND_DATA data;
    kdMemset(&data, 0, sizeof(data));
    if(FindNextFileA(dir->nativedir, &data))
    {
        kdMemcpy(dir->dirent_d_name, data.cFileName, 256);
    }
    else
    {
        KDint error = GetLastError();
        if(error == ERROR_NO_MORE_FILES)
        {
            return KD_NULL;
        }
#else
    struct dirent *de = readdir(dir->nativedir);
    if(de != KD_NULL)
    {
        kdMemcpy(dir->dirent_d_name, de->d_name, 256);
    }
    else if(errno == 0)
    {
        return KD_NULL;
    }
    else
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EIO | KD_ENOMEM);
        return KD_NULL;
    }
    return dir->dirent;
}

/* kdCloseDir: Close a directory. */
KD_API KDint KD_APIENTRY kdCloseDir(KDDir *dir)
{
#if defined(_WIN32)
    FindClose(dir->nativedir);
#else
    closedir(dir->nativedir);
#endif
    kdFree(dir->dirent_d_name);
    kdFree(dir->dirent);
    kdFree(dir);
    return 0;
}

/* kdGetFree: Get free space on a drive. */
KD_API KDoff KD_APIENTRY kdGetFree(const KDchar *pathname)
{
    KDoff freespace = 0;
    const KDchar *temp = pathname;
#if defined(_WIN32)
    if(GetDiskFreeSpaceEx(temp, (PULARGE_INTEGER)&freespace, KD_NULL, KD_NULL) == 0)
    {
        KDint error = GetLastError();
#else
    struct statfs buf;
    kdMemset(&buf, 0, sizeof(buf));
    if(statfs(temp, &buf) == 0)
    {
        KDsize _freespace = (KDsize)(buf.f_bsize / 1024L) * (KDsize)buf.f_bavail;
        freespace = (KDoff)(_freespace);
    }
    else
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EACCES | KD_EIO | KD_ENAMETOOLONG | KD_ENOENT | KD_ENOMEM | KD_ENOSYS | KD_EOVERFLOW);
        return (KDoff)-1;
    }
    return freespace;
}

/* kdBasenameVEN: Returns the path component following the final '/'. */
KD_API KDchar *KD_APIENTRY kdBasenameVEN(const KDchar *pathname)
{
    KDchar *ptr = kdStrrchrVEN(pathname, '/');
    if(ptr)
    {
        return ptr + 1;
    }
    else
    {
        kdMemcpy(&ptr, &pathname, sizeof(pathname));
        return ptr;
    }
}
