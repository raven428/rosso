/*
 * This file contains/describes the cluster chain ADO with its structures and
 * functions. Cluster chain ADOs hold a linked list of cluster numbers.
 * Together all clusters in a cluster chain hold the date of a file or a
 * directory in a FAT filesystem.
 */

#include "clusterchain.h"

#include <stdlib.h>
#include "errors.h"

struct sClusterChain *newClusterChain() {
  /*
   * create new cluster chain
   */
  struct sClusterChain *tmp;

  tmp = malloc(sizeof(struct sClusterChain));
  if (!tmp) {
    stderror();
    return 0;
  }
  tmp->cluster = 0;
  tmp->next = 0;
  return tmp;
}

int insertCluster(struct sClusterChain *chain, unsigned cluster) {
  /*
   * allocate memory and insert cluster into cluster chain
   */
  while (chain->next) {
    if (chain->cluster == cluster) {
      myerror("Loop in cluster chain detected (%08lx)!", cluster);
      return -1;
    }
    chain = chain->next;
  }

  chain->next = malloc(sizeof(struct sClusterChain));
  if (!chain->next) {
    stderror();
    return -1;
  }
  chain->next->cluster = cluster;
  chain->next->next = 0;

  return 0;
}

void freeClusterChain(struct sClusterChain *chain) {
  /*
   * free cluster chain
   */

  struct sClusterChain *tmp;

  while (chain) {
    tmp = chain;
    chain = chain->next;
    free(tmp);
  }

}
