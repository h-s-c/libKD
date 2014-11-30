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

#include "pthread.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <limits.h>
#include <errno.h>

#define cassert(X) typedef int __compile_time_assert__[(X)?1:-1]
#define UNUSED(X) (void)(X)

typedef struct pthread_cancel_t
{
	void (*fn)(void *arg);
	void *arg;
	struct pthread_cancel_t *next;
} pthread_cancel_t;

struct pthread_info_t
{
	HANDLE hThread;
	void *arg;
	void *result;
	void *(*start)(void *);
	pthread_cancel_t *cleanup;
	LONG refcount;
	int flags;
};

struct pthread_spinlockdata_t
{
	CRITICAL_SECTION crit;
};

struct pthread_mutexdata_t
{
	CRITICAL_SECTION crit;
};

struct pthread_conddata_t
{
	CONDITION_VARIABLE cond;
};

struct pthread_barrierdata_t
{
	unsigned count;
	LONG numWaiting;
	HANDLE hSema;
};

struct pthread_rwlockdata_t
{
	SRWLOCK lock;
	int exclusive;
};

typedef struct {
	DWORD slot;
	void (*fn)(void *value);
} pthread_key_destructor_t;

static pthread_once_t thread_setup_once = PTHREAD_ONCE_INIT;
static DWORD thread_setup_tls = TLS_OUT_OF_INDEXES;
static int pthread_concurrency = 0;

static unsigned key_count = 0;
static pthread_mutex_t key_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_destructor_t key_destructors[PTHREAD_KEYS_MAX];

static void *allocmem(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

static void freemem(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

static void pthread_unref(pthread_t thread)
{
	LONG count = InterlockedDecrement(&thread->refcount);
	if (count == 0)
	{
		CloseHandle(thread->hThread);
		freemem(thread);
	}
}

static void  pthread_docleanup(void *value_ptr)
{
	unsigned it, n, any;
	pthread_key_destructor_t *d;

	pthread_t thread = pthread_self();
	thread->result = value_ptr;

	// Run the cancellation handlers.
	while (thread->cleanup)
		pthread_cleanup_pop(1);

	// Run the key destructors.
	pthread_mutex_lock(&key_mutex);
	for (it=0;it<PTHREAD_DESTRUCTOR_ITERATIONS;it++)
	{
		any = 0;
		for (n=0;n<key_count;n++)
		{
			d = &key_destructors[n];
			if (d->fn)
			{
				void *value = pthread_getspecific(d->slot);
				if (value)
				{
					any = 1;
					pthread_setspecific(d->slot, NULL);
					d->fn(value);
				}
			}
		}

		if (!any)
			break;
	}
	pthread_mutex_unlock(&key_mutex);

	pthread_unref(thread);
}

static unsigned __stdcall pthread_main(void *arg)
{
	void *result;
	pthread_t thread = (pthread_t)arg;
	InterlockedIncrement(&thread->refcount);
	TlsSetValue(thread_setup_tls, thread);
	result = thread->start(thread->arg);
	pthread_docleanup(result);
	return 0;
}

static void thread_setup(void)
{
	thread_setup_tls = TlsAlloc();
}

static int pthread_cond_ondemand(pthread_cond_t *cond)
{
	pthread_cond_t tmp;
	int err;
	void *old;

	err = pthread_cond_init(&tmp, NULL);
	if (err)
		return err;

	// The old switcheroo. If someone else beat us, simply drop our copy and use theirs.
	old = InterlockedCompareExchangePointer((void **)&cond->data, tmp.data, NULL);
	if (old != NULL)
		pthread_cond_destroy(&tmp);

	return 0;
}

static int pthread_mutex_ondemand(pthread_mutex_t *mutex)
{
	pthread_mutex_t tmp;
	int err;
	void *old;

	err = pthread_mutex_init(&tmp, NULL);
	if (err)
		return err;

	// The old switcheroo. If someone else beat us, simply drop our copy and use theirs.
	old = InterlockedCompareExchangePointer((void **)&mutex->data, tmp.data, NULL);
	if (old != NULL)
		pthread_mutex_destroy(&tmp);

	return 0;
}

void *pthread_gethandle(pthread_t thread)
{
	return thread->hThread;
}

int sched_get_priority_max(int policy)
{
	UNUSED(policy);
	return THREAD_PRIORITY_TIME_CRITICAL;
}

int sched_get_priority_min(int policy)
{
	UNUSED(policy);
	return THREAD_PRIORITY_LOWEST;
}


int   pthread_atfork(void (*prepare)(void), void (*parent)(void), void(*child)(void))
{
	// Windows doesn't have fork, nothing we can do here.
	UNUSED(prepare); UNUSED(parent); UNUSED(child);
	return EINVAL;
}

static int pthread_setattr(pthread_attr_t *attr, int mask, int flags)
{
	if (!attr)
		return EINVAL;
	attr->flags = (attr->flags & ~mask) | (flags & mask);
	return 0;
}

static int pthread_getattr(const pthread_attr_t *attr, int mask, int *out)
{
	if (!attr || !out)
		return EINVAL;

	*out = attr->flags & mask;
	return 0;
}

int   pthread_attr_destroy(pthread_attr_t *attr)
{
	// Nothing to do here.
	UNUSED(attr);
	return 0;
}

int   pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
	return pthread_getattr(attr, PTHREAD_CREATE_JOINABLE, detachstate);
}

int   pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(guardsize);
	return EINVAL;
}

