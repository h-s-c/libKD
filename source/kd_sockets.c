// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2018 Kevin Schmidt
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

/******************************************************************************
 * KD includes
 ******************************************************************************/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#define _GNU_SOURCE /* O_CLOEXEC */
#endif
#include "kdplatform.h"        // for KD_API, KD_APIENTRY, KD_UNUSED, KDuint32
#include <KD/kd.h>             // for KDint, KDSocket, kdSetError, KDSockaddr
#include <KD/KHR_formatted.h>  // for kdSnprintfKHR
#include <KD/kdext.h>          // for kdSetErrorPlatformVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/******************************************************************************
 * C includes
 ******************************************************************************/

#if !defined(_WIN32) && !defined(KD_FREESTANDING)
#include <errno.h>  // for errno
#endif

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
// IWYU pragma: no_include <linux/in.h>
#include <unistd.h>
#include <netdb.h>       // for addrinfo, freeaddrinfo, getaddrinfo
#include <netinet/in.h>  // for sockaddr_in, in_addr, INADDR_ANY, htonl
#include <sys/socket.h>  // for socket, AF_INET, bind, connect, recv
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h> /* WSA.. */
#include <ws2tcpip.h>
#undef s_addr /* OpenKODE uses this */
#endif

/******************************************************************************
 * Network sockets
 ******************************************************************************/

/* kdNameLookup: Look up a hostname. */
typedef struct {
    const KDchar *hostname;
    void *eventuserptr;
    KDThread *destination;
} _KDNameLookupPayload;
static void *__kdNameLookupHandler(void *arg)
{
    /* TODO: Make async, threadsafe and cancelable */
    _KDNameLookupPayload *payload = (_KDNameLookupPayload *)arg;

    static KDEventNameLookup lookupevent;
    kdMemset(&lookupevent, 0, sizeof(lookupevent));

    static KDSockaddr addr;
    kdMemset(&addr, 0, sizeof(addr));
    addr.family = KD_AF_INET;

    struct addrinfo *result = KD_NULL;
    struct addrinfo hints;
    kdMemset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    KDint retval = getaddrinfo(payload->hostname, 0, &hints, &result);
    if(retval != 0)
    {
        lookupevent.error = KD_EHOST_NOT_FOUND;
    }
    else
    {
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
        struct sockaddr_in sockin;
        kdMemcpy(&sockin, result->ai_addr, (sizeof(struct sockaddr)));
        addr.data.sin.address = sockin.sin_addr.s_addr;
#if defined(_WIN32)
#undef s_addr
#endif
        lookupevent.resultlen = 1;
        lookupevent.result = &addr;
        freeaddrinfo(result);
    }

    /* Post event to the original thread */
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_NAME_LOOKUP_COMPLETE;
    event->userptr = payload->eventuserptr;
    event->data.namelookup = lookupevent;
    kdPostThreadEvent(event, payload->destination);

    return 0;
}
KD_API KDint KD_APIENTRY kdNameLookup(KDint af, const KDchar *hostname, void *eventuserptr)
{
    if(af != KD_AF_INET)
    {
        kdSetError(KD_EINVAL);
        return -1;
    }

    static _KDNameLookupPayload payload;
    kdMemset(&payload, 0, sizeof(payload));
    payload.hostname = hostname;
    payload.eventuserptr = eventuserptr;
    payload.destination = kdThreadSelf();

    KDThread *thread = kdThreadCreate(KD_NULL, __kdNameLookupHandler, &payload);
    if(thread == KD_NULL)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            kdLogMessage("kdNameLookup() needs a threading implementation.");
            return -1;
        }
        kdSetError(KD_ENOMEM);
        return -1;
    }
    kdThreadDetach(thread);

    return 0;
}

/* kdNameLookupCancel: Selectively cancels ongoing kdNameLookup operations. */
KD_API void KD_APIENTRY kdNameLookupCancel(KD_UNUSED void *eventuserptr)
{
}

