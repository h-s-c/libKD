/*    LIMITS OF LIABILITY AND DISCLAIMER OF WARRANTY
 * 
 * The author and publisher of the book "UNIX Network Programming" have
 * used their best efforts in preparing this software.  These efforts
 * include the development, research, and testing of the theories and
 * programs to determine their effectiveness.  The author and publisher
 * make no warranty of any kind, express or implied, with regard to
 * these programs or the documentation contained in the book. The author
 * and publisher shall not be liable in any event for incidental or
 * consequential damages in connection with, or arising out of, the
 * furnishing, performance, or use of these programs.
 */
 
#define _POSIX_C_SOURCE 200809L
#include "mqueue.h"

typedef struct mq_info    *mqd_t;

struct mq_attr 
{
  long  mq_flags;
  long  mq_maxmsg;
  long  mq_msgsize;
  long  mq_curmsgs;
};

struct mq_hdr 
{
  struct mq_attr    mqh_attr;
  long              mqh_head;
  long              mqh_free;
  long              mqh_nwait;
  pid_t             mqh_pid;
  struct sigevent   mqh_event;
  pthread_mutex_t   mqh_lock;
  pthread_cond_t    mqh_wait;
};

struct msg_hdr 
{
  long          msg_next;
  ssize_t       msg_len;
  unsigned int  msg_prio;
};

struct mq_info 
{
  struct mq_hdr*    mqi_hdr;
  long              mqi_magic;
  int               mqi_flags;
};

#define MQI_MAGIC   0x98765432
#define MSGSIZE(i)  ((((i) + sizeof(long)-1) / sizeof(long)) * sizeof(long))

int mq_close(mqd_t mqd)
{
    long    msgsize, filesize;
    struct mq_hdr *mqhdr;
    struct mq_attr    *attr;
    struct mq_info    *mqinfo;

    mqinfo = mqd;
    if (mqinfo->mqi_magic != MQI_MAGIC) {
        errno = EBADF;
        return(-1);
    }
    mqhdr = mqinfo->mqi_hdr;
    attr = &mqhdr->mqh_attr;

    if (mq_notify(mqd, NULL) != 0)    /* unregister calling process */
        return(-1);

    msgsize = MSGSIZE(attr->mq_msgsize);
    filesize = sizeof(struct mq_hdr) + (attr->mq_maxmsg *
               (sizeof(struct msg_hdr) + msgsize));
    if (munmap(mqinfo->mqi_hdr, filesize) == -1)
        return(-1);

    mqinfo->mqi_magic = 0;      /* just in case */
    free(mqinfo);
    return(0);
}

int mq_getattr(mqd_t mqd, struct mq_attr *mqstat)
{
  int   n;
  struct mq_hdr *mqhdr;
  struct mq_attr  *attr;
  struct mq_info  *mqinfo;

  mqinfo = mqd;
  if (mqinfo->mqi_magic != MQI_MAGIC) {
    errno = EBADF;
    return(-1);
  }
  mqhdr = mqinfo->mqi_hdr;
  attr = &mqhdr->mqh_attr;
  if ( (n = pthread_mutex_lock(&mqhdr->mqh_lock)) != 0) {
    errno = n;
    return(-1);
  }

  mqstat->mq_flags = mqinfo->mqi_flags; /* per-open */
  mqstat->mq_maxmsg = attr->mq_maxmsg;  /* remaining three per-queue */
  mqstat->mq_msgsize = attr->mq_msgsize;
  mqstat->mq_curmsgs = attr->mq_curmsgs;

  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(0);
}

int mq_notify(mqd_t mqd, const struct sigevent *notification)
{
  int   n;
  pid_t pid;
  struct mq_hdr *mqhdr;
  struct mq_info  *mqinfo;

  mqinfo = mqd;
  if (mqinfo->mqi_magic != MQI_MAGIC) {
    errno = EBADF;
    return(-1);
  }
  mqhdr = mqinfo->mqi_hdr;
  if ( (n = pthread_mutex_lock(&mqhdr->mqh_lock)) != 0) {
    errno = n;
    return(-1);
  }

  pid = getpid();
  if (notification == NULL) {
    if (mqhdr->mqh_pid == pid) {
      mqhdr->mqh_pid = 0; /* unregister calling process */
    }             /* no error if caller not registered */
  } else {
    if (mqhdr->mqh_pid != 0) {
      if (kill(mqhdr->mqh_pid, 0) != -1 || errno != ESRCH) {
        errno = EBUSY;
        goto err;
      }
    }
    mqhdr->mqh_pid = pid;
    mqhdr->mqh_event = *notification;
  }
  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(0);

err:
  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(-1);
}

