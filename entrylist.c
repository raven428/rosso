/*
 * This file contains/describes some ADOs which are used to represent the
 * structures of FAT32 directory entries and entry lists.
 */

#include <stdlib.h>
#include <string.h>

#include "entrylist.h"
#include "errors.h"
#include "FAT32.h"
#include "natstrcmp.h"
#include "options.h"
#include "stringlist.h"

// random number
int irand(int xr, int ya) {
  int zu = ya - xr + 1;
  return xr + zu * rand() / (RAND_MAX + 1);
}

// List functions

struct sDirEntryList *newDirEntryList() {
  /*
   * create new dir entry list
   */
  struct sDirEntryList *tmp;

  tmp = malloc(sizeof(struct sDirEntryList));
  if (!tmp) {
    stderror();
    return 0;
  }
  memset(tmp, 0, sizeof(struct sDirEntryList));
  return tmp;
}

struct sDirEntryList *newDirEntry(char *sname, char *lname,
  struct sShortDirEntry *sde, struct sLongDirEntryList *ldel,
  unsigned entries) {
  /*
   * create a new directory entry holder
   */
  struct sDirEntryList *tmp;

  tmp = malloc(sizeof(struct sDirEntryList));
  if (!tmp) {
    stderror();
    return 0;
  }
  tmp->sname = malloc(strlen(sname) + 1);
  if (!tmp->sname) {
    stderror();
    free(tmp);
    return 0;
  }
  strcpy(tmp->sname, sname);
  tmp->lname = malloc(strlen(lname) + 1);
  if (!tmp->lname) {
    stderror();
    free(tmp->sname);
    free(tmp);
    return 0;
  }
  strcpy(tmp->lname, lname);

  tmp->sde = malloc(sizeof(struct sShortDirEntry));
  if (!tmp->sde) {
    stderror();
    free(tmp->lname);
    free(tmp->sname);
    free(tmp);
    return 0;
  }
  memcpy(tmp->sde, sde, DIR_ENTRY_SIZE);
  tmp->ldel = ldel;
  tmp->entries = entries;
  tmp->next = 0;
  return tmp;
}

struct sLongDirEntryList *insertLongDirEntryList(struct sLongDirEntry *lde,
  struct sLongDirEntryList *list) {
  /*
   * insert a long directory entry to list
   */

  struct sLongDirEntryList *tmp, *nw;

  nw = malloc(sizeof(struct sLongDirEntryList));
  if (!nw) {
    stderror();
    return 0;
  }
  nw->lde = malloc(sizeof(struct sLongDirEntry));
  if (!nw->lde) {
    stderror();
    free(nw);
    return 0;
  }
  memcpy(nw->lde, lde, DIR_ENTRY_SIZE);
  nw->next = 0;

  if (list) {
    tmp = list;
    while (tmp->next)
      tmp = tmp->next;
    tmp->next = nw;
    return list;
  }
  else {
    return nw;
  }
}

int stripSpecialPrefixes(char *old, char *nw) {
  /*
   * strip special prefixes "a" and "the"
   */
  struct sStringList *prefix = OPT_IGNORE_PREFIXES_LIST;

  size_t len, len_old;

  len_old = strlen(old);

  while (prefix->next) {
    len = strlen(prefix->next->str);
    if (!strncasecmp(old, prefix->next->str, len)) {
      strncpy(nw, old + len, len_old - len);
      nw[len_old - len] = 0;
      return 1;
    }
    prefix = prefix->next;
  }

  return 0;
}

