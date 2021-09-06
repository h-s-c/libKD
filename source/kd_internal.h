/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2021 Kevin Schmidt
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

struct KDFile
{
#if defined(__ANDROID__) || defined(_WIN32)
    void *nativefile;
#else
    KDint nativefile;
    KDint8 padding[4];
#endif
    KDchar pathname[4096];
    KDboolean eof;
    KDboolean error;
};

typedef struct _KDQueue _KDQueue;
typedef struct _KDCallback _KDCallback;
typedef struct _KDThreadInternal _KDThreadInternal;
struct KDThread
{
    _KDThreadInternal *internal;
    _KDQueue *eventqueue;
    KDEvent *lastevent;
    KDint lasterror;
    KDint callbackindex;
    _KDCallback **callbacks;
    void *tlsptr;
};

typedef struct _KDImageATX _KDImageATX;
struct _KDImageATX
{
    KDuint8 *buffer;
    KDsize size;
    KDint width;
    KDint height;
    KDint levels;
    KDint bpp;
    KDint format;
    KDboolean alpha;
};

#if defined(__ANDROID__)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
#include "miniz.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

typedef struct __KDApk __KDApk;
struct __KDApk
{
    KDint handle;
    char padding[4];
    void *data;
    mz_zip_archive *archive;
    KDsize size;
};

extern __KDApk *__kd_apk;
#endif

void __kdMallocInit(void);
void __kdMallocFinal(void);
void __kdMallocThreadInit(void);
void __kdMallocThreadFinal(void);

KDThread *__kdThreadInit(void);
void __kdThreadInitOnce(void);
void __kdThreadFree(KDThread *thread);

void __kdCleanupThreadStorageKHR(void);

_KDQueue *__kdQueueCreate(KDsize size);
KDint __kdQueueFree(_KDQueue *queue);
KDsize __kdQueueSize(_KDQueue *queue);
KDint __kdQueuePush(_KDQueue *queue, void *value);
void *__kdQueuePull(_KDQueue *queue);

#if defined(_WIN32) && defined(KD_FREESTANDING) && !defined(__MINGW32__)
KDint _fltused;
#endif

KDint __kdDecompressPVRTC(const KDuint8 *pCompressedData, KDint Do2bitMode, KDint XDim, KDint YDim, KDuint8 *pResultImage);

#if !defined(_WIN32)
KDint __kdPosixStat(const KDchar *pathname, struct KDStat *buf);
#endif
