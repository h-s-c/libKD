/******************************************************************************
 * Copyright (c) 2014 Kevin Schmidt
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

#ifndef _MQUEUE_H
#define _MQUEUE_H

#include <signal.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mqd_t mqd_t;

struct mq_attr
{
   long    mq_flags;  
   long    mq_maxmsg; 
   long    mq_msgsize;
   long    mq_curmsgs;
};

int      mq_close(mqd_t);
int      mq_getattr(mqd_t, struct mq_attr *);
int      mq_notify(mqd_t, const struct sigevent *);
mqd_t    mq_open(const char *, int, ...);
ssize_t  mq_receive(mqd_t, char *, size_t, unsigned *);
int      mq_send(mqd_t, const char *, size_t, unsigned );
int      mq_setattr(mqd_t, const struct mq_attr *restrict, struct mq_attr *restrict);
ssize_t  mq_timedreceive(mqd_t, char *restrict, size_t, unsigned *restrict, const struct timespec *restrict);
int      mq_timedsend(mqd_t, const char *, size_t, unsigned , const struct timespec *);
int      mq_unlink(const char *);

#ifdef __cplusplus
}
#endif

#endif /* _MQUEUE_H_ */