int cmpEntries(struct sDirEntryList *de1, struct sDirEntryList *de2) {
  /*
   * compare two directory entries
   */

  char s1[MAX_PATH_LEN + 1];
  char s2[MAX_PATH_LEN + 1];
  char s1_col[MAX_PATH_LEN * 2 + 1];
  char s2_col[MAX_PATH_LEN * 2 + 1];

  // the volume label must always remain at the beginning of the (root)
  // directory
  if ((de1->sde->DIR_Atrr &
      (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID |
        ATTR_DIRECTORY)) == ATTR_VOLUME_ID) {
    return -1;
  }
  if ((de2->sde->DIR_Atrr &
      (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID |
        ATTR_DIRECTORY)) == ATTR_VOLUME_ID) {
    return 1;
    // the special "." and ".." directories must always remain at the
    // beginning of directories, in this order
  }
  if (!strcmp(de1->sname, "."))
    return -1;
  if (!strcmp(de2->sname, "."))
    return 1;
  if (!strcmp(de1->sname, ".."))
    return -1;
  if (!strcmp(de2->sname, "..")) {
    return 1;
    // deleted entries should be moved to the end of the directory
  }
  if ((de1->sname[0] & 0xFF) == DE_FREE)
    return 1;
  if ((de2->sname[0] & 0xFF) == DE_FREE)
    return -1;

  char *ss1, *ss2;

  if (de1->lname && de1->lname[0])
    ss1 = de1->lname;
  else
    ss1 = de1->sname;
  if (de2->lname && de2->lname[0])
    ss2 = de2->lname;
  else
    ss2 = de2->sname;

  // it's not necessary to compare files for listing and randomization,
  // each entry will be put to the end of the list
  if (OPT_LIST || OPT_RANDOM)
    return 1;

  // directories will be put above normal files
  if (!OPT_ORDER) {
    if (de1->sde->DIR_Atrr & ATTR_DIRECTORY && de2->sde->DIR_Atrr &
      ~ATTR_DIRECTORY) {
      return -1;
    }
    if (de1->sde->DIR_Atrr & ~ATTR_DIRECTORY && de2->sde->DIR_Atrr &
      ATTR_DIRECTORY) {
      return 1;
    }
  }
  else if (OPT_ORDER == 1) {
    if (de1->sde->DIR_Atrr & ATTR_DIRECTORY && de2->sde->DIR_Atrr &
      ~ATTR_DIRECTORY) {
      return 1;
    }
    if (de1->sde->DIR_Atrr & ~ATTR_DIRECTORY && de2->sde->DIR_Atrr &
      ATTR_DIRECTORY) {
      return -1;
    }
  }

  // consider last modification time
  if (OPT_MODIFICATION) {
    int md1, md2;
    md1 = de1->sde->DIR_WrtDate << 16 | de1->sde->DIR_WrtTime;
    md2 = de2->sde->DIR_WrtDate << 16 | de2->sde->DIR_WrtTime;
    if (md1 < md2)
      return -OPT_REVERSE;
    if (md1 > md2)
      return OPT_REVERSE;
    return 0;
  }

  // strip special prefixes
  if (OPT_IGNORE_PREFIXES_LIST->next) {
    if (stripSpecialPrefixes(ss1, s1))
      ss1 = s1;
    if (stripSpecialPrefixes(ss2, s2)) {
      ss2 = s2;
    }
  }

  if (!OPT_ASCII) {
    // consider locale for comparison
    if (strxfrm(s1_col, ss1, MAX_PATH_LEN * 2) == MAX_PATH_LEN * 2 ||
      strxfrm(s2_col, ss2, MAX_PATH_LEN * 2) == MAX_PATH_LEN * 2) {
      myerror("String collation error!");
      exit(1);
    }
  }

  if (OPT_NATURAL_SORT) {
    if (OPT_IGNORE_CASE)
      return natstrcasecmp(ss1, ss2) * OPT_REVERSE;
    return natstrcmp(ss1, ss2) * OPT_REVERSE;
  }
  if (OPT_ASCII) {
    // use plain ASCII corder
    if (OPT_IGNORE_CASE)
      return strcasecmp(ss1, ss2) * OPT_REVERSE;
    return strcmp(ss1, ss2) * OPT_REVERSE;
  }
  if (OPT_IGNORE_CASE)
    return strcasecmp(s1_col, s2_col) * OPT_REVERSE;
  return strcmp(s1_col, s2_col) * OPT_REVERSE;
}

void insertDirEntryList(struct sDirEntryList *nw, struct sDirEntryList *list) {
  /*
   * insert a directory entry into list
   */

  struct sDirEntryList *tmp, *dummy;

  tmp = list;

  while (tmp->next && cmpEntries(nw, tmp->next) >= 0)
    tmp = tmp->next;

  dummy = tmp->next;
  tmp->next = nw;
  nw->next = dummy;
}

void freeDirEntryList(struct sDirEntryList *list) {
  /*
   * free dir entry list
   */
  struct sDirEntryList *tmp;
  struct sLongDirEntryList *ldelist, *tmp2;

  while (list) {
    if (list->sname)
      free(list->sname);
    if (list->lname)
      free(list->lname);
    if (list->sde)
      free(list->sde);

    ldelist = list->ldel;
    while (ldelist) {
      free(ldelist->lde);
      tmp2 = ldelist;
      ldelist = ldelist->next;
      free(tmp2);
    }

    tmp = list;
    list = list->next;
    free(tmp);
  }
}

void randomizeDirEntryList(struct sDirEntryList *list, int entries) {
  /*
   * randomize entry list
   */
  struct sDirEntryList *randlist, *tmp, *dummy1, *dummy2;
  int i, j, pos;
  int skip = 0;

  randlist = list;

  // the volume label must always remain at the beginning of the (root)
  // directory
  // the special "." and ".." directories must always remain at the beginning
  // of directories, so skip them
  while (randlist->next && ((randlist->next->sde->DIR_Atrr &
        (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID |
          ATTR_DIRECTORY)) == ATTR_VOLUME_ID ||
      !strcmp(randlist->next->sname, ".") ||
      !strcmp(randlist->next->sname, ".."))) {

    randlist = randlist->next;
    skip++;
  }

  for (i = skip; i < entries; i++) {
    pos = irand(0, entries - 1 - i);

    tmp = randlist;
    // after the loop tmp->next is the selected item
    for (j = 0; j < pos; j++)
      tmp = tmp->next;

    // put selected entry to top of list
    dummy1 = tmp->next;
    tmp->next = dummy1->next;

    dummy2 = randlist->next;
    randlist->next = dummy1;
    dummy1->next = dummy2;

    randlist = randlist->next;
  }
}