/* kdSocketCreate: Creates a socket. */
struct KDSocket {
#if defined(_WIN32)
    SOCKET nativesocket;
#else
    KDint nativesocket;
#endif
    KDint type;
    const struct KDSockaddr *addr;
    void *userptr;
};
KD_API KDSocket *KD_APIENTRY kdSocketCreate(KDint type, void *eventuserptr)
{
    KDSocket *sock = (KDSocket *)kdMalloc(sizeof(KDSocket));
    if(sock == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    sock->type = type;
    sock->addr = KD_NULL;
    sock->userptr = eventuserptr;
    if(sock->type == KD_SOCK_TCP)
    {
#if defined(_WIN32)
        sock->nativesocket = WSASocketA(AF_UNIX, SOCK_STREAM, 0, 0, 0, 0);
        if(sock->nativesocket == INVALID_SOCKET)
        {
            KDint error = WSAGetLastError();
#else
        sock->nativesocket = socket(AF_UNIX, SOCK_STREAM, 0);
        if(sock->nativesocket == -1)
        {
            KDint error = errno;
#endif
            kdFree(sock);
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EMFILE | KD_ENOMEM | KD_ENOSYS);
            return KD_NULL;
        }
    }
    else if(sock->type == KD_SOCK_UDP)
    {
#if defined(_WIN32)
        sock->nativesocket = WSASocketA(AF_UNIX, SOCK_DGRAM, 0, 0, 0, 0);
        if(sock->nativesocket == INVALID_SOCKET)
        {
            KDint error = WSAGetLastError();
#else
        sock->nativesocket = socket(AF_UNIX, SOCK_DGRAM, 0);
        if(sock->nativesocket == -1)
        {
            KDint error = errno;
#endif
            kdFree(sock);
            kdSetErrorPlatformVEN(error, KD_EACCES | KD_EINVAL | KD_EIO | KD_EMFILE | KD_ENOMEM | KD_ENOSYS);
            return KD_NULL;
        }
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_SOCKET_READABLE;
        event->userptr = sock->userptr;
        event->data.socketreadable.socket = sock;
        kdPostEvent(event);
    }
    else
    {
        kdFree(sock);
        kdSetError(KD_EINVAL);
        return KD_NULL;
    }
    return sock;
}

/* kdSocketClose: Closes a socket. */
KD_API KDint KD_APIENTRY kdSocketClose(KDSocket *socket)
{
#if defined(_WIN32)
    closesocket(socket->nativesocket);
#else
    close(socket->nativesocket);
#endif
    return 0;
}

/* kdSocketBind: Bind a socket. */
KD_API KDint KD_APIENTRY kdSocketBind(KDSocket *socket, const KDSockaddr *addr, KD_UNUSED KDboolean reuse)
{
    if(addr->family != KD_AF_INET)
    {
        kdSetError(KD_EAFNOSUPPORT);
        return -1;
    }

    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == KD_INADDR_ANY)
    {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
    KDint retval = bind(socket->nativesocket, (struct sockaddr *)&address, sizeof(address));
#if defined(_WIN32)
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EADDRINUSE | KD_EADDRNOTAVAIL | KD_EAFNOSUPPORT | KD_EINVAL | KD_EIO | KD_EISCONN | KD_ENOMEM);
        return -1;
    }

    socket->addr = addr;
    if(socket->type == KD_SOCK_TCP)
    {
        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_SOCKET_READABLE;
        event->userptr = socket->userptr;
        event->data.socketreadable.socket = socket;
        kdPostEvent(event);
    }
    return 0;
}

/* kdSocketGetName: Get the local address of a socket. */
KD_API KDint KD_APIENTRY kdSocketGetName(KDSocket *socket, KDSockaddr *addr)
{
    kdMemcpy(&addr, &socket->addr, sizeof(KDSockaddr));
    return 0;
}

