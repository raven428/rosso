/*
 * This file contains file io functions
 */

#ifndef __fileio_h__
#define __fileio_h__

#include <stdio.h>

FILE *fs_open(char *path, char *mode);
int fs_seek(FILE *stream, long offset, int whence);
size_t fs_read(void *ptr, size_t size, size_t n, FILE *stream);
size_t fs_write(const void *ptr, size_t size, size_t n, FILE *stream);
int fs_close(FILE *file);

#endif // __fileio_h__