int   pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched)
{
	return pthread_getattr(attr, PTHREAD_INHERIT_SCHED, inheritsched);
}

int   pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(param);
	return EINVAL;
}

int   pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(policy);
	return EINVAL;
}

int   pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(contentionscope);
	return EINVAL;
}

int   pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(stackaddr); UNUSED(stacksize);
	return EINVAL;
}

int   pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr)
{
	// Not supported on Windows.
	UNUSED(attr); UNUSED(stackaddr);
	return EINVAL;
}

int   pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	if (!attr || !stacksize)
		return EINVAL;

	*stacksize = attr->stack;
	return 0;
}

int   pthread_attr_init(pthread_attr_t *attr)
{
	if (!attr)
		return EINVAL;
	attr->stack = 0;
	attr->prio = 0;
	attr->flags = PTHREAD_CREATE_JOINABLE | PTHREAD_CANCEL_ENABLE;
	return 0;
}

int   pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	return pthread_setattr(attr, PTHREAD_CREATE_JOINABLE, detachstate);
}

int   pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize)
{
	// Not supported on Windows, you have to take whatever guard the OS gives you.
	UNUSED(attr); UNUSED(guardsize);
	return EINVAL;
}

int   pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched)
{
	return pthread_setattr(attr, PTHREAD_INHERIT_SCHED, inheritsched);
}

int   pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
	if (!attr || !param)
		return EINVAL;

	attr->prio = param->sched_priority;
	return 0;
}

int   pthread_attr_setschedpolicy(pthread_attr_t *attr, int param)
{
	// Not supported, silently ignore.
	UNUSED(attr); UNUSED(param);
	return 0;
}

int   pthread_attr_setscope(pthread_attr_t *attr, int contentionscope)
{
	// Not supported, silently ignore.
	UNUSED(attr); UNUSED(contentionscope);
	return 0;
}

int   pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize)
{
	// User stacks are not supported on Windows.
	UNUSED(attr); UNUSED(stackaddr); UNUSED(stacksize);
	return EINVAL;
}

int   pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr)
{
	// User stacks are not supported on Windows.
	UNUSED(attr); UNUSED(stackaddr);
	return EINVAL;
}

int   pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	if (!attr)
		return EINVAL;
	attr->stack = stacksize;
	return 0;
}

int   pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	if (!barrier || !barrier->data)
		return EINVAL;

	CloseHandle(barrier->data->hSema);
	freemem(barrier->data);
	return 0;
}

