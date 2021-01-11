#ifndef _HS_CONFIG_H_
#define _HS_CONFIG_H_

#define HS_SYS_EPOLL 0
#define HS_OFP_EPOLL 1

#define HS_EPOLL_TYPE HS_OFP_EPOLL

#if HS_EPOLL_TYPE == HS_SYS_EPOLL
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#else
#include <ofp.h>
#endif

#endif