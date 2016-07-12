/*
 * This file contains/describes the cluster chain ADO with its structures and
 * functions. Cluster chain ADOs hold a linked list of cluster numbers.
 * Together all clusters in a cluster chain hold the date of a file or a
 * directory in a FAT filesystem.
 */

#ifndef __clusterchain_h__
#define __clusterchain_h__

struct sClusterChain {
  /*
   * this structure contains cluster chains
   */
  unsigned cluster;
  struct sClusterChain *next;
};

// create new cluster chain
struct sClusterChain *newClusterChain();

// allocate memory and insert cluster into cluster chain
int insertCluster(struct sClusterChain *chain, unsigned cluster);

// free cluster chain
void freeClusterChain(struct sClusterChain *chain);


#endif // __clusterchain_h__