int   pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned count)
{
	if (!barrier || count == 0)
		return EINVAL;

	UNUSED(attr); // we don't support any of the attributes.

	barrier->data = (struct pthread_barrierdata_t *)allocmem(sizeof(struct pthread_barrierdata_t));
	if (barrier->data == NULL)
		return EAGAIN;

	// Windows 8 has a built-in barrier primitive. But I want this to work on Vista, so
	// I'll use a semaphore and counter to create the equivalent.
	barrier->data->count = count;
	barrier->data->numWaiting = 0;
	barrier->data->hSema = CreateSemaphoreW(NULL, 0, count, NULL);

	if (barrier->data->hSema == NULL)
	{
		freemem(barrier->data);
		return EAGAIN;
	}

	return 0;
}

int   pthread_barrier_wait(pthread_barrier_t *barrier)
{
	DWORD err;
	LONG result;

	if (!barrier || !barrier->data)
		return EINVAL;

	// Indicate that we are waiting.
	result = InterlockedIncrement(&barrier->data->numWaiting);

	if ((unsigned)result == barrier->data->count)
	{
		// We are the last thread to reach the barrier.
		// Everyone is now at the barrier, release them all.
		InterlockedExchange(&barrier->data->numWaiting, 0);
		if (!ReleaseSemaphore(barrier->data->hSema, result-1, NULL))
			return EIO;
		return PTHREAD_BARRIER_SERIAL_THREAD;
	} else {
		// Not all threads are at the barrier, so wait.
		err = WaitForSingleObject(barrier->data->hSema, INFINITE);
		if (err != WAIT_OBJECT_0)
			return EIO;
		return 0;
	}
}

static int pthread_setbarrierattr(pthread_barrierattr_t *attr, int mask, int flags)
{
	if (!attr)
		return EINVAL;
	attr->flags = (attr->flags & ~mask) | (flags & mask);
	return 0;
}

static int pthread_getbarrierattr(const pthread_barrierattr_t *attr, int mask, int *out)
{
	if (!attr || !out)
		return EINVAL;

	*out = attr->flags & mask;
	return 0;
}

int   pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{
	// Nothing to do here.
	UNUSED(attr);
	return 0;
}

int   pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr, int *pshared)
{
	return pthread_getbarrierattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_barrierattr_init(pthread_barrierattr_t *attr)
{
	if (!attr)
		return EINVAL;
	attr->flags = 0;
	return 0;
}

int   pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared)
{
	return pthread_setbarrierattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

static void __stdcall pthread_apccancel(ULONG_PTR dwParam)
{
	UNUSED(dwParam);
	pthread_exit(NULL);
}

int   pthread_cancel(pthread_t thread)
{
	if (!thread || !(thread->flags & PTHREAD_CREATE_JOINABLE))
		return EINVAL;

	thread->flags |= PTHREAD_CANCELED;
	
	// We can't implement a full POSIX-compatible cancel, it's just
	// not possible on Windows. APIs like read, accept, sleep, simply
	// have no way of being interrupted.
	// The best we can do is to queue an APC, which will at least allow
	// interruption of any alertable Windows functions, like SleepEx, ReadFileEx, etc.
	// I'm not even going to try and implement asynchronous cancellation,
	// as at best it just doesn't work, and at worst is just honestly dangerous.
	if (QueueUserAPC(pthread_apccancel, thread->hThread, 0) == 0)
		return EIO;
	return 0;
}

void  pthread_cleanup_push(void (*routine)(void *), void *arg)
{
	pthread_t self = pthread_self();
	pthread_cancel_t *top = (pthread_cancel_t *)allocmem(sizeof(pthread_cancel_t));
	top->fn = routine;
	top->arg = arg;
	top->next = self->cleanup;
	self->cleanup = top;
}

void  pthread_cleanup_pop(int execute)
{
	pthread_t self = pthread_self();
	pthread_cancel_t *top = self->cleanup;
	if (top)
	{
		self->cleanup = top->next;
		if (execute)
		{
			top->fn(top->arg);
			freemem(top);
		}
	}
}

int   pthread_cond_broadcast(pthread_cond_t *cond)
{
	if (!cond)
		return EINVAL;

	// Create on-demand if needed.
	if (!cond->data)
	{
		int err = pthread_cond_ondemand(cond);
		if (err)
			return err;
	}

	WakeAllConditionVariable(&cond->data->cond);
	return 0;
}

int   pthread_cond_destroy(pthread_cond_t *cond)
{
	if (!cond || !cond->data)
		return EINVAL;

	freemem(cond->data);
	return 0;
}

int   pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	if (!cond)
		return EINVAL;

	UNUSED(attr); // we don't support any of the attributes

	cond->data = (struct pthread_conddata_t *)allocmem(sizeof(struct pthread_conddata_t));
	if (cond->data == NULL)
		return EAGAIN;

	InitializeConditionVariable(&cond->data->cond);
	return 0;
}

