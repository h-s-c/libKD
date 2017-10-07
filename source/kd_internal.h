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

typedef struct _KDCallback _KDCallback;
typedef struct _KDImageATX _KDImageATX;
typedef struct _KDQueue _KDQueue;

struct KDFile {
#if defined(_WIN32)
    HANDLE nativefile;
#else
    KDint nativefile;
    KDint8 padding[4];
#endif
    const KDchar *pathname;
    KDboolean eof;
    KDboolean error;
};

#if defined(KD_THREAD_POSIX)
#   include <pthread.h>
#elif defined(KD_THREAD_C11)
#   include <threads.h>
#endif

struct KDThread {
#if defined(KD_THREAD_C11)
    thrd_t nativethread;
#elif defined(KD_THREAD_POSIX)
    pthread_t nativethread;
#elif defined(KD_THREAD_WIN32)
    HANDLE nativethread;
#endif
    void *(*start_routine)(void *);
    void *arg;
    const KDThreadAttr *attr;
    _KDQueue *eventqueue;
    KDEvent *lastevent;
    KDint lasterror;
    KDint callbackindex;
    _KDCallback **callbacks;
    void *tlsptr;
};

struct _KDImageATX {
    KDuint8 *buffer;
    KDsize size;
    KDint width;
    KDint height;
    KDint levels;
    KDint bpp;
    KDint format;
    KDboolean alpha;
};

KDThread *__kdThreadInit(void);
void __kdThreadInitOnce(void);
void __kdThreadFree(KDThread *thread);

void __kdCleanupThreadStorageKHR(void);

_KDQueue* __kdQueueCreate(KDsize size);
KDint __kdQueueFree(_KDQueue* queue);
KDsize __kdQueueSize(_KDQueue *queue);
KDint __kdQueuePush(_KDQueue *queue, void *value);
void* __kdQueuePull(_KDQueue *queue);


extern KDThreadOnce __kd_threadinit_once;
extern KDThreadStorageKeyKHR __kd_threadlocal;
extern KDThreadMutex *__kd_tls_mutex;
