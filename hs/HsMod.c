#include "HsMod.h"
#include "HsRing.h"
#include "HsHash.h"
#include "HsAtomic.h"
#include "HsPeer.h"
#include "HsHash.h"
#include "HsEpoll.h"
#include "HsThread.h"
#include "HsMutex.h"
#include "HsStream.h"
#include "HsCommon.h"

static HsPeer *getPeer(HsMod *mod, int peerId)
{
	HS_ASSERT(0 <= peerId || peerId < mod->peerCount);
	return mod->peers + peerId;
}

static int hsChannelTriggerRead(HsChannel *channel, HsMod *mod)
{
	channel->isIn = 1;
	hsEpollTriggerRead(mod->epoll, channel->sock);
	return 1;
}

static int hsChannelTriggerWrite(HsChannel *channel, HsMod *mod)
{
	channel->isIn = 0;
	hsEpollTriggerWrite(mod->epoll, channel->sock);
	return 1;
}

static int sendOnePeer(HsMod *mod, int peerId, int msgType, const char *buf, int bufLen)
{
	HsItem *item;
	HsChannel *channel;
	HsPeer *peer = getPeer(mod, peerId);
	if (NULL == peer)
		return 0;
	channel = peer->server;
	if (NULL == channel)
	{
		HS_DBG(("when send msg %d to peer %d on server %d, connection is not ready.\n", msgType, peerId, mod->listeningPort));
		return 0;
	}
	item = (HsItem *)HS_MALLOC(sizeof(HsItem) + bufLen);
	hsAtomic32Increase(peer->idCur);
	item->id = hsAtomic32Get(peer->idCur);
	item->type = msgType;
	item->u.stamp = HS_TIME_NOW;
	item->len = sizeof(HsItem) + bufLen - sizeof(int);
	HS_MEMCPY(item->buf, buf, bufLen);
	hsRingIn(channel->sendRing, item);
	hsChannelTriggerWrite(channel, mod);
	return 1;	
}

static int hsMonitorCheckConnection(HsMod *mod)
{
	int cur;
	HsPeer *peer;
	HsChannel *channel;
	int sock;
	int isAllOk = 1;

	hsMutexLock(&mod->connectionMutex);
	for (cur = 0; cur < mod->peerCount; cur++)
	{
		peer = mod->peers + cur;
		if (NULL == peer->server)
		{
			sock = hsEpollConnect(mod->epoll, mod->selfIp, mod->listeningPort, peer->ip, peer->port);
			if (HS_INVALID_SOCK != sock)
			{
				channel = (HsChannel *)hsRingOut(mod->channelRing);
				if (NULL == channel)
				{
					HS_DBG(("none channel exists in ring %d.\n", mod->listeningPort));
					hsEpollRemoveSock(mod->epoll, sock);
					HS_SOCK_CLOSE(sock);
				}
				else
				{
					char buf[64];
					HsStream stream;

					hsStreamInit(&stream, buf, 64);
					hsStreamWriteStr(&stream, mod->selfIp);
					hsStreamWriteInt32(&stream, mod->listeningPort);
					channel->peer = peer;
					channel->sock = sock;
					peer->server = channel;
					hsHashAdd(mod->sock2ChannelHash, sock, channel);
					sendOnePeer(mod, cur, HS_MSG_TYPE_REGISTER, buf, (char *)stream.p - buf);
				}
			}
			else
				isAllOk = 0;
		}
	}
	hsMutexUnlock(&mod->connectionMutex);
	return isAllOk;
}

static void hsEchoRingIterate(void *user, struct HsHash *hash, void *value)
{
	HS_TIME_T lapse;
	HsMod *mod = (HsMod *)user;
	HsItem *item = (HsItem *)value;

	lapse = mod->lastCheckEchoRingTime - item->u.stamp;
	if (lapse > 60)
	{
		hsHashDel(hash, item->id);
		HS_DBG(("%s %d timeout, delete it.\n", hash->name, (int)lapse));
	}
}

