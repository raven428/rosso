/*
  This file contains file io functions for UNIX/Linux
*/

#include "fileio.h"
#include <stdio.h>
#include <sys/types.h>

int fs_seek(FILE *stream, off_t offset, int whence) {
  return fseeko(stream, offset, whence);
}

off_t fs_read(void *ptr, uint32_t size, uint32_t n, FILE *stream) {
  return fread(ptr, size, n, stream);
}

off_t fs_write(const void *ptr, uint32_t size, uint32_t n, FILE *stream) {
  return fwrite(ptr, size, n, stream);
}

int fs_close(FILE* file) {
  return fclose(file);
}

