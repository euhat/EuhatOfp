#include "HsRing.h"
#include "HsCommon.h"
#include <rte_ring.h>
#include <rte_errno.h>

int hsRingInit(HsRing *ring, char *name, int size)
{
	size = getUpperPower2(size);
	
	ring->name = HS_STRDUP(name);
	ring->ring = rte_ring_create(ring->name, size, SOCKET_ID_ANY, RING_F_SC_DEQ | RING_F_SP_ENQ);
	if (NULL == ring->ring)
	{
		HS_DBG(("%s %s [%s] failed.\n", HS_FUNC, name, rte_strerror(rte_errno)));
		return 0;
	}
	HS_DBG(("%s %s succeeded.\n", HS_FUNC, name));
	return 1;
}

int hsRingFini(HsRing *ring)
{
	rte_ring_free(ring->ring);
	HS_FREE(ring->name);
	return 1;
}

int hsRingIn(HsRing *ring, void *p)
{
	int ret = rte_ring_enqueue(ring->ring, p);
	HS_DBG(("ring %s in %p.\n", ring->name, p));
	if (ret < 0)
		HS_DBG(("%s %s is full.\n", HS_FUNC, ring->name));
	return ret >= 0;
}

void *hsRingOut(HsRing *ring)
{
	void *p;
	int ret = rte_ring_dequeue(ring->ring, &p);
	HS_DBG(("ring %s out %p.\n", ring->name, p));
	if (ret < 0)
	{
		HS_DBG(("%s %s is empty.\n", HS_FUNC, ring->name));
		return NULL;
	}
	return p;
}