/*
  This file contains file io functions for UNIX/Linux
*/

#ifndef __fileio_h__
#define __fileio_h__

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <windows.h>

FILE* fs_open(char* path, uint32_t mode);
int fs_seek(FILE *stream, off_t offset, int whence);
off_t fs_read(void *ptr, uint32_t size, uint32_t n, FILE *stream);
off_t fs_write(const void *ptr, uint32_t size, uint32_t n, FILE *stream);
int fs_close(FILE* file);

#endif  // __fileio_h__
