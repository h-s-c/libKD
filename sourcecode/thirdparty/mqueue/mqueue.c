/***************************************************************************
 *   Copyright (C) 2013 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert_np.h>
#include <pthread.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>

#if defined(_WIN32) &&  defined(_MSC_VER)
	#include <errno.h>
	#include <search.h>
	#define  PATH_MAX	 255
#elif defined(__CYGWIN32__) || defined(__CYGWIN__)
	#include <sys/errno.h>
#else  /* GLIBC */
	#include <errno.h>
#endif

#ifndef PATH_MAX
# define  PATH_MAX	 24
#endif

#define _MQ_NO_WARN_VAR(x) ((void)x)  /* silence warnings about unused
										 variables */

/*****************************************************************************
 * local definitions
 *****************************************************************************/
#define NUMBER_OF_QUEUES		100
#define NUMBER_OF_MESSAGES		100
#define NUMBER_OF_FILES			100
#define CREATE_ONLY				(NUMBER_OF_FILES + 1)

/* Calculates number of messages in queue */
#define NUMB_MESS(Q) ((Q->mBox.mIdxIn>=Q->mBox.mIdxOut)? \
	Q->mBox.mIdxIn - Q->mBox.mIdxOut: \
	Q->mq_attr.mq_maxmsg - Q->mBox.mIdxOut + Q->mBox.mIdxIn \
)

#define O_ISRDONLY( M ) \
	((M & O_RDONLY) == O_RDONLY)

#define O_ISWRONLY( M ) \
	((M & O_WRONLY) == O_WRONLY)

#define O_ISRDWR( M ) \
	((M & O_RDWR) == O_RDWR)

#define TNOW() (clock())

typedef clock_t				hr_time;

typedef struct{
	hr_time					timeStamp;
	unsigned int			prio;
	unsigned int			inOrder;
}OrderT;

typedef struct{
	char					*buffer; /* Buffer where message is stored */
	int						msgSz;  /* Length of message when sent */
	OrderT					order;  /* Based on time-stamp and mess prio */
}MessT;

typedef struct {
	size_t					mIdxIn; /*The index for writing */
	size_t					mIdxOut;/*The index for reading */
	MessT					*messArray;
	unsigned int			lastInOrder; /*together with time-stamp => unique */
}MBox;

typedef struct{
	int						taken;
	char					mq_name[PATH_MAX];
	struct mq_attr		  	mq_attr;
	MBox					mBox;
	sem_t					sem;
	/*Todo: mode (access mode) */
}QueueD;

typedef struct {	  /*File descriptor */
	int						taken;
	pthread_t				tId;	 /* Belongs to task */
	mode_t					oflags; /* Open for mode	*/
	int						qId;	 /* Attached queue  */
}FileD;

/*****************************************************************************
 * private data
 *****************************************************************************/
static QueueD	queuePool[ NUMBER_OF_QUEUES ];
static int		qPIdx = 0; /* Todo: Make this round-trip */

static FileD	filePool[ NUMBER_OF_FILES ];
static int		fPIdx = 0; /* Todo: Make this round-trip */

static sem_t	poolAccessSem;
static pthread_once_t mq_once= PTHREAD_ONCE_INIT;

/*****************************************************************************
 * private function declarations
 *****************************************************************************/
static void initialize( void );
static void sortByPrio( QueueD *Q );
/*Compare function, to be used by qsort */
static int compMess( const void *elem1, const void *elem2 );

