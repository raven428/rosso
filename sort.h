/*
  This file contains/describes functions for sorting of FAT filesystems.
*/

#ifndef __sort_h__
#define __sort_h__

#include <stdlib.h>
#include <sys/types.h>
#include "FAT_fs.h"
#include "clusterchain.h"

// sorts FAT file system
int32_t sortFileSystem(char *filename);

// sorts the root directory of a FAT12 or FAT16 file system
int32_t sortFAT1xRootDirectory(struct sFileSystem *fs);

// sorts directory entries in a cluster
int32_t sortClusterChain(struct sFileSystem *fs, u_int32_t cluster, const char (*path)[MAX_PATH_LEN+1]);

// returns cluster chain for a given start cluster
int32_t getClusterChain(struct sFileSystem *fs, u_int32_t startCluster, struct sClusterChain *chain);

#endif // __sort_h__
