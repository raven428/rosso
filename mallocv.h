/*
  This file contains/describes debug versions for malloc and free
*/

#ifndef __mallocv_h__
#define __mallocv_h__

#include <stdlib.h>

#if DEBUG >= 2
#define malloc(size) mallocv(__FILE__, __LINE__, size)
#define realloc(ptr, size) reallocv(__FILE__, __LINE__, ptr, size)
#define free(ptr) freev(__FILE__, __LINE__, ptr)
#define REPORT_MEMORY_LEAKS reportLeaks();
void *mallocv(char *filename, u_int32_t line, size_t size);
void *reallocv(char *filename, u_int32_t line, void *ptr, size_t size);
void freev(char *filename, u_int32_t line, void *ptr);
void reportLeaks();
#else
#define REPORT_MEMORY_LEAKS
#endif

#endif // __mallocv_h__