int   pthread_cond_signal(pthread_cond_t *cond)
{
	if (!cond)
		return EINVAL;

	// Create on-demand if needed.
	if (!cond->data)
	{
		int err = pthread_cond_ondemand(cond);
		if (err)
			return err;
	}

	WakeConditionVariable(&cond->data->cond);
	return 0;
}

int   pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	if (!cond || !mutex)
		return EINVAL;

	// Create on-demand if needed.
	if (!cond->data)
	{
		int err = pthread_cond_ondemand(cond);
		if (err)
			return err;
	}
	if (!mutex->data)
	{
		int err = pthread_mutex_ondemand(mutex);
		if (err)
			return err;
	}

	if (!SleepConditionVariableCS(&cond->data->cond, &mutex->data->crit, INFINITE))
		return EPERM;

	return 0;
}

static int pthread_setcondattr(pthread_condattr_t *attr, int mask, int flags)
{
	if (!attr)
		return EINVAL;
	attr->flags = (attr->flags & ~mask) | (flags & mask);
	return 0;
}

static int pthread_getcondattr(const pthread_condattr_t *attr, int mask, int *out)
{
	if (!attr || !out)
		return EINVAL;

	*out = attr->flags & mask;
	return 0;
}

int   pthread_condattr_destroy(pthread_condattr_t *attr)
{
	if (!attr)
		return EINVAL;
	// Nothing to do here.
	return 0;
}

int   pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared)
{
	return pthread_getcondattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_condattr_init(pthread_condattr_t *attr)
{
	if (!attr)
		return EINVAL;
	attr->flags = 0;
	return 0;
}

int   pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared)
{
	return pthread_setcondattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg)
{
	pthread_attr_t tmp;

	if (!thread || !start)
		return EINVAL;

	pthread_once(&thread_setup_once, thread_setup);
	if (thread_setup_tls == TLS_OUT_OF_INDEXES)
		return EAGAIN;

	*thread = (struct pthread_info_t *)allocmem(sizeof(struct pthread_info_t));
	if (!(*thread))
		return EAGAIN;

	if (!attr)
	{
		pthread_attr_init(&tmp);
		attr = &tmp;
	}

	(*thread)->arg = arg;
	(*thread)->start = start;
	(*thread)->result = NULL;
	(*thread)->flags = attr->flags;
	(*thread)->cleanup = NULL;

	if (attr->flags & PTHREAD_CREATE_JOINABLE)
		(*thread)->refcount = 1;
	else
		(*thread)->refcount = 0;

	(*thread)->hThread = (HANDLE)_beginthreadex(NULL, (unsigned)attr->stack, pthread_main, *thread, CREATE_SUSPENDED, NULL);
	if ((*thread)->hThread == 0)
	{
		freemem(*thread);
		return errno;
	}

	if (attr->flags & PTHREAD_INHERIT_SCHED)
	{
		SetThreadPriority((*thread)->hThread, GetThreadPriority(GetCurrentThread()));
	} else {
		SetThreadPriority((*thread)->hThread, attr->prio);
	}

	if (ResumeThread((*thread)->hThread) == -1)
		return EAGAIN;
	return 0;
}

int   pthread_detach(pthread_t thread)
{
	if (!thread || !(thread->flags & PTHREAD_CREATE_JOINABLE))
		return EINVAL;

	thread->flags &= ~PTHREAD_CREATE_JOINABLE;
	pthread_unref(thread);

	return 0;
}

int   pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1 == t2;
}

