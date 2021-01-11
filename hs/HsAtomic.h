#ifndef _HS_ATOMIC_H_
#define _HS_ATOMIC_H_

typedef struct HsAtomic32
{
	void *atomic;
} HsAtomic32;

int hsAtomic32Init(HsAtomic32 *atomic);
int hsAtomic32Fini(HsAtomic32 *atomic);
int hsAtomic32Increase(HsAtomic32 *atomic);
int hsAtomic32Get(HsAtomic32 *atomic);

#endif