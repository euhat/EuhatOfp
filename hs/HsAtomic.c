#include "HsAtomic.h"
#include "HsCommon.h"
#include <rte_atomic.h>

int hsAtomic32Init(HsAtomic32 *atomic)
{
    atomic->atomic = HS_MALLOC(sizeof(rte_atomic32_t));
    rte_atomic32_init(atomic->atomic);
    return 1;
}

int hsAtomic32Fini(HsAtomic32 *atomic)
{
    HS_FREE(atomic->atomic);
    return 1;
}

int hsAtomic32Increase(HsAtomic32 *atomic)
{
    rte_atomic32_inc((rte_atomic32_t *)atomic->atomic);
    return 1;
}

int hsAtomic32Get(HsAtomic32 *atomic)
{
    return rte_atomic32_read((rte_atomic32_t *)atomic->atomic);
}