static int hsEchoRingTimeOutCheck(HsMod *mod)
{
	int cur;
	HsPeer *peer;
	
	for (cur = 0; cur < mod->peerCount; cur++)
	{
		peer = mod->peers + cur;
		hsHashScan(peer->echoRing, hsEchoRingIterate, mod);
	}
	return 1;
}

static int hsMonitorThread(HsThread *thread)
{
	HsMod *mod = (HsMod *)thread->user;
	HS_TIME_T now;
	int checkConnectionSpan = 5;

	HS_DBG(("monitor listening port is %d.\n", mod->listeningPort));
	
	while (thread->isRunning)
	{
		now = HS_TIME_NOW;
		if (now - mod->lastCheckConnectionTime > checkConnectionSpan)
		{
			mod->lastCheckConnectionTime = now;
			if (hsMonitorCheckConnection(mod))
				checkConnectionSpan = 60;
			else
				checkConnectionSpan = 5;
		}
		whSleep(3);
	}
	return 1;
}

static int hsEpollThread(HsThread *thread)
{
	HsMod *mod = (HsMod *)thread->user;
	HS_TIME_T now;

	HS_DBG(("epoll listening port is %d.\n", mod->listeningPort));

	while (thread->isRunning)
	{
		now = HS_TIME_NOW;
		if (now - mod->lastCheckEchoRingTime > 60)
		{
			mod->lastCheckEchoRingTime = now;
			hsEchoRingTimeOutCheck(mod);
		}

		hsEpollOp(mod->epoll);
	}
	return 1;
}

static int onGetPacket(HsMod *mod, HsChannel *channel, HsItem *item)
{
	char buf[64];
	int bufLen;
	int len;
	HsPeer *peer;
	HsItem *itemResponse;
	HsItem *itemEcho;
	HsStream stream;
	char *ip = "0.0.0.0";
	int port;
	
	if (NULL == channel->peer)
	{
		if (item->type == HS_MSG_TYPE_REGISTER)
		{
			int cur;
			HsPeer *peer;

			hsStreamInit(&stream, item->buf, item->len + sizeof(item->len) - sizeof(*item));
			ip = hsStreamReadStr(&stream);
			port = hsStreamReadInt32(&stream);

			for (cur = 0; cur < mod->peerCount; cur++)
			{
				peer = mod->peers + cur;
				if (peer->port == port && HS_STREQU(peer->ip, ip))
				{
					peer->client = channel;
					channel->peer = peer;
					break;
				}
			}
		}
	}
	peer = channel->peer;
	if (NULL == peer)
	{
		HS_DBG(("%s:%d does not exist in peer list.\n", ip, port));
		return 0;
	}
	if (channel == peer->client)
	{
		buf[0] = 1;
		bufLen = 1;
		itemResponse = (HsItem *)HS_MALLOC(sizeof(HsItem) + bufLen);
		itemResponse->id = item->id;
		itemResponse->type = item->type;
		itemResponse->u.isRequest = 0;
		itemResponse->len = sizeof(HsItem) + bufLen - sizeof(int);
		HS_MEMCPY(itemResponse->buf, buf, bufLen);
		hsRingIn(channel->sendRing, itemResponse);
		hsChannelTriggerWrite(channel, mod);

		if (HS_MSG_TYPE_REGISTER == item->type)
			return 1;
		mod->recvFunc(mod->user, peer - mod->peers, item->type, item->buf, item->len + sizeof(int) - sizeof(HsItem));
	}
	else
	{
		itemEcho = (HsItem *)hsHashFind(peer->echoRing, item->id);
		if (NULL == itemEcho)
		{
			HS_DBG(("%s response packet is not in echo ring.\n", HS_FUNC));
			return 0;
		}
		hsHashDel(peer->echoRing, item->id);
		HS_FREE(itemEcho);
	}

	return 1;
}