/*****************************************************************************
 * public function implementations
 *****************************************************************************/

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_open.html
*/
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_close.html
*/
int mq_close(
	mqd_t					mq
){
	pthread_once(&mq_once, initialize);
	_MQ_NO_WARN_VAR(mq);
	assert_ext(sem_wait(&poolAccessSem) == 0);

	assert_ext(sem_post(&poolAccessSem) == 0);

	return 0;
}

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_getattr.html
*/
int mq_getattr(
	mqd_t					 mq,
	struct mq_attr			*attrbuf
){
	pthread_once(&mq_once, initialize);
	_MQ_NO_WARN_VAR(mq);
	_MQ_NO_WARN_VAR(attrbuf);
	assert_ext(sem_wait(&poolAccessSem) == 0);
	/*Find next empy slot in qPool*/

	assert_ext(sem_post(&poolAccessSem) == 0);

	return 0;
}

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_open.html
*/
mqd_t mq_open(
	const char			*mq_name,
	int					oflags,
	mode_t				mode,	/* Used for permissions. Is ignored ATM */
	struct mq_attr		*mq_attr
){
	int					i,j;
	size_t				k;
	int					qId;	/* queue ID in message pool */
	int					dId;	/* descriptor */

	pthread_once(&mq_once, initialize);
	_MQ_NO_WARN_VAR(mode);

	assert_ext(sem_wait(&poolAccessSem) == 0);

	/*First test some harmless common things */
	if (strlen(mq_name)>PATH_MAX ) { /* Name too long */
		errno = ENAMETOOLONG;
		assert_ext(sem_post(&poolAccessSem) == 0);
		return(-1);
	}

	if (mq_name[0] != '/' ) { /* Invalid name of queue */
		errno = EINVAL;
		assert_ext(sem_post(&poolAccessSem) == 0);
		return(-1);
	}

	if (oflags & O_CREAT) {	  /*Wishes to create new queue */
		int reuse = 0;

		/* Seek for new empty slot in queue pool */
		for (i=qPIdx,j=0; j<NUMBER_OF_QUEUES && queuePool[i].taken != 0; j++) {
			i++;
			i%=NUMBER_OF_QUEUES;
		}

		if (j==NUMBER_OF_QUEUES) { /* No empty slot found */
			errno = EMFILE;
			assert_ext(sem_post(&poolAccessSem) == 0);
			return(-1);
		}
		qId = j; /* slot identifier */
		queuePool[i].taken = 1;

		/* Scan the pool to see if duplicate name exists */
		for (i=0; i<NUMBER_OF_QUEUES; i++) {
			if (queuePool[i].taken){
				if (strncmp( queuePool[i].mq_name, mq_name, PATH_MAX) == 0){
					/* Duplicate name exists */
					if (oflags & O_EXCL) {
						/*Exclusive ?*/
						assert_ext(sem_post(&poolAccessSem) == 0);
						errno =  EEXIST;
						return(-1);
					} else {
						/* Attach to existing queue, don't create*/
						/* Silently reuse... */
						qId = i;
						reuse = 1;
					}
				}
			}
		}

		/* So far so good. Create the queue */
		if (!reuse) {
			queuePool[qId].taken = 1;
			strncpy( queuePool[qId].mq_name, mq_name, PATH_MAX  );

			/* Create default attributes (TBD) */
			/*
			Todo: ... queuePool[qId].mode->xx = xx

			Mean-while, assure it exists!
			*/
			if (mq_attr == NULL){
				errno = EINVAL;
				return(-1);
			}

			/*Todo: doesn't work.. why? */
			/*memcpy( &queuePool[qId].mq_attr, mq_attr, sizeof(struct mq_attr));*/
			/*strange...*/
			queuePool[qId].mq_attr.mq_maxmsg = mq_attr->mq_maxmsg;
			queuePool[qId].mq_attr.mq_msgsize = mq_attr->mq_msgsize;

			/* Create the message array */
			queuePool[qId].mBox.messArray =
			(MessT*)calloc(mq_attr->mq_maxmsg, sizeof(MessT));
			/* Create every message buffer and put it in array */
			if (queuePool[qId].mBox.messArray == NULL) {
				/*No more memory left on heap */
				assert_ext(sem_post(&poolAccessSem) == 0);
				errno =  ENFILE;
				return(-1);
			}

			for (k=0; k<mq_attr->mq_maxmsg; k++){
				queuePool[qId].mBox.messArray[k].buffer =
				(char*)calloc(1,mq_attr->mq_msgsize);
				if (queuePool[qId].mBox.messArray[k].buffer == NULL){

					/*No more memory left on heap */
					/*Tidy up..*/
					for (j=k; j>=0; j--){
						free(queuePool[qId].mBox.messArray[j].buffer);
					}
					free(queuePool[qId].mBox.messArray);

					assert_ext(sem_post(&poolAccessSem) == 0);
					errno =  ENFILE;
					return(-1);
				}

				queuePool[qId].mBox.messArray[k].msgSz = 0;
				queuePool[qId].mBox.messArray[k].order.inOrder = 0;
				queuePool[qId].mBox.messArray[k].order.prio	 = 0; /*Tada...!*/
				queuePool[qId].mBox.messArray[k].order.timeStamp = 0;
				queuePool[qId].mBox.lastInOrder = 0;
			}

			queuePool[qId].mBox.mIdxIn = 0; /*No pending messages */
			queuePool[qId].mBox.mIdxOut = 0;

			/* initialize the queues semaphore */
			if (!(oflags & O_NONBLOCK))
				assert_ext(sem_init (&queuePool[qId].sem, 0, 0) == 0);
		}
	}

	/* Attach queue handle to file handle if flags indicate also usage */
	if ( O_ISRDONLY(oflags) || O_ISWRONLY(oflags) || O_ISRDWR(oflags) ) {
		if (!(oflags & O_CREAT)) { /* Hasn't been created this time. Find it! */
			for (i=0; i<NUMBER_OF_QUEUES; i++) {
				if (queuePool[i].taken)
					if (strncmp( queuePool[i].mq_name, mq_name, PATH_MAX) == 0)
						break;
			}
			if ( i == NUMBER_OF_QUEUES ) {/* Not found ... */
				assert_ext(sem_post(&poolAccessSem) == 0);
				errno =  ENOENT;
				return(-1);
			}
			qId = i;
		}/* else: The qId is valid already */


		/* Attach the pool handle to the file descriptor */
		/* Seek for new empty slot in file pool */
		for (i=fPIdx,j=0; j<NUMBER_OF_FILES && filePool[i].taken != 0; j++) {
			i++;
			i%=NUMBER_OF_FILES;
		}

		if (j==NUMBER_OF_FILES) { /* No empty slot found */
			errno = EMFILE;
			assert_ext(sem_post(&poolAccessSem) == 0);
			return(-1);
		}
		dId = j; /* slot identifier */
		filePool[i].taken = 1;
		filePool[i].qId = qId;
		filePool[i].oflags = oflags;
		filePool[i].tId = pthread_self();

	} else {
		dId =  CREATE_ONLY;  /* Not valid file handle, Created only */
	}

	assert_ext(sem_post( &poolAccessSem) == 0);
	return(dId);
}

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_receive.html
*/
size_t mq_receive(
	mqd_t				mq,
	char				*msg_buffer,
	size_t				buflen,
	unsigned int		*msgprio
){
	QueueD	*Q;
	size_t	msgSize;

	pthread_once(&mq_once, initialize);
	assert_ext(sem_wait(&poolAccessSem) == 0);

	/*Is valid file-handle?*/

	if (!(
		((mq>=0) && (mq<NUMBER_OF_FILES)) &&
		( filePool[mq].taken == 1 ) &&
		( filePool[mq].tId == pthread_self())
	)){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}

	if (!( O_ISRDONLY(filePool[mq].oflags) ||
		  O_ISRDWR(filePool[mq].oflags)))
	{
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}

	Q = &queuePool[filePool[mq].qId];

	if (Q->taken != 1){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}

	/* Check if message fits */
	if (!(buflen >= Q->mq_attr.mq_msgsize)){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno = EMSGSIZE;
		return(-1);
	}

	if ( NUMB_MESS(Q) <= 0 ) {
		if (filePool[mq].oflags & O_NONBLOCK){
			assert_ext(sem_post(&poolAccessSem) == 0);
			errno =  EAGAIN;
			return(-1);
		}
	}

	/* OK so far */
	/* Release pool-access */
	assert_ext(sem_post(&poolAccessSem) == 0);

	/* Will block here if necessary waiting for messages, i.e.
	 * not just short disruptions protecting integrity */
	assert_ext(sem_wait(&Q->sem) == 0);

	assert_ext(sem_wait(&poolAccessSem) == 0);

	memcpy(
		msg_buffer,
		Q->mBox.messArray[Q->mBox.mIdxOut].buffer,
		Q->mBox.messArray[Q->mBox.mIdxOut].msgSz
	);

	/* Special case if attribute is NULL. Check what standard says about
	 * that */
	if (msgprio) {
		*msgprio = Q->mBox.messArray[Q->mBox.mIdxIn].order.prio;
	}

	msgSize = Q->mBox.messArray[Q->mBox.mIdxOut].msgSz;

	Q->mBox.mIdxOut++;
	Q->mBox.mIdxOut %= Q->mq_attr.mq_maxmsg;
	assert_ext(sem_post(&poolAccessSem) == 0);

	return(msgSize);
}

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_setattr.html
*/
int mq_setattr(
	mqd_t					mqdes,
	const struct mq_attr	*new_attrs,
	struct mq_attr			*old_attrs
){
	pthread_once(&mq_once, initialize);
	_MQ_NO_WARN_VAR(mqdes);
	_MQ_NO_WARN_VAR(new_attrs);
	_MQ_NO_WARN_VAR(old_attrs);
	assert_ext(sem_wait(&poolAccessSem) == 0);

	assert_ext(sem_post(&poolAccessSem) == 0);

	return 0;
}

