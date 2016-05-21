/*
  This file contains/describes functions for error handling and messaging.
*/

#ifndef __errors_h__
#define __errors_h__

#include <stdarg.h>
#include <string.h>

// macros
#if DEBUG >= 1
#define DEBUGMSG(msg...) errormsg("DEBUG", msg);
#else 
#define DEBUGMSG(msg...)
#endif
#define myerror(msg...) errormsg(__func__, msg);
#define stderror() errormsg(__func__, "%s!", strerror(errno));

// error messages with function name and argument list
void errormsg(const char *func, const char *str, ...);

#endif // __errors_h__
