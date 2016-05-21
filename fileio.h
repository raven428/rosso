/*
  This file contains file io functions for UNIX/Linux
*/

#ifndef __fileio_h__
#define __fileio_h__

#include <stdio.h>
#include <sys/types.h>
#include "platform.h"

int fs_seek(FILE *stream, off_t offset, int whence);
off_t fs_read(void *ptr, u_int32_t size, u_int32_t n, FILE *stream);
off_t fs_write(const void *ptr, u_int32_t size, u_int32_t n, FILE *stream);
int fs_close(FILE* file);

#endif  // __fileio_h__
