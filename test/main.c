#include "../hs/HsMod.h"
#include "../hs/HsPeer.h"
#include "../hs/HsCommon.h"
#include "../hs/HsList.h"
#include "../hs/HsThread.h"
#include "../hs/HsEpoll.h"

typedef struct HsPeerCfgItem
{
	struct list_head node;
	char *ip;
	int port;
} HsPeerCfgItem;

static int cfgRead(const char *filePath, struct list_head *head)
{
	FILE *fp;
	char line[1024];
	char *p;
	HsPeerCfgItem *item;

	INIT_LIST_HEAD(head);

	fp = fopen(filePath, "r");
	if (NULL == fp)
		return 0;

	while (NULL != fgets(line, 1023, fp))
	{
		p = strstr(line, "\n");
		if (NULL != p)
			*p = 0;
		p = strstr(line, ":");
		if (NULL == p)
			continue;
		*p++ = 0;
		item = (HsPeerCfgItem *)HS_MALLOC(sizeof(HsPeerCfgItem));
		item->ip = HS_STRDUP(line);
		item->port = HS_ATOI(p);
		list_add_tail(&item->node, head);
	}

	fclose(fp);
	return 1;
}

static int cfgFini(struct list_head *head)
{
	HsPeerCfgItem *item;

	while (!list_is_empty(head))
	{
		item = container_of(head->next, HsPeerCfgItem, node);
		list_del(&item->node);
		HS_FREE(item->ip);
		HS_FREE(item);
	}
	return 1;
}

typedef struct HsServerContext
{
	struct list_head node;
	struct list_head *cfgHead;
	HsPeerCfgItem *cfgItem;
	int cfgCount;
	int serverId;
	HsMod *mod;
	HsThread thread;
	struct HsEnv *env;
} HsServerContext;

static int recvFunc(void *user, int peerId, int msgType, char *buf, int bufLen)
{
	HsServerContext *serverContext = (HsServerContext *)user;
	HsPeer *peer = serverContext->mod->peers + peerId;

	if (HS_STRLEN(buf) + 1 != bufLen)
	{
		HS_DBG(("%s msg type %d buf len is wrong, [%s]:%d.\n", HS_FUNC, msgType, buf, bufLen - 1));
		return 0;
	}
	HS_DBG(("recv from %s:%d, [%s], msg type %d.\n", peer->ip, peer->port, buf, msgType));
	return 1;
}

static int serverProc(HsThread *thread)
{
	const char *hellos[] = {
		"  Hello, the world!",
		"  What's your problem?",
		"  My name is Jack.",
		"  How old are you?",
		"  I'm 15."
	};
    HsServerContext *context;
	HsPeerCfgItem *item;
	int i = 0;

	context = (HsServerContext *)thread->user;
 
	context->mod = hsModCreate(recvFunc, context, context->cfgCount - 1, context->cfgItem->ip, context->cfgItem->port, context->env);

	list_for_each_entry(item, context->cfgHead, node)
	{
		if (item != context->cfgItem)
			hsModAddPeer(context->mod, item->ip, item->port);
	}

	hsModStart(context->mod);

	while (1)
	{
		whSleep(1);

		i = (i + 1) % (sizeof(hellos) / sizeof(*hellos));
		hsModSend(context->mod, -1, 1, hellos[i], HS_STRLEN(hellos[i]) + 1);
	}

	hsModDelete(context->mod);

	return 1;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	struct list_head cfgHead;
	struct list_head serverHead;
	int count = 0;
	int i = 0;
	HsPeerCfgItem *item;
	HsServerContext *context;
	int serverIdx;
	char *argvInner[32];
	int argcInner;
	struct HsEnv *env;

	if (argc < 2)
		serverIdx = -1;
	else
		serverIdx = HS_ATOI(argv[1]);

	argvInner[0] = "test";
	argvInner[1] = "-i";
	argvInner[2] = "0";
	argvInner[3] = "-f";
	argvInner[4] = "ofp_netwrap.cli";
	argcInner = 5;

	env = hsEnvCreate(argcInner, argvInner);
	if (NULL == env)
	{
		HS_DBG(("epll global init failed.\n"));
		return 0;
	}

#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	cfgRead("servers_loop.cfg", &cfgHead);
#else
	cfgRead("servers.cfg", &cfgHead);
#endif
	list_for_each_entry(item, &cfgHead, node)
		count++;

	INIT_LIST_HEAD(&serverHead);
	list_for_each_entry(item, &cfgHead, node)
	{
		if (serverIdx == -1)
			;
		else if (serverIdx != i)
		{
			i++;
			continue;
		}
		context = (HsServerContext *)HS_MALLOC(sizeof(*context));
		context->env = env;
		context->cfgItem = item;
		context->cfgHead = &cfgHead;
		context->cfgCount = count;
		context->serverId = i++;
		list_add(&context->node, &serverHead);
		hsThreadInit(&context->thread, serverProc, context, env);
		hsThreadStart(&context->thread);
		//serverProc(context);
	}

	while (1)
		whSleep(10);

	while (list_is_empty(&serverHead))
	{
		context = container_of(serverHead.next, HsServerContext, node);
		list_del(&context->node);
		hsThreadStop(&context->thread);
		HS_FREE(context);
	}

out:
	cfgFini(&cfgHead);

	return ret;
}