static int onDisconnect(HsMod *mod, int fd)
{
	int cur;
	HsChannel *channel;
	HsPeer *peer;

	hsMutexLock(&mod->connectionMutex);
	channel = (HsChannel *)hsHashFind(mod->sock2ChannelHash, fd);
	if (NULL == channel)
	{
		HS_DBG(("couldn't find the disconnecting sock %d in server %d.\n", fd, mod->listeningPort));
	}
	else
	{
		for (cur = 0; cur < mod->peerCount; cur++)
		{
			peer = mod->peers + cur;
			if (channel == peer->server)
			{
				peer->server = NULL;
				break;
			}
			else if (channel == peer->client)
			{
				peer->client = NULL;
				break;
			}
		}
		hsHashDel(mod->sock2ChannelHash, fd);
		if (NULL != channel->peer && channel == channel->peer->client)
			HS_FREE(channel->sendStream.p);
		hsChannelReset(channel);
		channel->sock = HS_INVALID_SOCK;
		channel->peer = NULL;
		hsRingIn(mod->channelRing, channel);
	}
	hsMutexUnlock(&mod->connectionMutex);

	hsEpollRemoveSock(mod->epoll, fd);
	HS_SOCK_CLOSE(fd);

	return 1;
}

static int hsSock2ChannelOnAccept(HsMod *mod, int clientFd)
{
	int cur;
	HsChannel *channel;

	hsMutexLock(&mod->connectionMutex);
	channel = (HsChannel *)hsHashFind(mod->sock2ChannelHash, clientFd);
	if (NULL != channel)
	{
		HS_DBG(("there already exists sock %d on accept.", clientFd));
	}
	else
	{	
		channel = (HsChannel *)hsRingOut(mod->channelRing);
		if (NULL == channel)
		{
			HS_DBG(("none channel exists in ring %d on accept.\n", mod->listeningPort));
		}
		else
		{
			channel->sock = clientFd;
			hsHashAdd(mod->sock2ChannelHash, clientFd, channel);
			hsChannelTriggerRead(channel, mod);
		}
	}
	hsMutexUnlock(&mod->connectionMutex);
	return 1;
}

static HsChannel *getChannelBySockId(HsMod *mod, int fd)
{
	HsChannel *channel;
	hsMutexLock(&mod->connectionMutex);
	channel = (HsChannel *)hsHashFind(mod->sock2ChannelHash, fd);
	hsMutexUnlock(&mod->connectionMutex);
	if (NULL == channel)
		HS_DBG(("%s %d failed.\n", HS_FUNC, fd));
	return channel;
}

static int onAccept(void *user, int clientFd, const char *ip, int port)
{
	HsMod *mod = (HsMod *)user;
	hsSock2ChannelOnAccept(mod, clientFd);
	return 1;
}

static int getPacket(HsMod *mod, HsChannel *channel)
{
	int len;
	int packetLen;
	if (channel->recvStream.leftLen < 4)
		return 0;
	len = *(int *)channel->recvStream.p;
	packetLen = len + sizeof(int);
	if (packetLen > channel->recvStream.leftLen)
		return 0;

	onGetPacket(mod, channel, (HsItem *)channel->recvStream.p);

	channel->recvStream.p += packetLen;
	channel->recvStream.leftLen -= packetLen;

	return 1;
}

static int onRead(void *user, int clientFd)
{
	int len;
	int leftRoom;
	HsMod *mod = (HsMod *)user;
	HsChannel *channel;
	channel = getChannelBySockId(mod, clientFd);
	if (NULL == channel)
		return 0;

	while (1)
	{
		len = HS_SOCK_RECV(clientFd, channel->recvStream.p, channel->recvStream.leftLen, 0);
		if (len <= 0)
		{
			HS_DBG(("%s %d dead.\n", HS_FUNC, clientFd));
			onDisconnect(mod, clientFd);
			return 0;
		}
		leftRoom = channel->recvStream.leftLen - len;
		len += (char *)channel->recvStream.p - (char *)channel->recvBuf;
		channel->recvStream.p = channel->recvBuf;
		channel->recvStream.leftLen = len;
		while (getPacket(mod, channel))
			;
		if (channel->recvStream.p != channel->recvBuf)
		{
			if (channel->recvStream.leftLen > 0)
				HS_MEMMOVE(channel->recvBuf, channel->recvStream.p, channel->recvStream.leftLen);
			channel->recvStream.p = channel->recvBuf + channel->recvStream.leftLen;
			channel->recvStream.leftLen = channel->recvBufLen - channel->recvStream.leftLen;
		}
		if (leftRoom > 0)
			break;
		HS_DBG(("%s %d left room is 0, so receive again.\n", HS_FUNC, clientFd));
	}

	return 1;
}

