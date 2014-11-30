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

#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Provide the basic definitions that sched.h would normally provide:
#ifndef SCHED_OTHER
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2

int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);

struct sched_param {
	int sched_priority;
};
#endif // SCHED_OTHER


// Thread flags.
#define PTHREAD_CANCEL_ASYNCHRONOUS		0x00000001
#define PTHREAD_CANCEL_ENABLE			0x00000002
#define PTHREAD_CREATE_JOINABLE			0x00000004
#define PTHREAD_INHERIT_SCHED			0x00000008
#define PTHREAD_CANCELED				0x00000010

// Inferred from their opposite flag.
#define PTHREAD_CANCEL_DEFERRED			0x00000000
#define PTHREAD_CANCEL_DISABLE			0x00000000
#define PTHREAD_CREATE_DETACHED			0x00000000
#define PTHREAD_EXPLICIT_SCHED			0x00000000
#define PTHREAD_PROCESS_PRIVATE			0x00000000
#define PTHREAD_SCOPE_PROCESS			0x00000000

// Mutex flags.
#define PTHREAD_MUTEX_NORMAL			0x00000000
#define PTHREAD_MUTEX_DEFAULT			0x00000000
#define PTHREAD_MUTEX_ERRORCHECK		0x00000001
#define PTHREAD_MUTEX_RECURSIVE			0x00000002
#define PTHREAD_PRIO_NONE				0x00000000
#define PTHREAD_PRIO_INHERIT			0x00000004
#define PTHREAD_PRIO_PROTECT			0x00000008

// Sharing. (unsupported)
#define PTHREAD_PROCESS_SHARED			0x00010000
#define PTHREAD_SCOPE_SYSTEM			0x00020000

// Static initializers.
#define PTHREAD_COND_INITIALIZER		{ NULL }
#define PTHREAD_MUTEX_INITIALIZER		{ NULL }
#define PTHREAD_ONCE_INIT				{ 0 }

#define PTHREAD_BARRIER_SERIAL_THREAD	9999

// Limits. These would normally be in <limits.h>
// These limits are mostly guesses - it's up to the OS really what we can get.
#define PTHREAD_DESTRUCTOR_ITERATIONS	16
#define PTHREAD_KEYS_MAX				1088
#define PTHREAD_STACK_MIN				4096
#define PTHREAD_THREADS_MAX				1024

// pthread types
typedef struct { size_t stack; int flags; int prio; } pthread_attr_t;
typedef struct { struct pthread_barrierdata_t *data; } pthread_barrier_t;
typedef struct { int flags; } pthread_barrierattr_t;
typedef struct { struct pthread_conddata_t *data; } pthread_cond_t;
typedef struct { int flags; } pthread_condattr_t;
typedef unsigned pthread_key_t;
typedef struct { struct pthread_mutexdata_t *data; } pthread_mutex_t;
typedef struct { int flags; } pthread_mutexattr_t;
typedef struct { void *ptr; } pthread_once_t;
typedef struct { struct pthread_rwlockdata_t *data;  } pthread_rwlock_t;
typedef struct { int flags; } pthread_rwlockattr_t;
typedef struct { struct pthread_spinlockdata_t *data; } pthread_spinlock_t;
typedef struct pthread_info_t *pthread_t;

// Non-POSIX Win32 extensions:

// Returns the Win32 HANDLE of a pthread.
void *pthread_gethandle(pthread_t thread);


// pthread functions:

// Threads attributes.
int   pthread_attr_destroy(pthread_attr_t *);
int   pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int   pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
int   pthread_attr_getinheritsched(const pthread_attr_t *, int *);
int   pthread_attr_getschedparam(const pthread_attr_t *, struct sched_param *);
int   pthread_attr_getschedpolicy(const pthread_attr_t *, int *);
int   pthread_attr_getscope(const pthread_attr_t *, int *);
int   pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int   pthread_attr_getstackaddr(const pthread_attr_t *, void **);
int   pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int   pthread_attr_init(pthread_attr_t *);
int   pthread_attr_setdetachstate(pthread_attr_t *, int);
int   pthread_attr_setguardsize(pthread_attr_t *, size_t);
int   pthread_attr_setinheritsched(pthread_attr_t *, int);
int   pthread_attr_setschedparam(pthread_attr_t *, const struct sched_param *);
int   pthread_attr_setschedpolicy(pthread_attr_t *, int);
int   pthread_attr_setscope(pthread_attr_t *, int);
int   pthread_attr_setstack(pthread_attr_t *, void *, size_t);
int   pthread_attr_setstackaddr(pthread_attr_t *, void *);
int   pthread_attr_setstacksize(pthread_attr_t *, size_t);

