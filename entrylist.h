/*
 * This file contains/describes some ADOs which are used to represent the
 * structures of FAT directory entries and entry lists.
 */

#ifndef __entrylist_h__
#define __entrylist_h__

#include <stdint.h>
struct sLongDirEntry;
struct sShortDirEntry;

struct sLongDirEntryList {
  /*
   * list structures for directory entries list structure for a long name
   * entry
   */
  struct sLongDirEntry *lde;
  struct sLongDirEntryList *next;
};

struct sDirEntryList {
  /*
   * list structure for every file with short name entries and long name
   * entries
   */
  char *sname, *lname; // short and long name strings
  struct sShortDirEntry *sde; // short dir entry
  struct sLongDirEntryList *ldel; // long name entries in a list
  uint32_t entries; // number of entries
  struct sDirEntryList *next; // next dir entry
};

// create new dir entry list
struct sDirEntryList *newDirEntryList();

// randomize entry list
void randomizeDirEntryList(struct sDirEntryList *list, int32_t entries);

// create a new directory entry holder
struct sDirEntryList *newDirEntry(char *sname, char *lname,
  struct sShortDirEntry *sde, struct sLongDirEntryList *ldel,
  uint32_t entries);

// insert a long directory entry to list
struct sLongDirEntryList *insertLongDirEntryList(struct sLongDirEntry *lde,
  struct sLongDirEntryList *list);

// compare two directory entries
int32_t cmpEntries(struct sDirEntryList *de1, struct sDirEntryList *de2);

// insert a directory entry into list
void insertDirEntryList(struct sDirEntryList *q, struct sDirEntryList *list);

// free dir entry list
void freeDirEntryList(struct sDirEntryList *list);

#endif // __entrylist_h__
