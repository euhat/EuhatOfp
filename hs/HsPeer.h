#ifndef _HS_PEER_H_
#define _HS_PEER_H_

struct HsRing;
struct HsAtomic32;

#include "HsStream.h"
#include "HsCommon.h"

typedef struct HsItem
{
	int len;
	int id;
	int type;
	union
	{
		int isRequest;
		HS_TIME_T stamp;
	} u;
	char buf[0];
} HsItem;

struct HsPeer;

typedef struct HsChannel
{
	int sock;
	int isIn;
	struct HsRing *sendRing;

	void *recvBuf;
	int recvBufLen;
	HsStream sendStream;
	HsStream recvStream;

	struct HsPeer *peer;
} HsChannel;

int hsChannelInit(HsChannel *channel, int serverPort, int idx);
int hsChannelReset(HsChannel *channel);
int hsChannelFini(HsChannel *channel);

typedef struct HsPeer
{
	char *ip;
	int port;
	struct HsChannel *server;
	struct HsChannel *client;
	struct HsHash *echoRing;
	struct HsAtomic32 *idCur;
} HsPeer;

int hsPeerInit(HsPeer *peer, int serverPort, const char *ip, int port);
int hsPeerFini(HsPeer *peer);

#endif