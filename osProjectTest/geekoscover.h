#ifndef _GEEKOS_COVER_H_
#define _GEEKOS_COVER_H_

#include <stdio.h>
#include <stdlib.h>


#include <stdarg.h>

#define O_READ 0
#define O_WRITE 0

            

#define Close fclose

#define Free free

typedef FILE File;

int Open(const char*, int, File**);

int Read(File*, char*, int);

int Write(File*, char*, int);

int Seek(File*, int);

int Print(char* fmt, ...);

void* Malloc(int);

#endif
