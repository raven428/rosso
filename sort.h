/*
 * This file contains/describes functions for sorting of FAT filesystems.
 */

#ifndef __sort_h__
#define __sort_h__

#include "FAT_fs.h"
struct sClusterChain;

// sorts FAT file system
int sortFileSystem(char *filename);

// sorts directory entries in a cluster
int sortClusterChain(struct sFileSystem *fs, unsigned cluster,
  const char (*path)[MAX_PATH_LEN + 1]);

// returns cluster chain for a given start cluster
int getClusterChain(struct sFileSystem *fs, unsigned startCluster,
  struct sClusterChain *chain);

#endif // __sort_h__
