#ifndef _HS_MUTEX_H_
#define _HS_MUTEX_H_

#include <pthread.h>
#include "HsCommon.h"

typedef struct WhMutexStruct
{
	pthread_mutexattr_t attr_;
	pthread_mutex_t m_;
} WhMutexStruct;
#define WhMutexParam WhMutexStruct

#define whMutexInit(_handle) \
do { \
	if (pthread_mutexattr_init(&(_handle)->attr_) != 0) { \
		HS_DBG(("init mutex attr failed.\n")); \
		break; \
				} \
	pthread_mutexattr_settype(&(_handle)->attr_, PTHREAD_MUTEX_RECURSIVE_NP); \
	pthread_mutex_init(&(_handle)->m_, &(_handle)->attr_); \
} while (0)

#define whMutexEnter(_handle) pthread_mutex_lock(&(_handle)->m_)
#define whMutexLeave(_handle) pthread_mutex_unlock(&(_handle)->m_)

#define whMutexFini(_handle) \
do { \
	pthread_mutex_destroy(&(_handle)->m_); \
	pthread_mutexattr_destroy(&(_handle)->attr_); \
} while (0)

#include "HsSpinMutex.h"

typedef struct HsMutex
{
    WhMutexParam mutex;
	HsSpinMutex spin;
	HsSpinMutexRecur spinRecur;
	int type;
} HsMutex;

int hsMutexInit(HsMutex *mutex, int type);
int hsMutexFini(HsMutex *mutex);
void hsMutexLock(HsMutex *mutex);
void hsMutexUnlock(HsMutex *mutex);

#endif
