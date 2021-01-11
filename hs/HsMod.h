#ifndef _HS_MOD_H_
#define _HS_MOD_H_

#include "HsCommon.h"
#include "HsMutex.h"

struct HsThread;
struct HsHash;
struct HsPeer;
struct HsEnv;

typedef int (*HsModRecv)(void *user, int peerId, int msgType, char *buf, int bufLen);

typedef struct HsMod
{
	struct HsEpoll *epoll;
	struct HsHash *sock2ChannelHash;
	struct HsRing *channelRing;
	struct HsPeer *peers;
	int peerMaxCount;
	int peerCount;
	int peerCur;
	struct HsThread *monitorThread;
	struct HsThread *epollThread;
	HsMutex connectionMutex;
	HS_TIME_T lastCheckConnectionTime;
	HS_TIME_T lastCheckEchoRingTime;
	char *selfIp;
	int listeningPort;

	HsModRecv recvFunc;
	void *user;
} HsMod;

HsMod *hsModCreate(HsModRecv recvFunc, void *user, int serverCount, const char *selfIp, int listenPort, struct HsEnv *env);
int hsModDelete(HsMod *mod);
int hsModAddPeer(HsMod *mod, const char *ip, int port);
int hsModStart(HsMod *mod);
int hsModSend(HsMod *mod, int peerId, int msgType, const char *buf, int bufLen);

#endif