static int onWrite(void *user, int clientFd)
{
	int sentLen;
	HsMod *mod = (HsMod *)user;
	HsChannel *channel;
	int ret = 1;

	hsMutexLock(&mod->connectionMutex);
	channel = getChannelBySockId(mod, clientFd);
	if (NULL == channel)
	{
		ret = 0;
		goto out;
	}
	if (0 == channel->sendStream.leftLen)
	{
		HsItem *item;
		item = hsRingOut(channel->sendRing);
		if (NULL == item)
		{
			ret = 0;
			goto out;
		}
		if (NULL != channel->peer && channel == channel->peer->server)
		{
			hsHashAdd(channel->peer->echoRing, item->id, item);
			HS_DBG(("add item %d to echo ring %s:%d.\n", item->id, channel->peer->ip, channel->peer->port));
		}
		else if (NULL != channel->sendStream.p)
		{
			HS_FREE(channel->sendStream.p);
		}
		HS_DBG(("will send item %p:%d.\n", item, item->id));
		channel->sendStream.p = item;
		channel->sendStream.leftLen = item->len + sizeof(int);
	}
	sentLen = HS_SOCK_SEND(clientFd, channel->sendStream.p, channel->sendStream.leftLen, 0);
	if (sentLen < 0)
	{
		HS_DBG(("%s %d write error.\n", HS_FUNC, clientFd));
		ret = 0;
		goto out;
	}
	if (sentLen != channel->sendStream.leftLen)
	{
		HS_DBG(("%s %d write %d of %d.\n", HS_FUNC, clientFd, sentLen, channel->sendStream.leftLen));
	}
	channel->sendStream.p += sentLen;
	channel->sendStream.leftLen -= sentLen;
	if (0 == channel->sendStream.leftLen)
	{
		channel->sendStream.p = NULL;
		hsChannelTriggerRead(channel, mod);
	}
	else
		hsChannelTriggerWrite(channel, mod);

out:
	hsMutexUnlock(&mod->connectionMutex);
	return ret;
}

static void hsDataIterate(void *user, struct HsHash *hash, void *value)
{
	HS_TIME_T lapse;
	HsMod *mod = (HsMod *)user;
	HsChannel *channel = (HsChannel *)value;

/*	if (channel->isIn)
		onRead(user, channel->sock);
	else
*/		onWrite(user, channel->sock);
}

static int onData(void *user, int clientFd)
{
	HsMod *mod = (HsMod *)user;

	hsHashScan(mod->sock2ChannelHash, hsDataIterate, mod);
	return 1;
}

