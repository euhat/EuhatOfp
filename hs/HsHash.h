#ifndef _HS_HASH_H_
#define _HS_HASH_H_

struct rte_hash;

struct HsHash;
typedef void (*HsHashIterate)(void *user, struct HsHash *hash, void *value);
typedef struct HsHash
{
	struct rte_hash *hash;
	char *name;
} HsHash;

int hsHashInit(HsHash *hash, const char *name, int size);
int hsHashFini(HsHash *hash);

int hsHashAdd(HsHash *hash, int key, void *value);
int hsHashDel(HsHash *hash, int key);
void *hsHashFind(HsHash *hash, int key);
void hsHashScan(HsHash *hash, HsHashIterate it, void *user);

#endif