void  pthread_exit(void *value_ptr)
{
	pthread_docleanup(value_ptr);
	_endthreadex(0);
}

int   pthread_getconcurrency(void)
{
	return pthread_concurrency;
}

int   pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{
	// Not supported.
	UNUSED(thread); UNUSED(policy); UNUSED(param);
	return EINVAL;
}

void *pthread_getspecific(pthread_key_t key)
{
	return TlsGetValue(key);
}

int   pthread_join(pthread_t thread, void **value_ptr)
{
	DWORD i;
	if (!thread || !(thread->flags & PTHREAD_CREATE_JOINABLE))
		return EINVAL;

	i = WaitForSingleObjectEx(thread->hThread, INFINITE, TRUE);
	if (i != WAIT_OBJECT_0)
		return EPERM;

	if (value_ptr)
		*value_ptr = thread->result;

	return pthread_detach(thread);
}

int   pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	if (!key)
		return EINVAL;

	pthread_mutex_lock(&key_mutex);
	if (key_count >= PTHREAD_KEYS_MAX)
	{
		pthread_mutex_unlock(&key_mutex);
		return EAGAIN;
	}

	// Allocate a TLS slot for this key.
	*key = TlsAlloc();
	if (*key == TLS_OUT_OF_INDEXES)
	{
		pthread_mutex_unlock(&key_mutex);
		return EAGAIN;
	}

	// Make a note of the destructor.
	key_destructors[key_count].fn = destructor;
	key_destructors[key_count].slot = *key;
	key_count++;

	pthread_mutex_unlock(&key_mutex);
	return 0;
}

int   pthread_key_delete(pthread_key_t key)
{
	unsigned n = 0;
	pthread_mutex_lock(&key_mutex);

	// Find it in our table.
	while (n < key_count)
	{
		if (key_destructors[n].slot == key)
			break;
	}
	if (n >= key_count)
	{
		pthread_mutex_unlock(&key_mutex);
		return EINVAL;
	}

	// Free it up.
	TlsFree(key);
	key_destructors[n] = key_destructors[key_count-1];
	key_count--;

	pthread_mutex_unlock(&key_mutex);
	return 0;
}

int   pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (!mutex)
		return EINVAL;

	if (mutex->data)
	{
		DeleteCriticalSection(&mutex->data->crit);
		freemem(mutex->data);
	}
	return 0;
}

int   pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
	UNUSED(mutex); UNUSED(prioceiling);
	return 0;
}

int   pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	if (!mutex)
		return EINVAL;

	UNUSED(attr); // we don't support any of the attributes

	mutex->data = (struct pthread_mutexdata_t *)allocmem(sizeof(struct pthread_mutexdata_t));
	if (mutex->data == NULL)
		return EAGAIN;

	InitializeCriticalSection(&mutex->data->crit);
	return 0;
}

int   pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (!mutex)
		return EINVAL;

	// Create on-demand if needed.
	if (!mutex->data)
	{
		int err = pthread_mutex_ondemand(mutex);
		if (err)
			return err;
	}

	EnterCriticalSection(&mutex->data->crit);

	return 0;
}

int   pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
	// Not supported.
	UNUSED(mutex); UNUSED(prioceiling); UNUSED(old_ceiling);
	return 0;
}

int   pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if (!mutex)
		return EINVAL;

	// Create on-demand if needed.
	if (!mutex->data)
	{
		int err = pthread_mutex_ondemand(mutex);
		if (err)
			return err;
	}

	if (TryEnterCriticalSection(&mutex->data->crit))
		return 0;
	else
		return EBUSY;
}

int   pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (!mutex || !mutex->data)
		return EINVAL;

	LeaveCriticalSection(&mutex->data->crit);
	return 0;
}

static int pthread_setmutexattr(pthread_mutexattr_t *attr, int mask, int flags)
{
	if (!attr)
		return EINVAL;
	attr->flags = (attr->flags & ~mask) | (flags & mask);
	return 0;
}

static int pthread_getmutexattr(const pthread_mutexattr_t *attr, int mask, int *out)
{
	if (!attr || !out)
		return EINVAL;

	*out = attr->flags & mask;
	return 0;
}

