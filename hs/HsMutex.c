#include "HsMutex.h"

int hsMutexInit(HsMutex *mutex, int type)
{
	if (type == 2)
		hsSpinMutexRecurInit(&mutex->spinRecur);
	else if (type)
		hsSpinMutexInit(&mutex->spin);
	else
		whMutexInit(&mutex->mutex);
	mutex->type = type;
	return 0;
}

int hsMutexFini(HsMutex *mutex)
{
	if (mutex->type == 2)
		hsSpinMutexRecurFini(&mutex->spinRecur);
	else if (mutex->type)
		hsSpinMutexFini(&mutex->spin);
	else
		whMutexFini(&mutex->mutex);
	return 1;
}

void hsMutexLock(HsMutex *mutex)
{
	if (mutex->type == 2)
		hsSpinMutexRecurLock(&mutex->spinRecur);
	else if (mutex->type)
		hsSpinMutexLock(&mutex->spin);
	else
		whMutexEnter(&mutex->mutex);
}

void hsMutexUnlock(HsMutex *mutex)
{
	if (mutex->type == 2)
		hsSpinMutexRecurUnlock(&mutex->spinRecur);
	else if (mutex->type)
		hsSpinMutexUnlock(&mutex->spin);
	else
		whMutexLeave(&mutex->mutex);
}