/* kdSocketConnect: Connects a socket. */
KD_API KDint KD_APIENTRY kdSocketConnect(KDSocket *socket, const KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == KD_INADDR_ANY)
    {
        address.sin_addr.s_addr = kdHtonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
#if defined(_WIN32)
    KDint retval = WSAConnect(socket->nativesocket, (struct sockaddr *)&address, sizeof(address), 0, 0, 0, 0);
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    KDint retval = connect(socket->nativesocket, (struct sockaddr *)&address, sizeof(address));
    if(retval == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EADDRINUSE | KD_EAFNOSUPPORT | KD_EALREADY | KD_ECONNREFUSED | KD_ECONNRESET | KD_EHOSTUNREACH | KD_EINVAL | KD_EIO | KD_EISCONN | KD_ENOMEM | KD_ETIMEDOUT);
        return -1;
    }
    return 0;
}

/* kdSocketListen: Listen on a socket. */
KD_API KDint KD_APIENTRY kdSocketListen(KD_UNUSED KDSocket *socket, KD_UNUSED KDint backlog)
{
    kdSetError(KD_ENOSYS);
    return -1;
}

/* kdSocketAccept: Accept an incoming connection. */
KD_API KDSocket *KD_APIENTRY kdSocketAccept(KD_UNUSED KDSocket *socket, KD_UNUSED KDSockaddr *addr, KD_UNUSED void *eventuserptr)
{
    kdSetError(KD_EINVAL);
    return KD_NULL;
}

/* kdSocketSend, kdSocketSendTo: Send data to a socket. */
KD_API KDint KD_APIENTRY kdSocketSend(KDSocket *socket, const void *buf, KDint len)
{
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = kdMalloc(len);
    kdMemcpy(wsabuf.buf, buf, len);
    KDint retval = WSASend(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, 0, KD_NULL, KD_NULL);
    kdFree(wsabuf.buf);
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    result = (KDint)send(socket->nativesocket, buf, (KDsize)len, 0);
    if(result == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAFNOSUPPORT | KD_EAGAIN | KD_ECONNRESET | KD_EDESTADDRREQ | KD_EIO | KD_ENOMEM | KD_ENOTCONN);
        return -1;
    }
    return result;
}

KD_API KDint KD_APIENTRY kdSocketSendTo(KDSocket *socket, const void *buf, KDint len, const KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(addr->data.sin.address == kdHtonl(KD_INADDR_ANY))
    {
        address.sin_addr.s_addr = kdHtonl(INADDR_ANY);
    }
    else
    {
        address.sin_addr.s_addr = kdHtonl(addr->data.sin.address);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    address.sin_port = kdHtons(addr->data.sin.port);
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = kdMalloc(len);
    kdMemcpy(wsabuf.buf, buf, len);
    KDint retval = WSASendTo(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, 0, (const struct sockaddr *)&address, sizeof(address), KD_NULL, KD_NULL);
    kdFree(wsabuf.buf);
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    result = (KDint)sendto(socket->nativesocket, buf, (KDsize)len, 0, (struct sockaddr *)&address, sizeof(address));
    if(result == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAFNOSUPPORT | KD_EAGAIN | KD_ECONNRESET | KD_EDESTADDRREQ | KD_EIO | KD_ENOMEM | KD_ENOTCONN);
        return -1;
    }
    return result;
}

/* kdSocketRecv, kdSocketRecvFrom: Receive data from a socket. */
KD_API KDint KD_APIENTRY kdSocketRecv(KDSocket *socket, void *buf, KDint len)
{
    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = buf;
    KDint retval = WSARecv(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, (DWORD[]) {0}, KD_NULL, KD_NULL);
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    result = (KDint)recv(socket->nativesocket, buf, (KDsize)len, 0);
    if(result == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAGAIN | KD_ECONNRESET | KD_EIO | KD_ENOMEM | KD_ENOTCONN | KD_ETIMEDOUT);
        return -1;
    }
    return result;
    ;
}

