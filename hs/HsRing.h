#ifndef _HS_RING_H_
#define _HS_RING_H_

struct rte_ring;

typedef struct HsRing
{
    struct rte_ring *ring;
    char *name;
} HsRing;

int hsRingInit(HsRing *ring, char *name, int size);
int hsRingFini(HsRing *ring);
int hsRingIn(HsRing *ring, void *p);
void *hsRingOut(HsRing *ring);

#endif