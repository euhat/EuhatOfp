#include "HsHash.h"
#include "HsCommon.h"
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_errno.h>

int hsHashInit(HsHash *hash, const char *name, int size)
{
	struct rte_hash_parameters params;
	
	hash->name = HS_STRDUP(name);

	HS_MEMSET(&params, 0, sizeof(params));
	params.name = name;
	params.entries = getUpperPower2(size);
	params.key_len = sizeof (int);
	params.hash_func = rte_jhash;
	params.hash_func_init_val = 0;

	hash->hash = rte_hash_create(&params);
	if (NULL == hash->hash)
	{
		HS_DBG(("%s %s [%d:%s] failed.\n", HS_FUNC, name, rte_errno, rte_strerror(rte_errno)));
		return 0;
	}
	HS_DBG(("%s %s succeeded.\n", HS_FUNC, name));
	return 1;
}

int hsHashFini(HsHash *hash)
{
	rte_hash_free(hash->hash);
	HS_FREE(hash->name);
	return 1;
}

int hsHashAdd(HsHash *hash, int key, void *value)
{
	void *oldValue;
	int ret;
	oldValue = hsHashFind(hash, key);
	if (NULL != oldValue)
	{
		HS_DBG(("%s %s old value is not null.\n", HS_FUNC, hash->name));
		HS_FREE(oldValue);
	}
	ret = rte_hash_add_key_data(hash->hash, &key, value);
	if (ret < 0)
		HS_DBG(("%s %s %d add failed.\n", HS_FUNC, hash->name, ret));
	return ret >= 0;
}

int hsHashDel(HsHash *hash, int key)
{
	int ret;
	ret = rte_hash_del_key(hash->hash, &key);
	if (ret < 0)
	{
		HS_DBG(("%s %s %d failed.\n", HS_FUNC, hash->name, ret));
		return 0;
	}
/*	ret = rte_hash_free_key_with_position(hash->hash, ret);
	if (ret < 0)
	{
		HS_DBG(("%s %s %d add failed.\n", HS_FUNC, hash->name, ret));
	}
*/	return 1;
}

void *hsHashFind(HsHash *hash, int key)
{
	void *value;
	int ret;
	ret = rte_hash_lookup_data(hash->hash, &key, &value);
	if (ret < 0)
	{
/*		HS_DBG(("%s %s %d failed.\n", HS_FUNC, hash->name, ret));
*/		return NULL;
	}
	return value;
}

void hsHashScan(HsHash *hash, HsHashIterate it, void *user)
{
	uint32_t iter = 0;
    const void *nextKey;
    void *nextData;

	int pos = 0;
    while ((pos = rte_hash_iterate(hash->hash, &nextKey, &nextData, &iter)) >= 0)
    {
		it(user, hash, nextData);
	}
}