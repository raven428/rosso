/*
 * This file contains/describes the cluster chain ADO with its structures and
 * functions. Cluster chain ADOs hold a linked list of cluster numbers.
 * Together all clusters in a cluster chain hold the date of a file or a
 * directory in a FAT filesystem.
 */

#include "clusterchain.h"

#include <stdlib.h>
#include <errno.h>
#include "errors.h"

// const struct sClusterChain __INITCLUSTERCHAIN__ = {0, NULL};

struct sClusterChain *newClusterChain(void) {
  /*
   * create new cluster chain
   */
  struct sClusterChain *tmp;

  if ((tmp = malloc(sizeof(struct sClusterChain))) == NULL) {
    stderror();
    return NULL;
  }
  tmp->cluster = 0;
  tmp->next = NULL;
  return tmp;
}

int32_t insertCluster(struct sClusterChain *chain, uint32_t cluster) {
  /*
   * allocate memory and insert cluster into cluster chain
   */
  while (chain->next != NULL) {
    if (chain->cluster == cluster) {
      myerror("Loop in cluster chain detected (%08lx)!", cluster);
      return -1;
    }
    chain = chain->next;
  }

  if ((chain->next = malloc(sizeof(struct sClusterChain))) == NULL) {
    stderror();
    return -1;
  }
  chain->next->cluster = cluster;
  chain->next->next = NULL;

  return 0;
}

void freeClusterChain(struct sClusterChain *chain) {
  /*
   * free cluster chain
   */

  struct sClusterChain *tmp;

  while (chain != NULL) {
    tmp = chain;
    chain = chain->next;
    free(tmp);
  }

}
