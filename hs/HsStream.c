#include "HsStream.h"
#include "HsCommon.h"

int hsStreamInit(HsStream *stream, char *p, int leftLen)
{
    stream->p = p;
    stream->leftLen = leftLen;
    return 1;
}

int hsStreamFini(HsStream *stream)
{
    return 1;
}

int hsStreamWriteInt8(HsStream *stream, int ch)
{
    char *p;
    HS_ASSERT(stream->leftLen >= 1);

    p = (char *)stream->p;
    *p = (char)ch;
    stream->p++;
    stream->leftLen--;
    return 1;
}

int hsStreamWriteInt16(HsStream *stream, int sh)
{
    short *p;

    HS_ASSERT(stream->leftLen >= sizeof(short));

    p = (short *)stream->p;
    *p = (short)sh;
    stream->p += sizeof(short);
    stream->leftLen -= sizeof(short);
    return 1;
}

int hsStreamWriteInt32(HsStream *stream, int i)
{
    int *p;

    HS_ASSERT(stream->leftLen >= sizeof(int));

    p = (int *)stream->p;
    *p = i;
    stream->p += sizeof(int);
    stream->leftLen -= sizeof(int);
    return 1;
}

int hsStreamWriteBuf(HsStream *stream, char *buf, int bufLen)
{
    HS_ASSERT(stream->leftLen >= bufLen);

    HS_MEMCPY(stream->p, buf, bufLen);
    stream->p += bufLen;
    stream->leftLen -= bufLen;
    return 1;
}

int hsStreamWriteStr(HsStream *stream, char *str)
{
    int len = HS_STRLEN(str) + 1;
    hsStreamWriteInt16(stream, len);
    hsStreamWriteBuf(stream, str, len);
    return 1;
}

int hsStreamReadInt8(HsStream *stream)
{
    char *p;
    int ch;

    HS_ASSERT(stream->leftLen >= 1);

    p = (char *)stream->p;
    ch = (int)*p;
    stream->p++;
    stream->leftLen--;
    return ch;
}

int hsStreamReadInt16(HsStream *stream)
{
    short *p;
    int sh;

    HS_ASSERT(stream->leftLen >= sizeof(short));

    p = (short *)stream->p;
    sh = (int)*p;
    stream->p += sizeof(short);
    stream->leftLen -= sizeof(short);
    return sh;
}

int hsStreamReadInt32(HsStream *stream)
{
    int *p;
    int i;

    HS_ASSERT(stream->leftLen >= sizeof(int));

    p = (int *)stream->p;
    i = *p;
    stream->p += sizeof(int);
    stream->leftLen -= sizeof(int);
    return i;
}

int hsStreamReadBuf(HsStream *stream, char *buf, int bufLen)
{
    HS_ASSERT(stream->leftLen >= bufLen);

    HS_MEMCPY(buf, stream->p, bufLen);
    stream->p += bufLen;
    stream->leftLen -= bufLen;
    return bufLen;
}

char *hsStreamReadStr(HsStream *stream)
{
    char *buf;
    int bufLen = hsStreamReadInt16(stream);

    HS_ASSERT(stream->leftLen >= bufLen);

    buf = stream->p;
    stream->p += bufLen;
    stream->leftLen -= bufLen;
    return buf;
}