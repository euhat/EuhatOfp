#ifndef _HS_EPOLL_H_
#define _HS_EPOLL_H_

#include "HsThread.h"

#if HS_EPOLL_TYPE == HS_SYS_EPOLL
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#define HS_SOCK_PREFIX(_name) _name
#define HS_SOCK_PREFIX_UPPER(_name) _name
#else
#include <ofp.h>
#include "HsThread.h"
#define HS_SOCK_PREFIX(_name) ofp_##_name
#define OFP_EPOLLOUT OFP_EPOLLIN + 1
#define HS_SOCK_PREFIX_UPPER(_name) OFP_##_name
int hsOfpThreadStart(HsThread *thread, HsThreadFuncVoidP func);
int hsOfpThreadPrepare(HsThread *thread);
int hsOfpThreadStop(HsThread *thread);
#endif

#define HS_SOCK_AF_INET HS_SOCK_PREFIX_UPPER(AF_INET)
#define HS_SOCK_SOCK_STREAM HS_SOCK_PREFIX_UPPER(SOCK_STREAM)
#define HS_SOCK_IPPROTO_TCP HS_SOCK_PREFIX_UPPER(IPPROTO_TCP)
#define HS_SOCK_EPOLL_EVENT HS_SOCK_PREFIX(epoll_event)
#define HS_SOCK_EPOLLIN HS_SOCK_PREFIX_UPPER(EPOLLIN)
#define HS_SOCK_EPOLLOUT HS_SOCK_PREFIX_UPPER(EPOLLOUT)
#define HS_SOCK_EPOLL_CTL_ADD HS_SOCK_PREFIX_UPPER(EPOLL_CTL_ADD)
#define HS_SOCK_EPOLL_CTL_DEL HS_SOCK_PREFIX_UPPER(EPOLL_CTL_DEL)
#define HS_SOCK_EPOLL_CTL_MOD HS_SOCK_PREFIX_UPPER(EPOLL_CTL_MOD)
#define HS_SOCK_SOCKADDR_IN HS_SOCK_PREFIX(sockaddr_in)
#define HS_SOCK_SOCKADDR HS_SOCK_PREFIX(sockaddr)
#define HS_SOCK_INET_NTOA HS_SOCK_PREFIX(inet_ntoa)
#define HS_SOCK_RECV HS_SOCK_PREFIX(recv)
#define HS_SOCK_SEND HS_SOCK_PREFIX(send)
#define HS_SOCK_CLOSE HS_SOCK_PREFIX(close)
#define HS_SOCK_LISTEN HS_SOCK_PREFIX(listen)
#define HS_SOCK_SOCKET HS_SOCK_PREFIX(socket)
#define HS_SOCK_EPOLL_CREATE HS_SOCK_PREFIX(epoll_create)
#define HS_SOCK_EPOLL_CTL HS_SOCK_PREFIX(epoll_ctl)
#define HS_SOCK_EPOLL_WAIT HS_SOCK_PREFIX(epoll_wait)
#define HS_SOCK_BIND HS_SOCK_PREFIX(bind)
#define HS_SOCK_ACCEPT HS_SOCK_PREFIX(accept)
#define HS_SOCK_CONNECT HS_SOCK_PREFIX(connect)

struct HsEnv;

struct HsEnv *hsEnvCreate(int argc, char *argv[]);
void hsEnvDelete(struct HsEnv *env);

typedef struct HsEpoll
{
	void *user;
	int epollFd;
	int listenFd;

	int (*onAccept)(void *user, int clientFd, const char *ip, int port);
	int (*onRead)(void *user, int clientFd);
	int (*onWrite)(void *user, int clientFd);
	int (*onData)(void *user, int clientFd);
} HsEpoll;

int hsEpollInit(HsEpoll *ep, int listenPort);
int hsEpollFini(HsEpoll *ep);

int hsEpollOp(HsEpoll *ep);

int hsEpollAddSock(HsEpoll *ep, int fd);
int hsEpollRemoveSock(HsEpoll *ep, int fd);

int hsEpollTriggerWrite(HsEpoll *ep, int fd);
int hsEpollTriggerRead(HsEpoll *ep, int fd);

int hsEpollConnect(HsEpoll *ep, const char *selfIp, int selfPort, const char *ip, int port);

#endif
