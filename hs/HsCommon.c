#include "HsCommon.h"
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)

void hsLog(const char *format, ...)
{
	va_list argptr;
	char buf[1024 * 2];
	time_t t;
	struct tm *local;
	char timeBuf[256];
	char logPath[256];
	FILE *fp;

	va_start(argptr, format);
	vsprintf(buf, format, argptr);
	va_end(argptr);

	t = time(NULL);
	local = localtime(&t);
	strftime(timeBuf, 254, "[%Y-%m-%d %H:%M:%S] ", local);

	hsMkDir("log");
	sprintf(logPath, "log/%d.log", (unsigned int)gettid());

	fp = fopen(logPath, "a+");
	if (NULL != fp)
	{
		fprintf(fp, "%s%s", timeBuf, buf);
		fclose(fp);
	}
}

int hsMkDir(const char *path)
{
	return mkdir(path, 0755);
}

int getUpperPower2(int num)
{
	int i;
	int pattern = -1;
	int iter = 1;
	for (i = 0; i < 32; i++)
	{
		iter <<= 1;
		if (pattern & num)
			pattern ^= iter;
		else
			break;
	}
	return iter;
}