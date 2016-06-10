/*
  This file contains/describes miscellaneous functions.
*/

#include "misc.h"

#include <stdarg.h>
#include <stdio.h>
#include "options.h"

void infomsg(char *str, ...) {
/*
  info messages that can be muted with a command line option
*/
  va_list argptr;

  if (!OPT_QUIET) {
    va_start(argptr,str);
    vprintf(str,argptr);
    va_end(argptr);
  }

}
