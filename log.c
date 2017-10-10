/* Copyright (C) Teemu Suutari */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void DebugPrint(const char *str,...)
{
	va_list ap;
	va_start(ap,str);
	vfprintf(stdout,str,ap);
	fflush(stdout);
	va_end(ap);
}

void DebugPrintAndDie(const char *str,...)
{
	va_list ap;
	va_start(ap,str);
	vfprintf(stderr,str,ap);
	fflush(stderr);
	va_end(ap);
	abort();
}
