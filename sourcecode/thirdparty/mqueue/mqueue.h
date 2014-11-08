/***************************************************************************
 *   Copyright (C) 2013 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _MQUEUE_H
#define _MQUEUE_H

#if !defined(HAVE_ANDROID_OS)
#include <stdint.h>
typedef uintptr_t mqd_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct X_mq_attr{
   size_t mq_msgsize;
   size_t mq_maxmsg;
};

#define mq_attr     X_mq_attr

int X_mq_close(
   mqd_t                 mq
);

int X_mq_getattr(
   mqd_t                mq,
   struct mq_attr      *attrbuf
);

mqd_t X_mq_open(
   const char           *mq_name,
   int                   oflags,
   mode_t                mode,
   struct mq_attr       *mq_attr
);

size_t X_mq_receive(
   mqd_t                 mq,
   char                 *msg_buffer,
   size_t                buflen,
   unsigned int         *msgprio
);

int X_mq_setattr(
   mqd_t                 mqdes,
   const struct mq_attr *new_attrs,
   struct mq_attr       *old_attrs
);

#define mq_send     X_mq_send
#define mq_unlink   X_mq_unlink
#define mq_close    X_mq_close
#define mq_getattr  X_mq_getattr
#define mq_open     X_mq_open
#define mq_receive  X_mq_receive
#define mq_setattr  X_mq_setattr
#define mq_send     X_mq_send
#define mq_unlink   X_mq_unlink

#if defined(__cplusplus)
}
#endif

#endif
