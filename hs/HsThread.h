#ifndef _HS_THREAD_H_
#define _HS_THREAD_H_

#include <pthread.h>
#include <unistd.h>
#include "HsConfig.h"

#define whSleep(_secs) sleep(_secs)
#define WhThreadHandle pthread_t
#define WH_INVALID_THREAD 0
#define WH_THREAD_DEF(_proc, _arg) void *_proc(void *_arg)
#define WH_THREAD_PREPROCESS \
do { \
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); \
} while(0)
#define whThreadCreate(_handle, _loop, _param) pthread_create(&_handle, NULL, _loop, _param)
#define whThreadJoin(_handle) pthread_join(_handle, NULL)
#define whThreadTerminate(_handle) pthread_cancel(_handle)

struct HsEnv;
struct HsThread;
typedef int (*HsThreadFuncVoidP)(void *p);
typedef int (*HsThreadFunc)(struct HsThread *thread);

typedef struct HsThread
{
#if HS_EPOLL_TYPE == HS_SYS_EPOLL
    WhThreadHandle handle;
#elif HS_EPOLL_TYPE == HS_OFP_EPOLL
    odph_odpthread_t handle;
#endif
    HsThreadFunc func;
    void *user;
    int isRunning;
    struct HsEnv *env;
} HsThread;

int hsThreadWorkProc(HsThread *thread);
int hsThreadInit(HsThread *thread, HsThreadFunc func, void *user, struct HsEnv *env);
int hsThreadFini(HsThread *thread);
int hsThreadStart(HsThread *thread);
int hsThreadStop(HsThread *thread);


#endif