//------1---------2---------3---------4---------5---------6---------7---------8
/*!
@see http://www.opengroup.org/onlinepubs/009695399/functions/mq_send.html
*/
int mq_send(
	mqd_t				mq,
	const char			*msg,
	size_t				msglen,
	unsigned int		msgprio
){
	QueueD	*Q;
	time_t	ttime;

	pthread_once(&mq_once, initialize);
	assert_ext(sem_wait(&poolAccessSem) == 0);

	/*Is valid file-handle?*/

	if (!(
		((mq>=0) && (mq<NUMBER_OF_FILES)) &&
		( filePool[mq].taken == 1 ) &&
		( filePool[mq].tId == pthread_self())
	)){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}

	if (!( O_ISWRONLY(filePool[mq].oflags) ||
		  O_ISRDWR(filePool[mq].oflags)))
	{
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}


	Q = &queuePool[filePool[mq].qId];

	if (Q->taken != 1){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EBADF;
		return(-1);
	}

	/* Check if message fits */
	if (!(Q->mq_attr.mq_msgsize >= msglen)){
		assert_ext(sem_post(&poolAccessSem) == 0);
		errno =  EMSGSIZE;
		return(-1);
	}

	if (!(NUMB_MESS(Q) < Q->mq_attr.mq_maxmsg-1)){ /* queue is full Save 1, else
													 sort fucks up*/
		/* Todo: never bocks (yet) */
		if (filePool[mq].oflags & O_NONBLOCK){		/* to block or not to block */
			assert_ext(sem_post(&poolAccessSem) == 0);
			errno =  EAGAIN;
			return(-1);
		} else {
			assert_ext(sem_post(&poolAccessSem) == 0);
			errno =  EAGAIN; /* TODO: block on send full */
			return(-1);
		}
	}

	/*OK so far*/

	memcpy( Q->mBox.messArray[Q->mBox.mIdxIn].buffer, msg, msglen);

	Q->mBox.messArray[Q->mBox.mIdxIn].msgSz = msglen;

	Q->mBox.messArray[Q->mBox.mIdxIn].order.prio = msgprio;
	Q->mBox.messArray[Q->mBox.mIdxIn].order.inOrder =
		Q->mBox.lastInOrder++;
	Q->mBox.messArray[Q->mBox.mIdxIn].order.timeStamp = TNOW();

	Q->mBox.mIdxIn++;
	Q->mBox.mIdxIn %= Q->mq_attr.mq_maxmsg;

	sortByPrio( Q );

	assert_ext(sem_post(&Q->sem) == 0);

	assert_ext(sem_post(&poolAccessSem) == 0);

	return(0);
}

