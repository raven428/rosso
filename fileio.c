/*
  This file contains file io functions
*/

#include "fileio.h"
#include <stdio.h>
#include <sys/types.h>

FILE* fs_open(char* path, uint32_t mode) {
  return CreateFile(path, mode, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
    NULL);
}

int fs_seek(FILE *stream, off_t offset, int whence) {
  return SetFilePointer(stream, offset, NULL, whence);
}

off_t fs_read(void *ptr, uint32_t size, uint32_t n, FILE *stream) {
  DWORD q;
  return ReadFile(stream, ptr, size * n, &q, NULL);
}

off_t fs_write(const void *ptr, uint32_t size, uint32_t n, FILE *stream) {
  DWORD q;
  return WriteFile(stream, ptr, size * n, &q, NULL);
}

int fs_close(FILE* file) {
  return CloseHandle(file);
}

