/*
 * This file contains/describes functions for sorting of FAT filesystems.
 */

#ifndef __sort_h__
#define __sort_h__

#include <stdint.h>
#include "FAT_fs.h"
struct sClusterChain;

// sorts FAT file system
int32_t sortFileSystem(char *filename);

// sorts directory entries in a cluster
int32_t sortClusterChain(struct sFileSystem *fs, uint32_t cluster,
  const char (*path)[MAX_PATH_LEN + 1]);

// returns cluster chain for a given start cluster
int32_t getClusterChain(struct sFileSystem *fs, uint32_t startCluster,
  struct sClusterChain *chain);

#endif // __sort_h__