/*****************************************************************************
 * private function implementations
 *****************************************************************************/

/*****************************************************************************
 * FUNCTION NAME:
 *
 *
 * DESCRIPTION:
 *
 *
 * NOTES:
 *
 *****************************************************************************/
static void initialize( void ) {
	int i;

	/* Make atomic */
	assert_ext(sem_init (&poolAccessSem, 0, 1) == 0);
	/* Make atomic */

	assert_ext(sem_wait(&poolAccessSem) == 0);
	for (i=0; i<NUMBER_OF_QUEUES; i++) {
		queuePool[i].taken = 0;
			memset(queuePool[i].mq_name,'Q',PATH_MAX); /* Makes a mark in memory.
														 Easy to find the pool*/
			queuePool[i].mq_name[0]=0; /* Also invalidate it for any string
										 compartments just in case */
	}
	for (i=0; i<NUMBER_OF_FILES; i++) {
		filePool[i].taken = 0;
	}
	assert_ext(sem_post(&poolAccessSem) == 0);
}

/*****************************************************************************
 * FUNCTION NAME: compMess
 *
 *
 * DESCRIPTION: Compare function for quick sort. Sorts by prio, timestamp,
 *              incoming order.
 *
 *
 * NOTES:
 *
 *****************************************************************************/
static int compMess(
	const void *elem1,
	const void *elem2
){
	MessT				*e1 = (MessT*)elem1;
	MessT				*e2 = (MessT*)elem2;

	/* Per prio */
	if (e1->order.prio > e2->order.prio)
		return(-1);
	if (e1->order.prio < e2->order.prio)
		return(1);
	/* Per timestamp (resolution is 1 sec) */
	if (e1->order.timeStamp < e2->order.timeStamp)
		return(-1);
	if (e1->order.timeStamp > e2->order.timeStamp)
		return(1);
	/* Per order of sending */
	if (e1->order.inOrder < e2->order.inOrder)
		return(-1);
	if (e1->order.inOrder > e2->order.inOrder)
		return(1);
	return(0); /*Should never happen*/
}

