#ifndef _HS_COMMON_H_
#define _HS_COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void hsLog(const char *format, ...);
#define HS_DBG(_params) hsLog _params

#define HS_GET_PARENT(_address, _type, _field) \
	((_type *)((char *)(_address) - (int)(&((_type *)0)->_field)))

#define HS_MALLOC malloc
#define HS_FREE free
#define HS_MEMMOVE memmove
#define HS_MEMCPY memcpy
#define HS_MEMSET memset
#define HS_STRCPY strcpy
#define HS_STRLEN strlen
#define HS_STRDUP strdup
#define HS_STREQU(_l, _r) (strcmp(_l, _r) == 0)
#define HS_ATOI atoi
#define HS_TIME_T time_t
#define HS_TIME_NOW time(NULL)
#define HS_ASSERT(_b) \
do { \
	if (!(_b)) \
		HS_DBG(("[%s] false at %s:%d.\n", #_b, HS_FUNC, HS_LINE)); \
} while (0)

#define HS_FILE __FILE__
#define HS_FUNC __FUNCTION__
#define HS_LINE __LINE__
#define HS_INVALID_SOCK -1

#define HS_RING_SIZE_CROSS_THREAD 1024
#define HS_RING_SIZE_CROSS_PACKET 1024 * 10
#define HS_LISTENING_PORT 8024

#define HS_MSG_TYPE_REGISTER	0

int hsMkDir(const char *path);

int getUpperPower2(int num);

#endif
