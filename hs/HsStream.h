#ifndef _HS_STREAM_H_
#define _HS_STREAM_H_

typedef struct HsStream
{
	void *p;
	int leftLen;
} HsStream;

int hsStreamInit(HsStream *stream, char *p, int leftLen);
int hsStreamFini(HsStream *stream);

int hsStreamWriteInt8(HsStream *stream, int ch);
int hsStreamWriteInt16(HsStream *stream, int sh);
int hsStreamWriteInt32(HsStream *stream, int i);
int hsStreamWriteBuf(HsStream *stream, char *buf, int bufLen);
int hsStreamWriteStr(HsStream *stream, char *str);

int hsStreamReadInt8(HsStream *stream);
int hsStreamReadInt16(HsStream *stream);
int hsStreamReadInt32(HsStream *stream);
int hsStreamReadBuf(HsStream *stream, char *buf, int bufLen);
char *hsStreamReadStr(HsStream *stream);

#endif