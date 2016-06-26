/*
 * This file contains/describes functions for natural order sorting.
 */

#include "natstrcmp.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int32_t isDigit(const char c) {
  /*
   * return true if c is a digit, otherwise false
   */
  if (c >= '0' && c <= '9')
    return 1;

  return 0;
}

int32_t parseNumber(char **s) {
  /*
   * parse integer in string s
   */
  int32_t value = 0;

  if (!isDigit(**s))
    return -1;

  while (isDigit(**s)) {
    value = value * 10 + **s - '0';
    (*s)++;
  }

  return value;
}

int32_t natstrcompare(const char *str1, const char *str2,
  const uint32_t respectCase) {
  /*
   * natural order string compare
   */
  assert(str1 != NULL);
  assert(str2 != NULL);

  int32_t n1 = 0, n2 = 0;
  char *s1 = (char *) str1;
  char *s2 = (char *) str2;

  while (1) {
    if (*s1 == '\0' || *s2 == '\0') {
      return strcmp(s1, s2);
    }

    // compare characters until the first digit occurs
    while (1) {
      if (isDigit(*s1) || isDigit(*s2))
        break;
      else if (*s1 == '\0' && *s2 == '\0')
        return 0;
      else if (*s2 == '\0' ||
        respectCase ? toupper(*s1) > toupper(*s2) : *s1 > *s2)
        return 1;
      else if (*s1 == '\0' ||
        respectCase ? toupper(*s1) < toupper(*s2) : *s1 < *s2)
        return -1;
      s1++;
      s2++;
    }

    // at least one of the strings has a number in it
    n1 = parseNumber(&s1);
    n2 = parseNumber(&s2);

    // one of the strings had no number
    if (n1 == -1 || n2 == -1) {
      if (*s1 == '\0' && n1 == -1)
        return -1;
      else if (*s1 == '\0' && n2 == -1)
        return '0' < *s2 ? -1 : 1;
      else if (*s2 == '\0' && n1 == -1)
        return '0' < *s1 ? 1 : -1;
      else if (*s2 == '\0' && n2 == -1)
        return 1;
      // both strings had numbers in it
    }
    else if (n1 != n2)
      return n1 > n2 ? 1 : -1;
  }
}

int32_t natstrcmp(const char *str1, const char *str2) {
  return natstrcompare(str1, str2, 0);
}

int32_t natstrcasecmp(const char *str1, const char *str2) {
  return natstrcompare(str1, str2, 1);
}