HsMod *hsModCreate(HsModRecv recvFunc, void *user, int serverCount, const char *selfIp, int listenPort, struct HsEnv *env)
{
	int i;
	char name[256];
	HsMod *mod = (HsMod *)HS_MALLOC(sizeof(HsMod));
	mod->epoll = (HsEpoll *)HS_MALLOC(sizeof(HsEpoll));
	mod->epoll->user = mod;
	mod->epoll->onAccept = onAccept;
	mod->epoll->onRead = onRead;
	mod->epoll->onWrite = onWrite;
	mod->epoll->onData = onData;
	hsEpollInit(mod->epoll, listenPort);
	mod->peerMaxCount = serverCount;
	mod->peerCount = 0;
	mod->peerCur = 0;
	mod->peers = (HsPeer *)HS_MALLOC(sizeof(HsPeer) * serverCount);
	mod->channelRing = (HsRing *)HS_MALLOC(sizeof(HsRing));
	sprintf(name, "chnRing%d", listenPort);
	hsRingInit(mod->channelRing, name, serverCount * 3);
	for (i = 0; i < serverCount * 2; i++)
	{
		HsChannel *channel = (HsChannel *)HS_MALLOC(sizeof(HsChannel));
		hsChannelInit(channel, listenPort, i);
		hsRingIn(mod->channelRing, channel);
	}
	mod->sock2ChannelHash = (HsHash *)HS_MALLOC(sizeof(HsHash));
	sprintf(name, "sockPeer%d", listenPort);
	hsHashInit(mod->sock2ChannelHash, name, serverCount * 3);
	mod->monitorThread = (HsThread *)HS_MALLOC(sizeof(HsThread));
	hsThreadInit(mod->monitorThread, hsMonitorThread, mod, env);
	mod->epollThread = (HsThread *)HS_MALLOC(sizeof(HsThread));
	hsThreadInit(mod->epollThread, hsEpollThread, mod, env);
	hsMutexInit(&mod->connectionMutex, 2);
	mod->lastCheckConnectionTime = HS_TIME_NOW - 60 * 60 * 24;
	mod->lastCheckEchoRingTime = HS_TIME_NOW;
	mod->recvFunc = recvFunc;
	mod->user = user;

	mod->selfIp = HS_STRDUP(selfIp);
	mod->listeningPort = listenPort;
	HS_DBG(("listening port is %d.\n", listenPort));

	return mod;
}

static void deleteChannelInHash(void *user, struct HsHash *hash, void *value)
{
	HsChannel *channel = (HsChannel *)value;
	hsChannelFini(channel);
	HS_FREE(channel);
}

int hsModDelete(HsMod *mod)
{
	HsChannel *channel;
	int i;
	HS_FREE(mod->selfIp);
	hsMutexFini(&mod->connectionMutex);
	hsThreadFini(mod->epollThread);
	HS_FREE(mod->epollThread);
	hsThreadFini(mod->monitorThread);
	HS_FREE(mod->monitorThread);
	hsHashScan(mod->sock2ChannelHash, deleteChannelInHash, mod);
	hsHashFini(mod->sock2ChannelHash);
	HS_FREE(mod->sock2ChannelHash);
	while (NULL != (channel = hsRingOut(mod->channelRing)))
	{
		hsChannelFini(channel);
		HS_FREE(channel);
	}
	hsRingFini(mod->channelRing);
	HS_FREE(mod->channelRing);
	for (i = 0; i < mod->peerCur; i++)
		hsPeerFini(mod->peers + i);
	HS_FREE(mod->peers);
	hsEpollFini(mod->epoll);
	HS_FREE(mod->epoll);
	HS_FREE(mod);
	return 1;
}

int hsModAddPeer(HsMod *mod, const char *ip, int port)
{
	HsPeer *peer;
	if (mod->peerCur >= mod->peerMaxCount)
		return 0;
	peer = mod->peers + mod->peerCur++;
	hsPeerInit(peer, mod->listeningPort, ip, port);
	return 1;
}

int hsModStart(HsMod *mod)
{
	mod->peerCount = mod->peerCur;

	hsThreadStart(mod->epollThread);
	hsThreadStart(mod->monitorThread);

	return 1;
}

int hsModSend(HsMod *mod, int peerId, int msgType, const char *buf, int bufLen)
{
	int cur;
	HsPeer *peer;

	if (peerId >= 0)
		return sendOnePeer(mod, peerId, msgType, buf, bufLen);

	for (cur = 0; cur < mod->peerCount; cur++)
	{
		sendOnePeer(mod, cur, msgType, buf, bufLen);
	}
	return 1;
}
