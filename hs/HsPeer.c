#include "HsPeer.h"
#include "HsCommon.h"
#include "HsRing.h"
#include "HsHash.h"
#include "HsAtomic.h"

int hsChannelInit(HsChannel *channel, int serverPort, int idx)
{
	char name[256];
	channel->sock = HS_INVALID_SOCK;
	channel->isIn = 0;
	channel->sendRing = (HsRing *)HS_MALLOC(sizeof(HsRing));
	sprintf(name, "chn%d%d", serverPort, idx);
	hsRingInit(channel->sendRing, name, HS_RING_SIZE_CROSS_THREAD);

	channel->recvBufLen = 1024 * 1024 * 5;
	channel->recvBuf = (char *)HS_MALLOC(channel->recvBufLen);

	hsChannelReset(channel);

	channel->peer = NULL;
	return 1;
}

int hsChannelReset(HsChannel *channel)
{
	channel->sendStream.p = NULL;
	channel->sendStream.leftLen = 0;
	channel->recvStream.p = channel->recvBuf;
	channel->recvStream.leftLen = channel->recvBufLen;

	return 1;
}

int hsChannelFini(HsChannel *channel)
{
	HS_FREE(channel->recvBuf);

	hsRingFini(channel->sendRing);
	HS_FREE(channel->sendRing);
	return 1;
}

int hsPeerInit(HsPeer *peer, int serverPort, const char *ip, int port)
{
	char name[256];
	peer->ip = HS_STRDUP(ip);
	peer->port = port;
	peer->echoRing = (HsHash *)HS_MALLOC(sizeof(HsHash));
	sprintf(name, "e%d%s%d", serverPort, ip, port);
	hsHashInit(peer->echoRing, name, HS_RING_SIZE_CROSS_PACKET);
	peer->idCur = (HsAtomic32 *)HS_MALLOC(sizeof(HsAtomic32));
	hsAtomic32Init(peer->idCur);

	peer->server = NULL;
	peer->client = NULL;

	return 1;
}

int hsPeerFini(HsPeer *peer)
{
	hsAtomic32Fini(peer->idCur);
	HS_FREE(peer->idCur);
	hsHashFini(peer->echoRing);
	HS_FREE(peer->echoRing);
	HS_FREE(peer->ip);

	return 1;
}