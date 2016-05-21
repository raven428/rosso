/*
  This file contains/describes functions for natural order sorting.
*/

#ifndef __natstrcmp_h__
#define __natstrcmp_h__

#include <sys/types.h>

// natural order comparison
int32_t natstrcmp(const char *str1, const char *str2);

// natural order comparison ignoring case
int32_t natstrcasecmp(const char *str1, const char *str2);

#endif // __natstrcmp_h__
