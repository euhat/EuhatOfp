#include "HsThread.h"
#include "HsCommon.h"
#include "HsEpoll.h"

static WH_THREAD_DEF(workThreadCb, arg)
{
    HsThread *thread;
	WH_THREAD_PREPROCESS;

	thread = (HsThread *)arg;
    hsThreadWorkProc(thread);

	return 0;
}

int hsThreadWorkProc(HsThread *thread)
{
#if HS_EPOLL_TYPE == HS_OFP_EPOLL
    hsOfpThreadPrepare(thread);
#endif

    thread->func(thread);
    return 1;
}

int hsThreadInit(HsThread *thread, HsThreadFunc func, void *user, struct HsEnv *env)
{
    thread->func = func;
    thread->user = user;
    thread->isRunning = 0;
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
    thread->handle = WH_INVALID_THREAD;
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
    HS_MEMSET(&thread->handle, 0, sizeof(thread->handle));
#endif
    thread->env = env;
    return 1;
}

int hsThreadFini(HsThread *thread)
{
    hsThreadStop(thread);
    return 1;
}

int hsThreadStart(HsThread *thread)
{
	thread->isRunning = 1;

#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	whThreadCreate(thread->handle, workThreadCb, thread);
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
    hsOfpThreadStart(thread, (HsThreadFuncVoidP)hsThreadWorkProc);
#endif
    return 1;
}

int hsThreadStop(HsThread *thread)
{
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
	if (thread->handle == WH_INVALID_THREAD)
		return 0;

	thread->isRunning = 0;
	whThreadJoin(thread->handle);
	thread->handle = WH_INVALID_THREAD;
    return 1;
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
    hsOfpThreadStop(thread);
    return 1;
#endif
}