KD_API KDint KD_APIENTRY kdSocketRecvFrom(KDSocket *socket, void *buf, KDint len, KDSockaddr *addr)
{
    struct sockaddr_in address;
    kdMemset(&address, 0, sizeof(address));
    socklen_t addresssize = sizeof(address);

    KDint result = 0;
#if defined(_WIN32)
    WSABUF wsabuf;
    wsabuf.len = len;
    wsabuf.buf = buf;
    KDint retval = WSARecvFrom(socket->nativesocket, &wsabuf, 1, (DWORD *)&result, (DWORD[]) {0}, (struct sockaddr *)&address, &addresssize, KD_NULL, KD_NULL);
    if(retval == SOCKET_ERROR)
    {
        KDint error = WSAGetLastError();
#else
    result = (KDint)recvfrom(socket->nativesocket, buf, (KDsize)len, 0, (struct sockaddr *)&address, &addresssize);
    if(result == -1)
    {
        KDint error = errno;
#endif
        kdSetErrorPlatformVEN(error, KD_EAGAIN | KD_ECONNRESET | KD_EIO | KD_ENOMEM | KD_ENOTCONN | KD_ETIMEDOUT);
        return -1;
    }

    addr->family = KD_AF_INET;
#if defined(_WIN32)
#define s_addr S_un.S_addr
#endif
    if(address.sin_addr.s_addr == kdHtonl(INADDR_ANY))
    {
        addr->data.sin.address = kdHtonl(KD_INADDR_ANY);
    }
    else
    {
        addr->data.sin.address = kdHtonl(address.sin_addr.s_addr);
    }
#if defined(_WIN32)
#undef s_addr
#endif
    addr->data.sin.port = kdHtons(address.sin_port);
    return 0;
}

/* kdHtonl: Convert a 32-bit integer from host to network byte order. */
KD_API KDuint32 KD_APIENTRY kdHtonl(KDuint32 hostlong)
{
    union {
        KDint i;
        KDchar c;
    } u = {1};
    if(u.c)
    {
        KDuint8 *s = (KDuint8 *)&hostlong;
        return (KDuint32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
    }
    else
    {
        return hostlong;
    }
}

/* kdHtons: Convert a 16-bit integer from host to network byte order. */
KD_API KDuint16 KD_APIENTRY kdHtons(KDuint16 hostshort)
{
    union {
        KDint i;
        KDchar c;
    } u = {1};
    if(u.c)
    {
        KDuint8 *s = (KDuint8 *)&hostshort;
        return (KDuint16)(s[0] << 8 | s[1]);
    }
    else
    {
        return hostshort;
    }
}

/* kdNtohl: Convert a 32-bit integer from network to host byte order. */
KD_API KDuint32 KD_APIENTRY kdNtohl(KDuint32 netlong)
{
    return kdHtonl(netlong);
}

/* kdNtohs: Convert a 16-bit integer from network to host byte order. */
KD_API KDuint16 KD_APIENTRY kdNtohs(KDuint16 netshort)
{
    return kdHtons(netshort);
}

/* kdInetAton: Convert a "dotted quad" format address to an integer. */
KD_API KDint KD_APIENTRY kdInetAton(KD_UNUSED const KDchar *cp, KD_UNUSED KDuint32 *inp)
{
    kdSetError(KD_EOPNOTSUPP);
    return -1;
}

/* kdInetNtop: Convert a network address to textual form. */
KD_API const KDchar *KD_APIENTRY kdInetNtop(KDuint af, const void *src, KDchar *dst, KDsize cnt)
{
    if(af != KD_AF_INET)
    {
        kdSetError(KD_EAFNOSUPPORT);
        return KD_NULL;
    }
    if(cnt < KD_INET_ADDRSTRLEN)
    {
        kdSetError(KD_ENOSPC);
        return KD_NULL;
    }

    KDInAddr in;
    kdMemcpy(&in, src, (sizeof(KDInAddr)));
    KDuint32 address = kdNtohl(in.s_addr);
    KDuint8 *s = (KDuint8 *)&address;
    KDchar tempstore[sizeof("255.255.255.255")] = "";
    kdSnprintfKHR(tempstore, sizeof(tempstore), "%u.%u.%u.%u", s[3], s[2], s[1], s[0]);
    kdStrcpy_s(dst, cnt, tempstore);
    return (const KDchar *)dst;
}
