/*
 * This file contains file io functions
 */

#include "fileapi.h"
#include "fileio.h"
#include "handleapi.h"
#include "minwindef.h"
#include "stdint.h"
#include "winnt.h"
#include <stdio.h>
#include <sys/types.h>

FILE *fs_open(char *path, uint32_t mode) {
  return CreateFile(path, mode, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
    0);
}

int fs_seek(FILE *stream, off_t offset, int whence) {
  return SetFilePointer(stream, offset, 0, whence);
}

off_t fs_read(void *ptr, uint32_t size, uint32_t j, FILE *stream) {
  DWORD q;
  return ReadFile(stream, ptr, size * j, &q, 0);
}

off_t fs_write(const void *ptr, uint32_t size, uint32_t j, FILE *stream) {
  DWORD q;
  return WriteFile(stream, ptr, size * j, &q, 0);
}

int fs_close(FILE *file) {
  return CloseHandle(file);
}