#define   MAX_TRIES 10  /* for waiting for initialization */

struct mymq_attr  defattr = { 0, 128, 1024, 0 };

mqd_t mq_open(const char *pathname, int oflag, ...)
{
  int   i, fd, nonblock, created, save_errno;
  long  msgsize, filesize, index;
  va_list ap;
  mode_t  mode;
  int8_t  *mptr;
  struct stat statbuff;
  struct mq_hdr *mqhdr;
  struct msg_hdr  *msghdr;
  struct mq_attr  *attr;
  struct mq_info  *mqinfo;
  pthread_mutexattr_t mattr;
  pthread_condattr_t  cattr;

  created = 0;
  nonblock = oflag & O_NONBLOCK;
  oflag &= ~O_NONBLOCK;
  mptr = (int8_t *) MAP_FAILED;
  mqinfo = NULL;
again:
  if (oflag & O_CREAT) {
    va_start(ap, oflag);    /* init ap to final named argument */
    mode = va_arg(ap, va_mode_t) & ~S_IXUSR;
    attr = va_arg(ap, struct mymq_attr *);
    va_end(ap);

      /* 4open and specify O_EXCL and user-execute */
    fd = open(pathname, oflag | O_EXCL | O_RDWR, mode | S_IXUSR);
    if (fd < 0) {
      if (errno == EEXIST && (oflag & O_EXCL) == 0)
        goto exists;    /* already exists, OK */
      else
        return((mymqd_t) -1);
    }
    created = 1;
      /* 4first one to create the file initializes it */
    if (attr == NULL)
      attr = &defattr;
    else {
      if (attr->mq_maxmsg <= 0 || attr->mq_msgsize <= 0) {
        errno = EINVAL;
        goto err;
      }
    }
/* end mq_open1 */
/* include mq_open2 */
      /* 4calculate and set the file size */
    msgsize = MSGSIZE(attr->mq_msgsize);
    filesize = sizeof(struct mymq_hdr) + (attr->mq_maxmsg *
           (sizeof(struct mymsg_hdr) + msgsize));
    if (lseek(fd, filesize - 1, SEEK_SET) == -1)
      goto err;
    if (write(fd, "", 1) == -1)
      goto err;

      /* 4memory map the file */
    mptr = mmap(NULL, filesize, PROT_READ | PROT_WRITE,
          MAP_SHARED, fd, 0);
    if (mptr == MAP_FAILED)
      goto err;

      /* 4allocate one mymq_info{} for the queue */
/* *INDENT-OFF* */
    if ( (mqinfo = malloc(sizeof(struct mymq_info))) == NULL)
      goto err;
/* *INDENT-ON* */
    mqinfo->mqi_hdr = mqhdr = (struct mymq_hdr *) mptr;
    mqinfo->mqi_magic = MQI_MAGIC;
    mqinfo->mqi_flags = nonblock;

      /* 4initialize header at beginning of file */
      /* 4create free list with all messages on it */
    mqhdr->mqh_attr.mq_flags = 0;
    mqhdr->mqh_attr.mq_maxmsg = attr->mq_maxmsg;
    mqhdr->mqh_attr.mq_msgsize = attr->mq_msgsize;
    mqhdr->mqh_attr.mq_curmsgs = 0;
    mqhdr->mqh_nwait = 0;
    mqhdr->mqh_pid = 0;
    mqhdr->mqh_head = 0;
    index = sizeof(struct mymq_hdr);
    mqhdr->mqh_free = index;
    for (i = 0; i < attr->mq_maxmsg - 1; i++) {
      msghdr = (struct mymsg_hdr *) &mptr[index];
      index += sizeof(struct mymsg_hdr) + msgsize;
      msghdr->msg_next = index;
    }
    msghdr = (struct mymsg_hdr *) &mptr[index];
    msghdr->msg_next = 0;   /* end of free list */

      /* 4initialize mutex & condition variable */
    if ( (i = pthread_mutexattr_init(&mattr)) != 0)
      goto pthreaderr;
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    i = pthread_mutex_init(&mqhdr->mqh_lock, &mattr);
    pthread_mutexattr_destroy(&mattr);  /* be sure to destroy */
    if (i != 0)
      goto pthreaderr;

    if ( (i = pthread_condattr_init(&cattr)) != 0)
      goto pthreaderr;
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    i = pthread_cond_init(&mqhdr->mqh_wait, &cattr);
    pthread_condattr_destroy(&cattr); /* be sure to destroy */
    if (i != 0)
      goto pthreaderr;

      /* 4initialization complete, turn off user-execute bit */
    if (fchmod(fd, mode) == -1)
      goto err;
    close(fd);
    return((mymqd_t) mqinfo);
  }
/* end mq_open2 */
/* include mq_open3 */
exists:
    /* 4open the file then memory map */
  if ( (fd = open(pathname, O_RDWR)) < 0) {
    if (errno == ENOENT && (oflag & O_CREAT))
      goto again;
    goto err;
  }

    /* 4make certain initialization is complete */
  for (i = 0; i < MAX_TRIES; i++) {
    if (stat(pathname, &statbuff) == -1) {
      if (errno == ENOENT && (oflag & O_CREAT)) {
        close(fd);
        goto again;
      }
      goto err;
    }
    if ((statbuff.st_mode & S_IXUSR) == 0)
      break;
    sleep(1);
  }
  if (i == MAX_TRIES) {
    errno = ETIMEDOUT;
    goto err;
  }

  filesize = statbuff.st_size;
  mptr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mptr == MAP_FAILED)
    goto err;
  close(fd);

    /* 4allocate one mymq_info{} for each open */