//------1---------2---------3---------4---------5---------6---------7---------8
static void sortByPrio( QueueD *Q ){
	int szMess = NUMB_MESS(Q);
	int i,j;

	if (Q->mBox.mIdxOut <= Q->mBox.mIdxIn){
		qsort(
			&Q->mBox.messArray[Q->mBox.mIdxOut],
			szMess,
			sizeof(MessT),
			compMess
		);
	} else {
		MessT *tempT;

		tempT = (MessT*)calloc(szMess, sizeof(MessT));
		/* Copy the content from queue into temp storage */
		i = Q->mBox.mIdxOut;
		/*i --;
		i %= Q->mq_attr.mq_maxmsg;*/
		for (j=0; j<szMess; j++){
			tempT[j] = Q->mBox.messArray[i];
			i++;
			i %= Q->mq_attr.mq_maxmsg;
		}

		qsort(
			tempT,
			szMess,
			sizeof(MessT),
			compMess
		);

		/* Copy back */
		i = Q->mBox.mIdxOut;
		/*i --;
		i %= Q->mq_attr.mq_maxmsg;*/
		for (j=0; j<szMess; j++){
			Q->mBox.messArray[i] = tempT[j];
			i++;
			i %= Q->mq_attr.mq_maxmsg;
		}

		free(tempT);
	}
}

/*!
Just a stub to get test-code through compiler. Should be easy enough to
implement though
*/
int mq_unlink(
	const char *mq_name
){
	_MQ_NO_WARN_VAR(mq_name);
	return 0;
}


/** @defgroup POSIX_RT POSIX_RT: POSIX 1003.1b API
@ingroup PTHREAD
@brief RT queues and semaphores - POSIX 1003.1b API

<em>*Documentation and implementation in progress*</em>

Good references about the API:
@see http://www.opengroup.org/onlinepubs/009695399/idx/im.html (look for functions starting with mq_)
@see http://www.opengroup.org/onlinepubs/009695399/idx/is.html (look for functions starting with sem_)

<p><b>Go gack to</b> \ref COMPONENTS</p>

*/
