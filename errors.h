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
#define DEBUGMSG(...)
#endif
#define myerror(...) errormsg(__func__, __VA_ARGS__);
#define stderror() errormsg(__func__, "%s!", strerror(errno));

// error messages with function name and argument list
void errormsg(const char *func, const char *str, ...);

#endif // __errors_h__