/* *INDENT-OFF* */
  if ( (mqinfo = malloc(sizeof(struct mymq_info))) == NULL)
    goto err;
/* *INDENT-ON* */
  mqinfo->mqi_hdr = (struct mymq_hdr *) mptr;
  mqinfo->mqi_magic = MQI_MAGIC;
  mqinfo->mqi_flags = nonblock;
  return((mymqd_t) mqinfo);
/* $$.bp$$ */
pthreaderr:
  errno = i;
err:
    /* 4don't let following function calls change errno */
  save_errno = errno;
  if (created)
    unlink(pathname);
  if (mptr != MAP_FAILED)
    munmap(mptr, filesize);
  if (mqinfo != NULL)
    free(mqinfo);
  close(fd);
  errno = save_errno;
  return((mymqd_t) -1);
}

ssize_t mq_receive(mqd_t mqd, char *ptr, size_t maxlen, unsigned int *priop)
{
  int   n;
  long  index;
  int8_t  *mptr;
  ssize_t len;
  struct mq_hdr *mqhdr;
  struct mq_attr  *attr;
  struct msg_hdr  *msghdr;
  struct mq_info  *mqinfo;

  mqinfo = mqd;
  if (mqinfo->mqi_magic != MQI_MAGIC) {
    errno = EBADF;
    return(-1);
  }
  mqhdr = mqinfo->mqi_hdr;  /* struct pointer */
  mptr = (int8_t *) mqhdr;  /* byte pointer */
  attr = &mqhdr->mqh_attr;
  if ( (n = pthread_mutex_lock(&mqhdr->mqh_lock)) != 0) {
    errno = n;
    return(-1);
  }

  if (maxlen < attr->mq_msgsize) {
    errno = EMSGSIZE;
    goto err;
  }
  if (attr->mq_curmsgs == 0) {    /* queue is empty */
    if (mqinfo->mqi_flags & O_NONBLOCK) {
      errno = EAGAIN;
      goto err;
    }
      /* 4wait for a message to be placed onto queue */
    mqhdr->mqh_nwait++;
    while (attr->mq_curmsgs == 0)
      pthread_cond_wait(&mqhdr->mqh_wait, &mqhdr->mqh_lock);
    mqhdr->mqh_nwait--;
  }
/* end mq_receive1 */
/* include mq_receive2 */

  if ( (index = mqhdr->mqh_head) == 0)
    err_dump("mymq_receive: curmsgs = %ld; head = 0", attr->mq_curmsgs);

  msghdr = (struct mymsg_hdr *) &mptr[index];
  mqhdr->mqh_head = msghdr->msg_next; /* new head of list */
  len = msghdr->msg_len;
  memcpy(ptr, msghdr + 1, len);   /* copy the message itself */
  if (priop != NULL)
    *priop = msghdr->msg_prio;

    /* 4just-read message goes to front of free list */
  msghdr->msg_next = mqhdr->mqh_free;
  mqhdr->mqh_free = index;

    /* 4wake up anyone blocked in mq_send waiting for room */
  if (attr->mq_curmsgs == attr->mq_maxmsg)
    pthread_cond_signal(&mqhdr->mqh_wait);
  attr->mq_curmsgs--;

  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(len);

err:
  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(-1);
}

