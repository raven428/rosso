/*
 * This file contains/describes functions that parse command line options.
 */

#include "options.h"

#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "stringlist.h"

uint32_t OPT_VERSION, OPT_HELP, OPT_INFO, OPT_IGNORE_CASE, OPT_ORDER,
  OPT_LIST, OPT_REVERSE, OPT_NATURAL_SORT, OPT_RECURSIVE, OPT_RANDOM,
  OPT_MORE_INFO, OPT_MODIFICATION, OPT_ASCII;

struct sStringList *OPT_INCL_DIRS = 0;
struct sStringList *OPT_EXCL_DIRS = 0;
struct sStringList *OPT_INCL_DIRS_REC = 0;
struct sStringList *OPT_EXCL_DIRS_REC = 0;
struct sStringList *OPT_IGNORE_PREFIXES_LIST = 0;

int32_t addDirPathToStringList(struct sStringList *stringList,
  const char (*str)[MAX_PATH_LEN + 1]) {
  /*
   * insert new string into string list
   */

  char *newStr;

  int32_t ret, prefix = 0, suffix = 0, len;

  len = strlen((char *) str);

  // determine whether we have to add slashes
  if (((const char *) str)[0] != '/')
    prefix = 1;
  if (((const char *) str)[len - 1] != '/')
    suffix = 1;

  // allocate memory for string
  newStr = malloc(prefix + len + suffix + 1);
  if (!newStr) {
    stderror();
    return -1;
  }

  // copy string to new structure including missing slashes
  newStr[0] = 0;
  strncat(newStr, "/", prefix);
  strncat(newStr, (const char *) str, len);
  strncat(newStr, "/", suffix);

  if (prefix + len + suffix > MAX_PATH_LEN)
    newStr[MAX_PATH_LEN] = 0;
  else
    newStr[prefix + len + suffix] = 0;

  ret = addStringToStringList(stringList, newStr);

  free(newStr);

  return ret;

}

int32_t matchesDirPathLists(struct sStringList *includes,
  struct sStringList *includes_recursion, struct sStringList *excludes,
  struct sStringList *excludes_recursion,
  const char (*str)[MAX_PATH_LEN + 1]) {
  /*
   * evaluate whether str matches the include an exclude dir path lists or
   * not
   */

  int32_t incl, incl_rec, excl, excl_rec;

  incl = matchesStringList(includes, (const char *) str);
  incl_rec = matchesStringList(includes_recursion, (const char *) str);
  excl = matchesStringList(excludes, (const char *) str);
  excl_rec = matchesStringList(excludes_recursion, (const char *) str);

  // if no options -d and -D are used
  if (!includes->next && !includes_recursion->next) {
    // match all directories except those are supplied via -x
    // and those and subdirs that are supplied via -X
    if (excl != RETURN_EXACT_MATCH && excl_rec == RETURN_NO_MATCH) {
      return 1; // match
    }
    // if options -d and -D are used
  }
  else {
    /*
     * match all dirs that are supplied via -d, and all dirs and subdirs that
     * are supplied via -D, except those that excplicitly excluded via -x, or
     * those and their subdirs that are supplied via -X
     */
    if ((incl == RETURN_EXACT_MATCH || incl_rec != RETURN_NO_MATCH) &&
      excl != RETURN_EXACT_MATCH && excl_rec == RETURN_NO_MATCH) {
      return 1; // match
    }
  }

  return 0; // no match
}

