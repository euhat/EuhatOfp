#ifndef _HS_SPIN_MUTEX_H_
#define _HS_SPIN_MUTEX_H_

typedef struct HsSpinMutex
{
	void *spin;
} HsSpinMutex;

typedef struct HsSpinMutexRecur
{
	void *spinRecur;
} HsSpinMutexRecur;

int hsSpinMutexInit(HsSpinMutex *mutex);
int hsSpinMutexFini(HsSpinMutex *mutex);
void hsSpinMutexLock(HsSpinMutex *mutex);
void hsSpinMutexUnlock(HsSpinMutex *mutex);

int hsSpinMutexRecurInit(HsSpinMutexRecur *mutex);
int hsSpinMutexRecurFini(HsSpinMutexRecur *mutex);
void hsSpinMutexRecurLock(HsSpinMutexRecur *mutex);
void hsSpinMutexRecurUnlock(HsSpinMutexRecur *mutex);

struct rte_spinlock_recursive_t;

#endif