int mq_send(mymqd_t mqd, const char *ptr, size_t len, unsigned int prio)
{
  int   n;
  long  index, freeindex;
  int8_t  *mptr;
  struct sigevent *sigev;
  struct mq_hdr *mqhdr;
  struct mq_attr  *attr;
  struct msg_hdr  *msghdr, *nmsghdr, *pmsghdr;
  struct mq_info  *mqinfo;

  mqinfo = mqd;
  if (mqinfo->mqi_magic != MQI_MAGIC) {
    errno = EBADF;
    return(-1);
  }
  mqhdr = mqinfo->mqi_hdr;  /* struct pointer */
  mptr = (int8_t *) mqhdr;  /* byte pointer */
  attr = &mqhdr->mqh_attr;
  if ( (n = pthread_mutex_lock(&mqhdr->mqh_lock)) != 0) {
    errno = n;
    return(-1);
  }

  if (len > attr->mq_msgsize) {
    errno = EMSGSIZE;
    goto err;
  }
  if (attr->mq_curmsgs == 0) {
    if (mqhdr->mqh_pid != 0 && mqhdr->mqh_nwait == 0) {
      sigev = &mqhdr->mqh_event;
      if (sigev->sigev_notify == SIGEV_SIGNAL) {
        sigqueue(mqhdr->mqh_pid, sigev->sigev_signo,
             sigev->sigev_value);
      }
      mqhdr->mqh_pid = 0;   /* unregister */
    }
  } else if (attr->mq_curmsgs >= attr->mq_maxmsg) {
      /* 4queue is full */
    if (mqinfo->mqi_flags & O_NONBLOCK) {
      errno = EAGAIN;
      goto err;
    }
      /* 4wait for room for one message on the queue */
    while (attr->mq_curmsgs >= attr->mq_maxmsg)
      pthread_cond_wait(&mqhdr->mqh_wait, &mqhdr->mqh_lock);
  }
/* end mq_send1 */
/* include mq_send2 */
    /* 4nmsghdr will point to new message */
  if ( (freeindex = mqhdr->mqh_free) == 0)
    err_dump("mymq_send: curmsgs = %ld; free = 0", attr->mq_curmsgs);
  nmsghdr = (struct mymsg_hdr *) &mptr[freeindex];
  nmsghdr->msg_prio = prio;
  nmsghdr->msg_len = len;
  memcpy(nmsghdr + 1, ptr, len);    /* copy message from caller */
  mqhdr->mqh_free = nmsghdr->msg_next;  /* new freelist head */

    /* 4find right place for message in linked list */
  index = mqhdr->mqh_head;
  pmsghdr = (struct mymsg_hdr *) &(mqhdr->mqh_head);
  while (index != 0) {
    msghdr = (struct mymsg_hdr *) &mptr[index];
    if (prio > msghdr->msg_prio) {
      nmsghdr->msg_next = index;
      pmsghdr->msg_next = freeindex;
      break;
    }
    index = msghdr->msg_next;
    pmsghdr = msghdr;
  }
  if (index == 0) {
      /* 4queue was empty or new goes at end of list */
    pmsghdr->msg_next = freeindex;
    nmsghdr->msg_next = 0;
  }
    /* 4wake up anyone blocked in mq_receive waiting for a message */
  if (attr->mq_curmsgs == 0)
    pthread_cond_signal(&mqhdr->mqh_wait);
  attr->mq_curmsgs++;

  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(0);

err:
  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(-1);
}

int mq_setattr(mymqd_t mqd, const struct mymq_attr *mqstat, struct mymq_attr *omqstat)
{
  int   n;
  struct mq_hdr *mqhdr;
  struct mq_attr  *attr;
  struct mq_info  *mqinfo;

  mqinfo = mqd;
  if (mqinfo->mqi_magic != MQI_MAGIC) {
    errno = EBADF;
    return(-1);
  }
  mqhdr = mqinfo->mqi_hdr;
  attr = &mqhdr->mqh_attr;
  if ( (n = pthread_mutex_lock(&mqhdr->mqh_lock)) != 0) {
    errno = n;
    return(-1);
  }

  if (omqstat != NULL) {
    omqstat->mq_flags = mqinfo->mqi_flags;  /* previous attributes */
    omqstat->mq_maxmsg = attr->mq_maxmsg;
    omqstat->mq_msgsize = attr->mq_msgsize;
    omqstat->mq_curmsgs = attr->mq_curmsgs; /* and current status */
  }

  if (mqstat->mq_flags & O_NONBLOCK)
    mqinfo->mqi_flags |= O_NONBLOCK;
  else
    mqinfo->mqi_flags &= ~O_NONBLOCK;

  pthread_mutex_unlock(&mqhdr->mqh_lock);
  return(0);
}

int mq_unlink(const char *pathname)
{
  if (unlink(pathname) == -1)
    return(-1);
  return(0);
}