int32_t parse_options(int argc, char *argv[]) {
  /*
   * parses command line options
   */

  int8_t j;

  static struct option longOpts[] = {
    // name, has_arg, flag, val
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {0, 0, 0, 0}
  };

  // no info by default
  OPT_INFO = 0;
  OPT_MORE_INFO = 0;

  /*
   * Default (1) is normal order, use -1 for reverse order.
   */
  OPT_REVERSE = 1;

  // natural sort
  OPT_NATURAL_SORT = 0;

  // random sort order
  OPT_RANDOM = 0;

  // default order (directories first)
  OPT_ORDER = 0;

  // default is case sensitive
  OPT_IGNORE_CASE = 0;

  // no version information by default
  OPT_VERSION = 0;

  // sort by last modification time
  OPT_MODIFICATION = 0;

  // sort by using locale collation order
  OPT_ASCII = 0;

  // empty string lists for inclusion and exclusion of dirs
  OPT_INCL_DIRS = newStringList();
  if (!OPT_INCL_DIRS) {
    myerror("Could not create stringList!");
    return -1;
  }
  OPT_INCL_DIRS_REC = newStringList();
  if (!OPT_INCL_DIRS_REC) {
    myerror("Could not create stringList!");
    freeOptions();
    return -1;
  }
  OPT_EXCL_DIRS = newStringList();
  if (!OPT_EXCL_DIRS) {
    myerror("Could not create stringList!");
    freeOptions();
    return -1;
  }
  OPT_EXCL_DIRS_REC = newStringList();
  if (!OPT_EXCL_DIRS_REC) {
    myerror("Could not create stringList!");
    freeOptions();
    return -1;
  }

  // empty string list for to be ignored prefixes
  OPT_IGNORE_PREFIXES_LIST = newStringList();
  if (!OPT_IGNORE_PREFIXES_LIST) {
    myerror("Could not create stringList!");
    freeOptions();
    return -1;
  }

  opterr = 0;
  while ((j =
      getopt_long(argc, argv, "imvhco:lrRnd:D:x:X:I:ta", longOpts,
        0)) != -1) {
    switch (j) {
    case 'a':
      OPT_ASCII = 1;
      break;
    case 'c':
      OPT_IGNORE_CASE = 1;
      break;
    case 'h':
      OPT_HELP = 1;
      break;
    case 'i':
      OPT_INFO = 1;
      break;
    case 'm':
      OPT_MORE_INFO = 1;
      break;
    case 'l':
      OPT_LIST = 1;
      break;
    case 'o':
      switch (optarg[0]) {
      case 'd':
        OPT_ORDER = 0;
        break;
      case 'f':
        OPT_ORDER = 1;
        break;
      case 'a':
        OPT_ORDER = 2;
        break;
      default:
        myerror("Unknown flag '%c' for option 'o'.", optarg[0]);
        myerror("Use -h for more help.");
        freeOptions();
        return -1;
      }
      break;
    case 'd':
      if (addDirPathToStringList(OPT_INCL_DIRS,
          (const char (*)[MAX_PATH_LEN + 1]) optarg)) {
        myerror("Could not add directory path to dirPathList");
        freeOptions();
        return -1;
      }
      break;
    case 'D':
      if (addDirPathToStringList(OPT_INCL_DIRS_REC,
          (const char (*)[MAX_PATH_LEN + 1]) optarg)) {
        myerror("Could not add directory path to string list");
        freeOptions();
        return -1;
      }
      break;
    case 'x':
      if (addDirPathToStringList(OPT_EXCL_DIRS,
          (const char (*)[MAX_PATH_LEN + 1]) optarg)) {
        myerror("Could not add directory path to string list");
        freeOptions();
        return -1;
      }
      break;
    case 'X':
      if (addDirPathToStringList(OPT_EXCL_DIRS_REC,
          (const char (*)[MAX_PATH_LEN + 1]) optarg)) {
        myerror("Could not add directory path to string list");
        freeOptions();
        return -1;
      }
      break;
    case 'I':
      if (addStringToStringList(OPT_IGNORE_PREFIXES_LIST, optarg)) {
        myerror("Could not add directory path to string list");
        freeOptions();
        return -1;
      }
      break;
    case 'n':
      OPT_NATURAL_SORT = 1;
      break;
    case 'r':
      OPT_REVERSE = -1;
      break;
    case 'R':
      OPT_RANDOM = 1;
      break;
    case 't':
      OPT_MODIFICATION = 1;
      break;
    case 'v':
      OPT_VERSION = 1;
      break;
    default:
      myerror("Unknown option '%c'.", optopt);
      myerror("Use -h for more help.");
      freeOptions();
      return -1;
    }
  }

  return 0;
}

void freeOptions() {
  freeStringList(OPT_INCL_DIRS);
  freeStringList(OPT_INCL_DIRS_REC);
  freeStringList(OPT_EXCL_DIRS);
  freeStringList(OPT_EXCL_DIRS_REC);
  freeStringList(OPT_IGNORE_PREFIXES_LIST);
}
