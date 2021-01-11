#include "HsSpinMutex.h"
#include "HsCommon.h"
#include <rte_spinlock.h>

int hsSpinMutexInit(HsSpinMutex *mutex)
{
	mutex->spin = HS_MALLOC(sizeof(rte_spinlock_t));
	rte_spinlock_init((rte_spinlock_t *)mutex->spin);
	return 0;
}

int hsSpinMutexFini(HsSpinMutex *mutex)
{
	HS_FREE(mutex->spin);
	return 0;
}

void hsSpinMutexLock(HsSpinMutex *mutex)
{
	rte_spinlock_lock((rte_spinlock_t *)mutex->spin);
}

void hsSpinMutexUnlock(HsSpinMutex *mutex)
{
	rte_spinlock_unlock((rte_spinlock_t *)mutex->spin);
}

int hsSpinMutexRecurInit(HsSpinMutexRecur *mutex)
{
	mutex->spinRecur = HS_MALLOC(sizeof(rte_spinlock_recursive_t));
	rte_spinlock_recursive_init((rte_spinlock_recursive_t *)mutex->spinRecur);
	return 0;
}

int hsSpinMutexRecurFini(HsSpinMutexRecur *mutex)
{
	HS_FREE(mutex->spinRecur);
}

void hsSpinMutexRecurLock(HsSpinMutexRecur *mutex)
{
	rte_spinlock_recursive_lock((rte_spinlock_recursive_t *)mutex->spinRecur);
}

void hsSpinMutexRecurUnlock(HsSpinMutexRecur *mutex)
{
	rte_spinlock_recursive_unlock((rte_spinlock_recursive_t *)mutex->spinRecur);
}