/*
 * This file contains/describes functions to manage string lists.
 */

#ifndef __stringlist_h__
#define __stringlist_h__

struct sStringList {
  char *str;
  struct sStringList *next;
};

// defines return values for function matchesStringList
#define RETURN_NO_MATCH 0
#define RETURN_EXACT_MATCH 1
#define RETURN_SUB_MATCH 2

// create a new string list
struct sStringList *newStringList();

// insert new directory path into directory path list
int addStringToStringList(struct sStringList *stringList, const char *str);

// evaluates whether str is contained in strList
int matchesStringList(struct sStringList *stringList, const char *str);

// evaluate whether str matches the include an exclude dir path lists or not
int matchesStringLists(struct sStringList *includes,
  struct sStringList *includes_recursion, struct sStringList *excludes,
  struct sStringList *excludes_recursion, const char *str);

// free string list
void freeStringList(struct sStringList *stringList);

#endif // __stringlist_h__
