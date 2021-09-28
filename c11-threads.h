/*
 * C11 Threads API
 *
 * Copyright (c) 2019-2021 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef C11_THREADS_H
#define C11_THREADS_H  1

#if __STDC_VERSION__ >= 201112L && !defined (__STDC_NO_THREADS__)

#include <threads.h>

#elif _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600

/*
 * Implementation via POSIX Threads
 *
 * CFLAGS += -pthread -D_XOPEN_SOURCE=600
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

#if __STDC_VERSION__ < 199901L
#define restrict
#endif

enum thrd_status {
	thrd_success,
	thrd_nomem,
	thrd_timedout,
	thrd_busy,
	thrd_error
};

typedef pthread_once_t	once_flag;
typedef pthread_cond_t	cnd_t;
typedef pthread_mutex_t	mtx_t;
typedef pthread_t	thrd_t;
typedef pthread_key_t	tss_t;

/* C11 7.25.2 Initialization functions */

#define ONCE_FLAG_INIT  PTHREAD_ONCE_INIT;

static inline void call_once (once_flag *o, void (*fn) (void))
{
	(void) pthread_once (o, fn);
}

/* C11 7.25.3 Condition variable functions */

static inline int cnd_broadcast (cnd_t *o)
{
	return pthread_cond_broadcast (o) == 0 ? thrd_success : thrd_error;
}

static inline void cnd_destroy (cnd_t *o)
{
	(void) pthread_cond_destroy (o);
}

static inline int cnd_init (cnd_t *o)
{
	int ret = pthread_cond_init (o, NULL);

	return ret == 0 ? thrd_success :
	       ret == ENOMEM ? thrd_nomem : thrd_error;
}

static inline int cnd_signal (cnd_t *o)
{
	return pthread_cond_signal (o) == 0 ? thrd_success : thrd_error;
}

static inline
int cnd_timedwait (cnd_t *restrict o, mtx_t *restrict m,
		   const struct timespec *restrict timeout)
{
	int ret = pthread_cond_timedwait (o, m, timeout);

	return ret == 0 ? thrd_success :
	       ret == ETIMEDOUT ? thrd_timedout : thrd_error;
}

static inline int cnd_wait (cnd_t *restrict o, mtx_t *restrict m)
{
	return pthread_cond_wait (o, m) == 0 ? thrd_success : thrd_error;
}

/* C11 7.25.4 Mutex functions */

enum mtx {
	mtx_plain,
	mtx_recursive,
	mtx_timed
};

static inline void mtx_destroy (mtx_t *o)
{
	pthread_mutex_destroy (o);
}

static inline int mtx_init (mtx_t *o, int type)
{
	pthread_mutexattr_t a;
	int ret;

	if (pthread_mutexattr_init (&a) != 0)
		return thrd_nomem;

	type = (type & mtx_recursive) != 0 ? PTHREAD_MUTEX_RECURSIVE :
					     PTHREAD_MUTEX_DEFAULT;
	pthread_mutexattr_settype (&a, type);

	ret = pthread_mutex_init (o, &a);
	pthread_mutexattr_destroy (&a);

	return ret == 0 ? thrd_success :
	       ret == ENOMEM ? thrd_nomem : thrd_error;
}

static inline int mtx_lock (mtx_t *o)
{
	return pthread_mutex_lock (o) == 0 ? thrd_success : thrd_error;
}

static inline
int mtx_timedlock (mtx_t *restrict o, const struct timespec *restrict timeout)
{
	int ret = pthread_mutex_timedlock (o, timeout);

	return ret == 0 ? thrd_success :
	       ret == ETIMEDOUT ? thrd_timedout : thrd_error;
}

static inline int mtx_trylock (mtx_t *o)
{
	int ret = pthread_mutex_trylock (o);

	return ret == 0 ? thrd_success :
	       ret == EBUSY ? thrd_busy : thrd_error;
}

static inline int mtx_unlock (mtx_t *o)
{
	return pthread_mutex_unlock (o) == 0 ? thrd_success : thrd_error;
}

/* C11 7.25.5 Threads functions */

typedef int (*thrd_start_t) (void *cookie);

static inline int thrd_create (thrd_t *o, thrd_start_t fn, void *cookie)
{
	int ret;

	ret = pthread_create (o, NULL, (void *(*) (void *)) fn, cookie);

	return ret == 0 ? thrd_success :
	       ret == EAGAIN ? thrd_nomem : thrd_error;
}

static inline thrd_t thrd_current (void)
{
	return pthread_self ();
}

static inline int thrd_detach (thrd_t o)
{
	return pthread_detach (o) == 0 ? thrd_success : thrd_error;
}

static inline int thrd_equal (thrd_t a, thrd_t b)
{
	return pthread_equal (a, b);
}

static inline void thrd_exit (int res)
{
	pthread_exit (NULL + res);
}

static inline int thrd_join (thrd_t o, int *res)
{
	return pthread_join (o, (void **) res) == 0 ? thrd_success :
						      thrd_error;
}

static inline
int thrd_sleep (const struct timespec *req, struct timespec *rem)
{
	return nanosleep (req, rem);
}

static inline void thrd_yield(void)
{
	(void) sched_yield ();
}

/* C11 Thread-specific storage functions */

typedef void (*tss_dtor_t) (void *);

static inline int tss_create (tss_t *key, tss_dtor_t free)
{
	return pthread_key_create (key, free) == 0 ? thrd_success : thrd_error;
}

static inline void tss_delete (tss_t key)
{
	(void) pthread_key_delete (key);
}

static inline void *tss_get (tss_t key)
{
	return pthread_getspecific (key);
}

static inline int tss_set (tss_t key, void *value)
{
	return pthread_setspecific (key, value) == 0 ? thrd_success :
						       thrd_error;
}

#else

#error "C11 threads does not supported"

#endif  /* no C11 threads */
#endif  /* C11_THREADS_H */
