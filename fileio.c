/*
 * This file contains file io functions
 */

#include "fileio.h"
#include <fileapi.h>
#include <handleapi.h>
#include <winnt.h>

FILE *fs_open(char *path, char *mode) {
  char q[PATH_MAX + 1] = {0};
  strcat(q, "\\\\.\\");
  strcat(q, path);
  return CreateFile(q, strcmp(mode,
      "r+b") ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, 0, 0,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

int fs_seek(FILE *stream, long offset, int whence) {
  return (int) SetFilePointer(stream, offset, 0, (unsigned) whence);
}

size_t fs_read(void *ptr, size_t size, size_t j, FILE *stream) {
  DWORD q;
  return (size_t) ReadFile(stream, ptr, (unsigned) (size * j), &q, 0);
}

size_t fs_write(const void *ptr, size_t size, size_t j, FILE *stream) {
  DWORD q;
  return (size_t) WriteFile(stream, ptr, (unsigned) (size * j), &q, 0);
}

int fs_close(FILE *file) {
  return CloseHandle(file);
}
