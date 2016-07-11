/*
 * This file contains file io functions
 */

#include "fileio.h"
#include <windows.h>

FILE *fs_open(char *path, uint32_t mode) {
  return CreateFile(path, mode, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
    0);
}

int fs_seek(FILE *stream, long offset, int whence) {
  return (int) SetFilePointer(stream, offset, 0, (uint32_t) whence);
}

off_t fs_read(void *ptr, size_t size, size_t j, FILE *stream) {
  DWORD q;
  return ReadFile(stream, ptr, (uint32_t) (size * j), &q, 0);
}

off_t fs_write(const void *ptr, size_t size, size_t j, FILE *stream) {
  DWORD q;
  return WriteFile(stream, ptr, (uint32_t) (size * j), &q, 0);
}

int fs_close(FILE *file) {
  return CloseHandle(file);
}
