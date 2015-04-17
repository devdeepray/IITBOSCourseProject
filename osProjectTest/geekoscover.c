#include "geekoscover.h"

#include <stdarg.h>
int Open(const char* fname, int perm, File** f)
{
	*f = fopen(fname, "r+");
	return (*f == 0);
}

int Read(File* f, char* buf, int nbytes)
{
	
	return fread(buf, 1, nbytes, f);
}

int Write(File* f, char* buf, int nbytes)
{
	int nwritten = fwrite(buf, 1, nbytes, f);
	return nwritten;
}

int Seek(File* f, int offset)
{
	return fseek(f, offset, SEEK_SET);
}

void* Malloc(int x)
{
	return  malloc(x);
}

int Print(char* fmt, ...)
{
	va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fflush(stdout);
    va_end(args);
    return 0;
}