int   pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	// Nothing to do here.
	UNUSED(attr);
	return 0;
}

int   pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
	// Not supported.
	UNUSED(attr); UNUSED(prioceiling);
	return EINVAL;
}

int   pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
	return pthread_getmutexattr(attr, PTHREAD_PRIO_INHERIT|PTHREAD_PRIO_PROTECT, protocol);
}

int   pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared)
{
	return pthread_getmutexattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	return pthread_getmutexattr(attr, PTHREAD_MUTEX_ERRORCHECK|PTHREAD_MUTEX_RECURSIVE, type);
}

int   pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	if (!attr)
		return EINVAL;
	attr->flags = PTHREAD_MUTEX_NORMAL;
	return 0;
}

int   pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
	// Not supported.
	UNUSED(attr); UNUSED(prioceiling);
	return 0;
}

int   pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
	return pthread_setmutexattr(attr, PTHREAD_PRIO_INHERIT|PTHREAD_PRIO_PROTECT, protocol);
}

int   pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
	return pthread_setmutexattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	return pthread_setmutexattr(attr, PTHREAD_MUTEX_ERRORCHECK|PTHREAD_MUTEX_RECURSIVE, type);
}

typedef struct {
	void (*init_routine)(void);
} oncehelper_t;

static BOOL __stdcall pthread_once_callback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
	oncehelper_t *help = (oncehelper_t *)Parameter;
	UNUSED(InitOnce); UNUSED(Context);
	help->init_routine();
	return TRUE;
}

cassert(sizeof(PINIT_ONCE) == sizeof(pthread_once_t));

int   pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	BOOL err;
	oncehelper_t help;
	help.init_routine = init_routine;

	if (!once_control)
		return EINVAL;

	err = InitOnceExecuteOnce((PINIT_ONCE)once_control, pthread_once_callback, &help, NULL);
	return err ? 0 : EAGAIN;
}

int   pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	if (!rwlock || !rwlock->data)
		return EINVAL;

	freemem(rwlock->data);
	return 0;
}

int   pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	if (!rwlock)
		return EINVAL;

	UNUSED(attr); // we don't support any of the attributes
	
	rwlock->data = (struct pthread_rwlockdata_t *)allocmem(sizeof(struct pthread_rwlockdata_t));
	if (rwlock->data == NULL)
		return EAGAIN;

	InitializeSRWLock(&rwlock->data->lock);
	rwlock->data->exclusive = 0;

	return 0;
}

int   pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	if (!rwlock || !rwlock->data)
		return EINVAL;

	AcquireSRWLockShared(&rwlock->data->lock);
	return 0;
}

int   pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	if (!rwlock || !rwlock->data)
		return EINVAL;

	if (rwlock->data->exclusive)
	{
		rwlock->data->exclusive = 0;
		ReleaseSRWLockExclusive(&rwlock->data->lock);
	} else {
		ReleaseSRWLockShared(&rwlock->data->lock);
	}

	return 0;
}

int   pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	if (!rwlock || !rwlock->data)
		return EINVAL;

	AcquireSRWLockExclusive(&rwlock->data->lock);

	// This is protected by the SRW lock.
	rwlock->data->exclusive = 1;
	return 0;
}

static int pthread_setrwlockattr(pthread_rwlockattr_t *attr, int mask, int flags)
{
	if (!attr)
		return EINVAL;
	attr->flags = (attr->flags & ~mask) | (flags & mask);
	return 0;
}

static int pthread_getrwlockattr(const pthread_rwlockattr_t *attr, int mask, int *out)
{
	if (!attr || !out)
		return EINVAL;

	*out = attr->flags & mask;
	return 0;
}

int   pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
	if (!attr)
		return EINVAL;
	// Nothing to do here.
	return 0;
}

int   pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr, int *pshared)
{
	return pthread_getrwlockattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

int   pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
	if (!attr)
		return EINVAL;
	attr->flags = 0;
	return 0;
}

