/*
 * This file contains/describes functions for error handling and messaging.
 */

#include <_mingw_stdarg.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"

void errormsg(const char *func, const char *str, ...) {
  /*
   * error messages with function name and argument list
   */
  char msg[129];
  va_list argptr;

  va_start(argptr, str);
  vsnprintf(msg, 128, str, argptr);
  fprintf(stderr, "%s: %s\n", func, msg);
  va_end(argptr);

}
void stderror() {
  errormsg(__func__, "%s!", strerror(errno));
}