// Threads.
int   pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int   pthread_detach(pthread_t);
int   pthread_equal(pthread_t, pthread_t);
void  pthread_exit(void *);
int   pthread_join(pthread_t, void **);
pthread_t pthread_self(void);

// Condition attributes.
int   pthread_condattr_destroy(pthread_condattr_t *);
int   pthread_condattr_getpshared(const pthread_condattr_t *, int *);
int   pthread_condattr_init(pthread_condattr_t *);
int   pthread_condattr_setpshared(pthread_condattr_t *, int);

// Conditions.
int   pthread_cond_broadcast(pthread_cond_t *);
int   pthread_cond_destroy(pthread_cond_t *);
int   pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int   pthread_cond_signal(pthread_cond_t *);
int   pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);

// Barrier attributes.
int   pthread_barrierattr_destroy(pthread_barrierattr_t *);
int   pthread_barrierattr_getpshared(const pthread_barrierattr_t *, int *);
int   pthread_barrierattr_init(pthread_barrierattr_t *);
int   pthread_barrierattr_setpshared(pthread_barrierattr_t *, int);

// Barriers.
int   pthread_barrier_destroy(pthread_barrier_t *);
int   pthread_barrier_init(pthread_barrier_t *, const pthread_barrierattr_t *, unsigned);
int   pthread_barrier_wait(pthread_barrier_t *);

// Forking (unsupported).
int   pthread_atfork(void (*)(void), void (*)(void), void(*)(void));

// One-time initialization.
int   pthread_once(pthread_once_t *, void (*)(void));

// Priority tweaks.
int   pthread_getconcurrency(void);
int   pthread_getschedparam(pthread_t, int *, struct sched_param *);
int   pthread_setconcurrency(int);
int   pthread_setschedparam(pthread_t, int, const struct sched_param *);
int   pthread_setschedprio(pthread_t, int);

// Cancellation.
int   pthread_cancel(pthread_t);
void  pthread_cleanup_push(void (*)(void *), void *);
void  pthread_cleanup_pop(int);
void  pthread_testcancel(void);
int   pthread_setcancelstate(int, int *);
int   pthread_setcanceltype(int, int *);

// Mutex attributes.
int   pthread_mutexattr_destroy(pthread_mutexattr_t *);
int   pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_getprotocol(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int   pthread_mutexattr_init(pthread_mutexattr_t *);
int   pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int   pthread_mutexattr_settype(pthread_mutexattr_t *, int);

// Mutexes.
int   pthread_mutex_destroy(pthread_mutex_t *);
int   pthread_mutex_getprioceiling(const pthread_mutex_t *, int *);
int   pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int   pthread_mutex_lock(pthread_mutex_t *);
int   pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
int   pthread_mutex_trylock(pthread_mutex_t *);
int   pthread_mutex_unlock(pthread_mutex_t *);

// Thread-local storage (TLS)
void *pthread_getspecific(pthread_key_t);
int   pthread_setspecific(pthread_key_t, const void *);
int   pthread_key_create(pthread_key_t *, void (*)(void *));
int   pthread_key_delete(pthread_key_t);

// RW lock attributes.
int   pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
int   pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *, int *);
int   pthread_rwlockattr_init(pthread_rwlockattr_t *);
int   pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);

// RW locks.
int   pthread_rwlock_destroy(pthread_rwlock_t *);
int   pthread_rwlock_init(pthread_rwlock_t *, const pthread_rwlockattr_t *);
int   pthread_rwlock_rdlock(pthread_rwlock_t *);
int   pthread_rwlock_unlock(pthread_rwlock_t *);
int   pthread_rwlock_wrlock(pthread_rwlock_t *);

// Spinlocks.
int   pthread_spin_destroy(pthread_spinlock_t *);
int   pthread_spin_init(pthread_spinlock_t *, int);
int   pthread_spin_lock(pthread_spinlock_t *);
int   pthread_spin_trylock(pthread_spinlock_t *);
int   pthread_spin_unlock(pthread_spinlock_t *);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_H__
