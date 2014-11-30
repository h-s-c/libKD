// pthread.h - Simple pthreads
//
// Standalone Win32 implementation of the POSIX pthreads API.
// No disclaimer or attribution is required to use this library.

// This is free and unencumbered software released into the public domain.
// 
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
// 
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// For more information, please refer to <http://unlicense.org/>

#define WINVER 0x600
#define _WIN32_WINNT 0x0600

#include "semaphore.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <limits.h>
#include <errno.h>

struct sem_data_t
{
    HANDLE hSema;
};

int    sem_destroy(sem_t *sem)
{
    if (!sem || !sem->data)
        return EINVAL;

    CloseHandle(sem->data->hSema);
    freemem(sem->data);
    return 0;
}

int    sem_getvalue(sem_t *sem, int *sval)
{
    LONG count = 0;
    if (!sem || !sem->data || !sval)
    {
        errno = EINVAL;
        return -1;
    }

    if (WAIT_OBJECT_0 == WaitForSingleObject(sem->data->hSema, 0))
        ReleaseSemaphore(sem->data->hSema, 1, &count);

    *sval = (int)count;
    return 0;
}

int    sem_init(sem_t *sem, int pshared, unsigned value)
{
    if (!sem)
    {
        errno = EINVAL;
        return -1;
    }

    UNUSED(pshared); // not supported

    sem->data = (struct sem_data_t *)allocmem(sizeof(struct sem_data_t));
    if (!sem->data)
    {
        errno = ENOSPC;
        return -1;
    }

    sem->data->hSema = CreateSemaphoreW(NULL, value, LONG_MAX, NULL);
    if (!sem->data->hSema)
    {
        freemem(sem->data);
        errno = ENOSPC;
        return -1;
    }

    return 0;
}

int    sem_post(sem_t *sem)
{
    if (!sem || !sem->data)
    {
        errno = EINVAL;
        return -1;
    }

    if (!ReleaseSemaphore(sem->data->hSema, 1, NULL))
    {
        errno = EIO;
        return -1;
    }

    return 0;
}

int    sem_trywait(sem_t *sem)
{
    DWORD err;
    if (!sem || !sem->data)
    {
        errno = EINVAL;
        return -1;
    }

    err = WaitForSingleObject(sem->data->hSema, 0);
    if (err == WAIT_TIMEOUT)
    {
        errno = EAGAIN;
        return -1;
    }
    if (err != WAIT_OBJECT_0)
    {
        errno = EIO;
        return -1;
    }

    return 0;
}

int    sem_wait(sem_t *sem)
{
    DWORD err;
    if (!sem || !sem->data)
    {
        errno = EINVAL;
        return -1;
    }

    err = WaitForSingleObject(sem->data->hSema, INFINITE);
    if (err != WAIT_OBJECT_0)
    {
        errno = EIO;
        return -1;
    }

    return 0;
}
