/*
 * This file contains/describes functions for natural order sorting.
 */

#include "natstrcmp.h"

#include <ctype.h>
#include <string.h>

int parseNumber(char **q) {
  /*
   * parse integer in string q
   */
  int value = 0;

  if (!isdigit(**q))
    return -1;

  while (isdigit(**q)) {
    value = value * 10 + **q - '0';
    (*q)++;
  }

  return value;
}

int natstrcompare(const char *str1, const char *str2,
  const unsigned respectCase) {
  /*
   * natural order string compare
   */

  int n1 = 0, n2 = 0;
  char *s1 = (char *) str1;
  char *s2 = (char *) str2;

  while (1) {
    if (!*s1 || !*s2)
      return strcmp(s1, s2);

    // compare characters until the first digit occurs
    while (1) {
      if (isdigit(*s1) || isdigit(*s2))
        break;
      if (!*s1 && !*s2)
        return 0;
      if (!*s2 || respectCase ? toupper(*s1) > toupper(*s2) : *s1 > *s2)
        return 1;
      if (!*s1 || respectCase ? toupper(*s1) < toupper(*s2) : *s1 < *s2)
        return -1;
      s1++;
      s2++;
    }

    // at least one of the strings has a number in it
    n1 = parseNumber(&s1);
    n2 = parseNumber(&s2);

    // one of the strings had no number
    if (n1 == -1 || n2 == -1) {
      if (!*s1 && n1 == -1)
        return -1;
      if (!*s1 && n2 == -1)
        return '0' < *s2 ? -1 : 1;
      if (!*s2 && n1 == -1)
        return '0' < *s1 ? 1 : -1;
      if (!*s2 && n2 == -1) {
        return 1;
      }
    }
    // both strings had numbers in it
    else if (n1 != n2) {
      return n1 > n2 ? 1 : -1;
    }
  }
}

int natstrcmp(const char *str1, const char *str2) {
  return natstrcompare(str1, str2, 0);
}

int natstrcasecmp(const char *str1, const char *str2) {
  return natstrcompare(str1, str2, 1);
}