int   pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared)
{
	return pthread_setrwlockattr(attr, PTHREAD_PROCESS_SHARED, pshared);
}

pthread_t pthread_self(void)
{
	pthread_t self;
	pthread_once(&thread_setup_once, thread_setup);

	self = (pthread_t)TlsGetValue(thread_setup_tls);

	if (!self)
	{
		// This is a non-pthread thread (e.g. someone used CreateThread)
		// The user shouldn't really be doing this, but let's make an effort
		// to support it anyway. Make a dummy structure for this new thread.
		//
		// This memory/handle probably gets leaked, not much we can do about that.
		self = (struct pthread_info_t *)allocmem(sizeof(struct pthread_info_t));
		if (self) {
			self->start = NULL;
			self->arg = NULL;
			self->result = NULL;
			self->refcount = 1;
			self->flags = PTHREAD_CREATE_JOINABLE;
			self->cleanup = NULL;

			DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &self->hThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
			TlsSetValue(thread_setup_tls, self);
		}
	}

	return self;
}

int   pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self = pthread_self();
	if (*oldstate)
		*oldstate = self->flags & PTHREAD_CANCEL_ENABLE;
	self->flags = (self->flags & ~PTHREAD_CANCEL_ENABLE) | (state & PTHREAD_CANCEL_ENABLE);
	return 0;
}

int   pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self = pthread_self();

	// We don't support async cancellations at all.
	if (type == PTHREAD_CANCEL_ASYNCHRONOUS)
		return EPERM;

	if (*oldtype)
		*oldtype = self->flags & PTHREAD_CANCEL_ASYNCHRONOUS;
	self->flags = (self->flags & ~PTHREAD_CANCEL_ASYNCHRONOUS) | (type & PTHREAD_CANCEL_ASYNCHRONOUS);
	return 0;
}

int   pthread_setconcurrency(int new_level)
{
	if (new_level < 0)
		return EINVAL;

	// We don't do anything with this, but make a note of it so we can return it.
	pthread_concurrency = new_level;
	return 0;
}

int   pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)
{
	if (!thread || !param)
		return EINVAL;

	UNUSED(policy); // we ignore the policy setting

	return SetThreadPriority(thread->hThread, param->sched_priority) ? EPERM : 0;
}

int   pthread_setschedprio(pthread_t thread, int prio)
{
	if (!thread)
		return EINVAL;

	return SetThreadPriority(thread->hThread, prio) ? EPERM : 0;
}

int   pthread_setspecific(pthread_key_t key, const void *value)
{
	return TlsSetValue(key, (LPVOID)value) ? 0 : EINVAL;
}

int   pthread_spin_destroy(pthread_spinlock_t *lock)
{
	if (!lock || !lock->data)
		return EINVAL;
	DeleteCriticalSection(&lock->data->crit);
	freemem(lock->data);
	return 0;
}

int   pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
	if (!lock)
		return EINVAL;

	UNUSED(pshared); // we don't supported this

	lock->data = (struct pthread_spinlockdata_t *)allocmem(sizeof(struct pthread_spinlockdata_t));
	if (lock->data == NULL)
		return EAGAIN;

	if (!InitializeCriticalSectionAndSpinCount(&lock->data->crit, 4000))
	{
		freemem(lock->data);
		return EAGAIN;
	}

	return 0;
}

int   pthread_spin_lock(pthread_spinlock_t *lock)
{
	if (!lock || !lock->data)
		return EINVAL;
	EnterCriticalSection(&lock->data->crit);
	return 0;
}

int   pthread_spin_trylock(pthread_spinlock_t *lock)
{
	if (!lock || !lock->data)
		return EINVAL;
	if (TryEnterCriticalSection(&lock->data->crit))
		return 0;
	return EBUSY;
}

int   pthread_spin_unlock(pthread_spinlock_t *lock)
{
	if (!lock || !lock->data)
		return EINVAL;
	LeaveCriticalSection(&lock->data->crit);
	return 0;
}

void  pthread_testcancel(void)
{
	pthread_t self = pthread_self();
	if (self->flags & PTHREAD_CANCELED)
		pthread_exit(NULL);
}
