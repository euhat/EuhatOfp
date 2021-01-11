#include "HsEpoll.h"
#include "HsCommon.h"
#include "HsThread.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#if 1
#define HS_EPOLL_MODE 0
#else
#define HS_EPOLL_MODE EPOLLET
#endif

#if HS_EPOLL_TYPE == HS_OFP_EPOLL

extern int ofp_wakeup_one(void *channel);

typedef struct {
	int core_count;
	int if_count;		/**< Number of interfaces to be used */
	char **if_names;	/**< Array of pointers to interface names */
	char *cli_file;
} appl_args_t;

static void parse_args(int argc, char *argv[], appl_args_t *appl_args)
{
	int opt;
	int long_index;
	char *names, *str, *token, *save;
	size_t len;
	int i;
	static struct option longopts[] = {
		{"count", required_argument, NULL, 'c'},
		{"interface", required_argument, NULL, 'i'},	/* return 'i' */
		{"help", no_argument, NULL, 'h'},		/* return 'h' */
		{"cli-file", required_argument, NULL, 'f'},/* return 'f' */
		{NULL, 0, NULL, 0}
	};

	memset(appl_args, 0, sizeof(*appl_args));

	while (1) {
		opt = getopt_long(argc, argv, "+c:i:hf:",
				  longopts, &long_index);

		if (opt == -1)
			break;	/* No more options */

		switch (opt) {
		case 'c':
			appl_args->core_count = atoi(optarg);
			break;
			/* parse packet-io interface names */
		case 'i':
			len = strlen(optarg);
			if (len == 0) {
				goto fail;
			}
			len += 1;	/* add room for '\0' */

			names = malloc(len);
			if (names == NULL) {
				goto fail;
			}

			/* count the number of tokens separated by ',' */
			strcpy(names, optarg);
			for (str = names, i = 0;; str = NULL, i++) {
				token = strtok_r(str, ",", &save);
				if (token == NULL)
					break;
			}
			appl_args->if_count = i;

			if (appl_args->if_count == 0) {
				goto fail;
			}

			/* allocate storage for the if names */
			appl_args->if_names =
				calloc(appl_args->if_count, sizeof(char *));

			/* store the if names (reset names string) */
			strcpy(names, optarg);
			for (str = names, i = 0;; str = NULL, i++) {
				token = strtok_r(str, ",", &save);
				if (token == NULL)
					break;
				appl_args->if_names[i] = token;
			}
			break;

		case 'h':
			goto fail;
			break;

		case 'f':
			len = strlen(optarg);
			if (len == 0) {
				goto fail;
			}
			len += 1;	/* add room for '\0' */

			appl_args->cli_file = malloc(len);
			if (appl_args->cli_file == NULL) {
				goto fail;
			}

			strcpy(appl_args->cli_file, optarg);
			break;

		default:
			break;
		}
	}

	if (appl_args->if_count == 0) {
		goto fail;
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */
	return;

fail:
	HS_DBG(("%s failed.\n", HS_FUNC));
}

static void print_info(char *progname, appl_args_t *appl_args)
{
	int i;

	printf("\n"
		   "ODP system info\n"
		   "---------------\n"
		   "ODP API version: %s\n"
		   "CPU model:       %s\n"
		   "Cache line size: %i\n"
		   "Core count:      %i\n"
		   "\n",
		   odp_version_api_str(), odp_cpu_model_str(),
		   odp_sys_cache_line_size(),
		   odp_cpu_count());

	printf("Running ODP appl: \"%s\"\n"
		   "-----------------\n"
		   "IF-count:        %i\n"
		   "Using IFs:      ",
		   progname, appl_args->if_count);
	for (i = 0; i < appl_args->if_count; ++i)
		printf(" %s", appl_args->if_names[i]);
	printf("\n\n");
	fflush(NULL);
}

static enum ofp_return_code fastpath_local_hook(odp_packet_t pkt, void *arg)
{
	int protocol = *(int *)arg;
	(void) pkt;
	(void) protocol;
	return OFP_PKT_CONTINUE;
}

typedef struct HsEnv
{
	odp_instance_t instance;
	ofp_global_param_t appInitParams;
} HsEnv;

int hsOfpThreadStart(HsThread *thread, HsThreadFuncVoidP func)
{
	odp_cpumask_t cpumask;
	odph_odpthread_params_t thr_params;

	odp_cpumask_zero(&cpumask);
	odp_cpumask_set(&cpumask, thread->env->appInitParams.linux_core_id);

	thr_params.start = func;
	thr_params.arg = thread;
	thr_params.thr_type = ODP_THREAD_WORKER;
	thr_params.instance = thread->env->instance;
	odph_odpthreads_create(&thread->handle,
			       &cpumask,
			       &thr_params);
	return 1;
}

int hsOfpThreadPrepare(HsThread *thread)
{
	if (ofp_init_local()) {
		HS_DBG(("Error: OFP local init failed.\n"));
		return 0;
	}
	return 1;
}

int hsOfpThreadStop(HsThread *thread)
{
	odph_odpthreads_join(&thread->handle);
	HS_MEMSET(&thread->handle, 0, sizeof(thread->handle));
	return 1;
}
#else

typedef struct HsEnv
{
	int i;
} HsEnv;

#endif

HsEnv *hsEnvCreate(int argc, char *argv[])
{
	HsEnv *env = NULL;
	int portCount;
	struct rte_eth_dev_info dev_info;

	env = (HsEnv *)HS_MALLOC(sizeof(*env));
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	int ret;
	ret = rte_eal_init(1, argv);
	if (ret < 0)
	{
		HS_DBG(("dpdk init failed.\n"));
		return 0;
	}
	portCount = rte_eth_dev_count_avail();
	portCount = rte_eth_dev_info_get(0, &dev_info);
	return env;
#else
#define MAX_WORKERS		32
	odp_init_t init_param;
	appl_args_t params;
	int core_count, num_workers;
	char cpumaskstr[64];
	odp_cpumask_t cpumask;
	odph_odpthread_params_t thr_params;
	odph_odpthread_t thread_tbl[MAX_WORKERS];

/*	rte_mempool_ring_init();
*/	
/*	struct rlimit rlp;
	getrlimit(RLIMIT_CORE, &rlp);
	printf("RLIMIT_CORE: %ld/%ld\n", rlp.rlim_cur, rlp.rlim_max);
	rlp.rlim_cur = 200000000;
	printf("Setting to max: %d\n", setrlimit(RLIMIT_CORE, &rlp));
*/
	/* Parse and store the application arguments */
	parse_args(argc, argv, &params);

	odp_init_param_init(&init_param);
	init_param.not_used.feat.ipsec = 1;
	init_param.not_used.feat.crypto = 1;
	init_param.not_used.feat.compress = 1;
	init_param.not_used.feat.tm = 1;

	if (0 != odp_init_global(&env->instance, &init_param, NULL))
	{
		HS_DBG(("init odp global failed.\n"));
		goto fail;
	}
	if (0 != odp_init_local(env->instance, ODP_THREAD_CONTROL))
	{
		HS_DBG(("init odp local failed.\n"));
		goto fail;
	}

	print_info(argv[0], &params);
	core_count = odp_cpu_count();
	num_workers = core_count;

	if (params.core_count)
		num_workers = params.core_count;

	ofp_init_global_param(&env->appInitParams);

	if (core_count > 1)
		num_workers--;

	num_workers = odp_cpumask_default_worker(&cpumask, num_workers);
	odp_cpumask_to_str(&cpumask, cpumaskstr, sizeof(cpumaskstr));

	printf("Num worker threads: %i\n", num_workers);
	printf("first CPU:          %i\n", odp_cpumask_first(&cpumask));
	printf("cpu mask:           %s\n", cpumaskstr);

	env->appInitParams.if_count = params.if_count;
	env->appInitParams.if_names = params.if_names;
	env->appInitParams.pkt_hook[OFP_HOOK_LOCAL] = fastpath_local_hook;
	if (ofp_init_global(env->instance, &env->appInitParams))
	{
		HS_DBG(("Error: OFP global init failed.\n"));
		goto fail;
	}

	memset(thread_tbl, 0, sizeof(thread_tbl));
	/* Start dataplane dispatcher worker threads */

	thr_params.start = default_event_dispatcher;
	thr_params.arg = ofp_eth_vlan_processing;
	thr_params.thr_type = ODP_THREAD_WORKER;
	thr_params.instance = env->instance;
	odph_odpthreads_create(thread_tbl,
			       &cpumask,
			       &thr_params);

	/* other app code here.*/
	/* Start CLI */
	ofp_start_cli_thread(env->instance, env->appInitParams.linux_core_id, params.cli_file);

	return env;
fail:
	HS_FREE(env);
	return NULL;
#endif
}

void hsEnvDelete(struct HsEnv *env)
{
	HS_FREE(env);
}

static int setNonBlocking(int sock)
{
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	int opts;
	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
	{
		HS_DBG(("fcntl(sock, GETFL) error.\n"));
	return 0;
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
	{
		HS_DBG(("fcntl(sock, SETFL, opts) error.\n"));
	return 0;
	}
	return 1;
#else
	return 1;
#endif
}

int hsListen(int sockFd, int backlog)
{
	char *ptr;
 
	if (NULL != (ptr = getenv("LISTENQ")))
		backlog = HS_ATOI(ptr);
	if (HS_SOCK_LISTEN(sockFd, backlog) < 0)
		HS_DBG(("listen error.\n"));
	return 1;
}

int hsEpollInit(HsEpoll *ep, int listenPort)
{
	struct HS_SOCK_EPOLL_EVENT ev;
	struct HS_SOCK_SOCKADDR_IN serverAddr = { 0 };

	ep->listenFd = HS_SOCK_SOCKET(HS_SOCK_AF_INET, HS_SOCK_SOCK_STREAM, HS_SOCK_IPPROTO_TCP);
	setNonBlocking(ep->listenFd);

	ep->epollFd = HS_SOCK_EPOLL_CREATE(256);
	ev.data.fd = ep->listenFd;
	ev.events = HS_SOCK_EPOLLIN | HS_EPOLL_MODE;
	HS_SOCK_EPOLL_CTL(ep->epollFd, HS_SOCK_EPOLL_CTL_ADD, ep->listenFd, &ev);

	serverAddr.sin_family = HS_SOCK_AF_INET;
/*	inet_aton("127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_addr.s_addr = ofp_port_get_ipv4_addr(0, 0, OFP_PORTCONF_IP_TYPE_IP_ADDR);
*/	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(listenPort);
#if HS_EPOLL_TYPE == HS_OFP_EPOLL
	serverAddr.sin_addr.s_addr = ofp_port_get_ipv4_addr(0, 0, OFP_PORTCONF_IP_TYPE_IP_ADDR);
	serverAddr.sin_port = odp_cpu_to_be_16(listenPort);
	serverAddr.sin_len = sizeof(serverAddr);
#endif

	while (1)
	{
		if (HS_SOCK_BIND(ep->listenFd, (struct HS_SOCK_SOCKADDR *)&serverAddr, sizeof(serverAddr)) < 0)
		{
			HS_DBG(("bind port %d failed, retry...\n", listenPort));
			whSleep(1);
		}
		else
		{
			HS_DBG(("bind port %d ok.\n", listenPort));
			break;
		}
	}
	hsListen(ep->listenFd, 50);
	return 1;
}

int hsEpollFini(HsEpoll *ep)
{
	return 1;
}

int hsEpollAddSock(HsEpoll *ep, int fd)
{
	int ret;
	struct HS_SOCK_EPOLL_EVENT ev;
	ev.data.fd = fd;
	ev.events= HS_SOCK_EPOLLIN | HS_EPOLL_MODE;
	ret = HS_SOCK_EPOLL_CTL(ep->epollFd, HS_SOCK_EPOLL_CTL_ADD, fd, &ev);
	return ret >= 0;
}

int hsEpollRemoveSock(HsEpoll *ep, int fd)
{
	int ret;
	ret = HS_SOCK_EPOLL_CTL(ep->epollFd, HS_SOCK_EPOLL_CTL_DEL, fd, NULL);
	return ret >= 0;
}

static int hsEpollOnWrite(HsEpoll *ep, int fd)
{
	return ep->onWrite(ep->user, fd);
}

int hsEpollTriggerRead(HsEpoll *ep, int fd)
{
	int ret;
	struct HS_SOCK_EPOLL_EVENT ev;
	ev.data.fd = fd;
	ev.events = HS_SOCK_EPOLLIN | HS_EPOLL_MODE;
	ret = HS_SOCK_EPOLL_CTL(ep->epollFd, HS_SOCK_EPOLL_CTL_MOD, fd, &ev);
#if HS_EPOLL_TYPE == HS_OFP_EPOLL
	ofp_wakeup_one(NULL);
#endif
	return ret >= 0;
}

int hsEpollTriggerWrite(HsEpoll *ep, int fd)
{
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	int ret;
	struct HS_SOCK_EPOLL_EVENT ev;
	ev.data.fd = fd;
	ev.events = HS_SOCK_EPOLLOUT | HS_EPOLL_MODE;
	ret = HS_SOCK_EPOLL_CTL(ep->epollFd, HS_SOCK_EPOLL_CTL_MOD, fd, &ev);
	return ret >= 0;
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
	return hsEpollTriggerRead(ep, fd);
#endif
}

static int hsEpollOnAccept(HsEpoll *ep)
{
	struct HS_SOCK_SOCKADDR_IN clientAddr = { 0 };
	socklen_t clientAddrLen;
	int fd;
	const char *ip;
	int port;

	clientAddrLen = sizeof(clientAddr);
	fd = HS_SOCK_ACCEPT(ep->listenFd, (struct HS_SOCK_SOCKADDR *)&clientAddr, &clientAddrLen);
	if (HS_INVALID_SOCK == fd)
		return 0;

	ip = HS_SOCK_INET_NTOA(clientAddr.sin_addr);
	port = ntohs(clientAddr.sin_port);
	HS_DBG(("accept a connection %s:%d.\n", ip, port));

	clientAddrLen = sizeof(clientAddr);
/*	if (HS_SOCK_GET_PEER_NAME(fd, (struct HS_SOCK_SOCKADDR *)&clientAddr, &clientAddrLen) < 0)
		HS_DBG(("get peer port error.\n"));
	else
	{
		port = ntohs(clientAddr.sin_port);
	}
*/
	setNonBlocking(fd);
	hsEpollAddSock(ep, fd);
	ep->onAccept(ep->user, fd, ip, port);

	return 1;
}

static int hsEpollOnRead(HsEpoll *ep, int fd)
{
	return ep->onRead(ep->user, fd);
}

static int hsEpollOnData(HsEpoll *ep, int fd)
{
	return ep->onData(ep->user, fd);
}

int hsEpollOp(HsEpoll *ep)
{
	int i;
	int eventCount;
	struct HS_SOCK_EPOLL_EVENT events[20];
	int clientFd;

	eventCount = HS_SOCK_EPOLL_WAIT(ep->epollFd, events, 20, 1);
	for (i = 0; i < eventCount; i++)
	{
		if (events[i].data.fd == ep->listenFd)
			hsEpollOnAccept(ep);
		else if (events[i].events & HS_SOCK_EPOLLIN)
			hsEpollOnRead(ep, events[i].data.fd);
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
		else if (events[i].events & HS_SOCK_EPOLLOUT)
			hsEpollOnWrite(ep, events[i].data.fd);
#endif
	}
#if HS_EPOLL_TYPE == HS_OFP_EPOLL
	hsEpollOnData(ep, -1);
#endif
	return 1;
}

int hsEpollConnect(HsEpoll *ep, const char *selfIp, int selfPort, const char *ip, int port)
{
	int fd;
	struct HS_SOCK_SOCKADDR_IN serverAddr = { 0 };
	struct HS_SOCK_SOCKADDR_IN clientAddr = { 0 };
	int fail = 0;
	int optval = 1;

#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	fd = HS_SOCK_SOCKET(HS_SOCK_AF_INET, HS_SOCK_SOCK_STREAM, HS_SOCK_IPPROTO_TCP);

	clientAddr.sin_addr.s_addr = inet_addr(ip);
	clientAddr.sin_family = HS_SOCK_AF_INET;
	clientAddr.sin_port = htons(port);
	fail = HS_SOCK_CONNECT(fd, (struct HS_SOCK_SOCKADDR *)&clientAddr, sizeof(struct HS_SOCK_SOCKADDR));
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
	fd = ofp_socket(OFP_AF_INET, OFP_SOCK_STREAM,
				OFP_IPPROTO_TCP);
	if (fd == HS_INVALID_SOCK) 
	{
		HS_DBG(("Faild to create SEND socket (errno = %d)\n", ofp_errno));
		return HS_INVALID_SOCK;
	}

	optval = 1;
	ofp_setsockopt(fd, OFP_SOL_SOCKET, OFP_SO_REUSEADDR,
		&optval, sizeof(optval));
	ofp_setsockopt(fd, OFP_SOL_SOCKET, OFP_SO_REUSEPORT,
		&optval, sizeof(optval));

	serverAddr.sin_len = sizeof(struct ofp_sockaddr_in);
	serverAddr.sin_family = OFP_AF_INET;
	serverAddr.sin_port = odp_cpu_to_be_16(10000 + port);
	serverAddr.sin_addr.s_addr = inet_addr(selfIp);

	while (1)
	{
		if (ofp_bind(fd, (const struct ofp_sockaddr *)&serverAddr, sizeof(struct ofp_sockaddr_in)) == -1)
		{
			HS_DBG(("Faild to bind connection socket at %d (errno = %d)\n", 10000 + port, ofp_errno));
			whSleep(1);			
		}
		else
			break;
	}

	clientAddr.sin_addr.s_addr = inet_addr(ip);
	clientAddr.sin_family = HS_SOCK_AF_INET;
	clientAddr.sin_port = htons(port);
//	clientAddr.sin_port = odp_cpu_to_be_16(port);
	clientAddr.sin_len = sizeof(clientAddr);
	if (HS_SOCK_CONNECT(fd, (struct HS_SOCK_SOCKADDR *)&clientAddr, sizeof(struct HS_SOCK_SOCKADDR)) == -1 && ofp_errno != OFP_EINPROGRESS)
		fail = 1;
#endif

	if (fail)
	{
		HS_DBG(("%s connect [%s:%d] failed.\n", HS_FUNC, ip, port));
		HS_SOCK_CLOSE(fd);
		return HS_INVALID_SOCK;
	}
	HS_DBG(("connect %s:%d ok, socket id %d.\n", ip, port, fd));

	setNonBlocking(fd);

	hsEpollAddSock(ep, fd);

	return fd;
}
