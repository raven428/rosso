/*
  This file contains/describes functions to manage string lists.
*/
#include "stringlist.h"

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "errors.h"

struct sStringList *newStringList() {
/*
  create a new string list
*/
  struct sStringList *stringList;

  // create the dummy head element
  stringList = malloc(sizeof(struct sStringList));
  if (stringList == NULL) {
    stderror();
    return NULL;
  }  
  stringList->str = NULL;
  stringList->next = NULL;

  return stringList;
}

int32_t addStringToStringList(struct sStringList *stringList, const char *str) {
/*
  insert new string into string list
*/
  assert(stringList != NULL);
  assert(stringList->str == NULL);
  assert(str != NULL);
  
  int32_t len;
  
  // find end of list
  while (stringList->next != NULL) {
    stringList = stringList->next;
  }

  // allocate memory for new entry
  stringList->next=malloc(sizeof(struct sStringList));
  if (stringList->next == NULL) {
    stderror();
    return -1;
  }
  stringList->next->next = NULL;
  
  len=strlen(str);
  
  // allocate memory for string
  stringList->next->str=malloc(len+1);
  if (stringList->next->str == NULL) {
    stderror();
    return -1;
  }

  strncpy(stringList->next->str, str, len);
  stringList->next->str[len] = '\0';

  return 0;
  
}

int32_t matchesStringList(struct sStringList *stringList, const char *str) {
/*
  evaluates whether str is contained in stringList
*/

  assert(stringList != NULL);
  assert(stringList->str == NULL);
  assert(str != NULL);

  int32_t ret=0; // not in list

  stringList=stringList->next;
  while (stringList != NULL) {
    if (strncmp(stringList->str, str, strlen(stringList->str)) == 0) {
      // contains a top level string of str
      ret=RETURN_SUB_MATCH;
    }
    if (strcmp(stringList->str, str) == 0) {
      // contains str exactly, so return immediately
      return RETURN_EXACT_MATCH;
    }
    stringList = stringList->next;
  }

  return ret;
}

void freeStringList(struct sStringList *stringList) {
/*
  free directory list
*/

  assert(stringList != NULL);

  struct sStringList *tmp;

  while (stringList != NULL) {
    if (stringList->str) free(stringList->str);
    tmp=stringList;
    stringList=stringList->next;
    free(tmp);
  }

}

