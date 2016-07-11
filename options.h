/*
 * This file contains/describes functions that parse command line options.
 */

#ifndef __options_h__
#define __options_h__

#include "FAT_fs.h"
#include <stdint.h>
struct sStringList;

extern int32_t OPT_VERSION, OPT_HELP, OPT_INFO, OPT_IGNORE_CASE, OPT_ORDER,
  OPT_LIST, OPT_REVERSE, OPT_NATURAL_SORT, OPT_RECURSIVE, OPT_RANDOM,
  OPT_MORE_INFO, OPT_MODIFICATION, OPT_ASCII;
extern struct sStringList *OPT_INCL_DIRS, *OPT_EXCL_DIRS, *OPT_INCL_DIRS_REC,
  *OPT_EXCL_DIRS_REC, *OPT_IGNORE_PREFIXES_LIST;

// parses command line options
int32_t parse_options(int argc, char *argv[]);

// evaluate whether str matches the include an exclude dir path lists or not
int32_t matchesDirPathLists(struct sStringList *includes,
  struct sStringList *includes_recursion, struct sStringList *excludes,
  struct sStringList *excludes_recursion,
  const char (*str)[MAX_PATH_LEN + 1]);

// free options
void freeOptions();

#endif // __options_h__
