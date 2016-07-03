/*
 * This file contains/describes functions for error handling and messaging.
 */

#ifndef __errors_h__
#define __errors_h__

// macros
#define myerror(...) errormsg(__func__, __VA_ARGS__);

// error messages with function name and argument list
void errormsg(const char *func, const char *str, ...);
void stderror();

#endif // __errors_h__
