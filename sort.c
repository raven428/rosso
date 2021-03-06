/*
 * This file contains/describes functions for sorting of FAT32 filesystems.
 */

#include "sort.h"

#include <iconv.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clusterchain.h"
#include "entrylist.h"
#include "errors.h"
#include "FAT32.h"
#include "fileio.h"
#include "options.h"

int parseLongFilenamePart(struct sLongDirEntry *lde, char *str, iconv_t cd) {
  /*
   * retrieves a part of a long filename from a directory entry (thanks to M$
   * for this ugly hack...)
   */

  size_t incount, outcount, ret;
  char *outptr = &(str[0]);
  char utf16str[28];
  char *inptr = &(utf16str[0]);

  str[0] = 0;

  memcpy(utf16str, (&lde->LDIR_Ord + 1), 10);
  memcpy(utf16str + 10, (&lde->LDIR_Ord + 14), 12);
  memcpy(utf16str + 22, (&lde->LDIR_Ord + 28), 4);
  memset(utf16str + 26, 0, 2);

  incount = 26;
  outcount = PATH_MAX;

  size_t i;
  for (i = 0; i < 12; i++) {
    if (!utf16str[i * 2] && !utf16str[i * 2 + 1]) {
      incount = i * 2;
      break;
    }
  }

  while (incount) {
    ret = iconv(cd, &inptr, &incount, &outptr, &outcount);
    if (ret == -1U) {
      stderror();
      myerror("iconv failed! %d", ret);
      return -1;
    }
  }
  outptr[0] = 0;

  return 0;
}

void parseShortFilename(struct sShortDirEntry *sde, char *str) {
  /*
   * parses short name of a file
   */

  char *go;
  strncpy(str, sde->DIR_Name, 8);
  str[8] = 0;
  go = strchr(str, ' ');
  if (go)
    go[0] = 0;
  if ((char) *(sde->DIR_Name + 8) != ' ') {
    strcat(str, ".");
    strncat(str, sde->DIR_Name + 8, 3);
    str[12] = 0;
  }
}

int checkLongDirEntries(struct sDirEntryList *list) {
  /*
   * does some integrity checks on LongDirEntries
   */
  int calculatedChecksum;
  unsigned i, nr;
  struct sLongDirEntryList *tmp;

  if (list->entries > 1) {
    calculatedChecksum = calculateChecksum(list->sde->DIR_Name);
    if (list->ldel->lde->LDIR_Ord != DE_FREE && // ignore deleted entries
      list->ldel->lde->LDIR_Ord & ~LAST_LONG_ENTRY) {
      myerror("LongDirEntry should be marked as last long dir entry but "
        "isn't!");
      return -1;
    }

    tmp = list->ldel;

    for (i = 0; i < list->entries - 1; i++) {
      if (tmp->lde->LDIR_Ord != DE_FREE) { // ignore deleted entries
        nr = tmp->lde->LDIR_Ord & ~LAST_LONG_ENTRY; // index of long dir entry
        if (nr != list->entries - 1 - i) {
          myerror("LongDirEntry number is %#x (%#x) but should be %#x!", nr,
            tmp->lde->LDIR_Ord, list->entries - 1 - i);
          return -1;
        }
        if (tmp->lde->LDIR_Checksum != calculatedChecksum) {
          myerror("Checksum for LongDirEntry is %#x but should be %#x!",
            tmp->lde->LDIR_Checksum, calculatedChecksum);
          return -1;
        }
      }
      tmp = tmp->next;
    }
  }

  return 0;
}

int parseClusterChain(struct sFileSystem *fs, struct sClusterChain *chain,
  struct sDirEntryList *list, int *direntries) {
  /*
   * parses a cluster chain and puts found directory entries to list
   */

  unsigned j, entries = 0;
  int ret;
  union sDirEntry de;
  struct sDirEntryList *lnde;
  struct sLongDirEntryList *llist;
  char tmp[PATH_MAX + 1], dummy[PATH_MAX + 1], sname[PATH_MAX + 1],
    lname[PATH_MAX + 1];

  *direntries = 0;

  chain = chain->next; // head element

  llist = 0;
  lname[0] = 0;
  char *q = malloc(fs->clusterSize);
  while (chain) {
    fs_seek(fs->fd, getClusterOffset(fs, chain->cluster), SEEK_SET);
    fs_read(q, 1, fs->clusterSize, fs->fd);
    for (j = 0; j < fs->maxDirEntriesPerCluster; j++) {
      memcpy(&de, q, DIR_ENTRY_SIZE);
      q += DIR_ENTRY_SIZE;
      entries++;
      ret = parseEntry(&de);

      switch (ret) {
      case -1:
        myerror("Failed to parse directory entry!");
        return -1;
      case 0: // current dir entry and following dir entries are free
        if (llist) {
          // short dir entry is still missing!
          myerror("ShortDirEntry is missing after LongDirEntries "
            "(cluster: %08lx, entry %u)!", chain->cluster, j);
          return -1;
        }
        return 0;
      case 1: // short dir entry
        parseShortFilename(&de.ShortDirEntry, sname);
        if (OPT_LIST && strcmp(sname, ".") && strcmp(sname, "..") &&
          (sname[0] & 0xFF) != DE_FREE && de.ShortDirEntry.DIR_Atrr &
          ~ATTR_VOLUME_ID) {

          if (OPT_MORE_INFO)
            printf("%s (%s)\n", lname[0] ? lname : "n/a", sname);
          else {
            printf("%s\n", lname[0] ? lname : sname);
          }
        }

        lnde = newDirEntry(sname, lname, &de.ShortDirEntry, llist, entries);
        if (!lnde) {
          myerror("Failed to create DirEntry!");
          return -1;
        }

        if (checkLongDirEntries(lnde)) {
          myerror("checkDirEntry failed in cluster %08lx at entry %u!",
            chain->cluster, j);
          return -1;
        }

        insertDirEntryList(lnde, list);
        (*direntries)++;
        entries = 0;
        llist = 0;
        lname[0] = 0;
        break;
      case 2: // long dir entry
        if (parseLongFilenamePart(&de.LongDirEntry, tmp, fs->cd)) {
          myerror("Failed to parse long filename part!");
          return -1;
        }

        // insert long dir entry in list
        llist = insertLongDirEntryList(&de.LongDirEntry, llist);
        if (!llist) {
          myerror("Failed to insert LongDirEntry!");
          return -1;
        }

        strncpy(dummy, tmp, PATH_MAX);
        dummy[PATH_MAX] = 0;
        strncat(dummy, lname, PATH_MAX - strlen(dummy));
        dummy[PATH_MAX] = 0;
        strncpy(lname, dummy, PATH_MAX);
        dummy[PATH_MAX] = 0;
        break;
      default:
        myerror("Unhandled return code!");
        return -1;
      }

    }
    chain = chain->next;
  }

  if (llist) {
    // short dir entry is still missing!
    myerror("ShortDirEntry is missing after LongDirEntries "
      "(root directory entry %d)!", j);
    return -1;
  }

  return 0;
}

int getClusterChain(struct sFileSystem *fs, unsigned startCluster,
  struct sClusterChain *chain) {
  /*
   * retrieves an array of all clusters in a cluster chain starting with
   * startCluster
   */

  unsigned cluster, data;
  int ju = 0;

  cluster = startCluster;

  if (fs->FSType == -1) {
    myerror("File system not FAT32!");
    return -1;
  }

  do {
    if (ju == (int) fs->maxClusterChainLength) {
      myerror("Cluster chain is too long!");
      return -1;
    }
    if ((cluster & 0x0fffffff) >= (unsigned) fs->clusters + 2) {
      myerror("Cluster %08x does not exist!", data);
      return -1;
    }
    if (insertCluster(chain, cluster) == -1) {
      myerror("Failed to insert cluster!");
      return -1;
    }
    ju++;
    if (getFAT32Entry(fs, cluster, &data)) {
      myerror("Failed to get FAT32 entry");
      return -1;
    }
    if (!(data & 0x0fffffff)) {
      myerror("Cluster %08x is marked as unused!", cluster);
      return -1;
    }
    cluster = data;
  } while ((cluster & 0x0fffffff) != 0x0ff8fff8 && (cluster &
      0x0fffffff) < 0x0ffffff8); // end of cluster
  return ju;
}

int writeClusterChain(struct sFileSystem *fs, struct sDirEntryList *list,
  struct sClusterChain *chain) {
  /*
   * writes all entries from list to the cluster chain
   */

  unsigned i = 0, entries = 0;
  struct sLongDirEntryList *tmp;
  struct sDirEntryList *ki = list->next;

  chain = chain->next; // we don't need to look at the head element

  if (fs_seek(fs->fd, getClusterOffset(fs, chain->cluster), SEEK_SET) == -1) {
    myerror("Seek error!");
    return -1;
  }

  char *ya = malloc(fs->clusterSize);
  char *zu = ya;
  fs_read(ya, 1, fs->clusterSize, fs->fd);
  fs_seek(fs->fd, (long) -fs->clusterSize, SEEK_CUR);
  while (ki) {
    if (entries + ki->entries <= fs->maxDirEntriesPerCluster) {
      tmp = ki->ldel;
      for (i = 1; i < ki->entries; i++) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      memcpy(zu, ki->sde, DIR_ENTRY_SIZE);
      zu += DIR_ENTRY_SIZE;
      entries += ki->entries;
    }
    else {
      tmp = ki->ldel;
      for (i = 1; i <= fs->maxDirEntriesPerCluster - entries; i++) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      chain = chain->next;
      // next cluster
      entries = ki->entries - (fs->maxDirEntriesPerCluster - entries);
      if (fs_seek(fs->fd, getClusterOffset(fs, chain->cluster),
          SEEK_SET) == -1) {
        myerror("Seek error!");
        return -1;
      }
      while (tmp) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      memcpy(zu, ki->sde, DIR_ENTRY_SIZE);
      zu += DIR_ENTRY_SIZE;
    }
    ki = ki->next;
  }
  if (entries < fs->maxDirEntriesPerCluster) {
    memset(zu, 0, DIR_ENTRY_SIZE);
    if (!fs_write(ya, 1, fs->clusterSize, fs->fd)) {
      stderror();
      return -1;
    }
  }

  return 0;

}

int sortSubdirectories(struct sFileSystem *fs, struct sDirEntryList *list,
  const char (*path)[PATH_MAX + 1]) {
  /*
   * sorts sub directories in a FAT32 file system
   */
  struct sDirEntryList *ki;
  char newpath[PATH_MAX + 1] = { 0 };
  unsigned qu, value;

  // sort sub directories
  ki = list->next;
  while (ki) {
    if (ki->sde->DIR_Atrr & ATTR_DIRECTORY && (ki->sde->DIR_Name[0] &
        0xFF) != DE_FREE && ki->sde->DIR_Atrr & ~ATTR_VOLUME_ID &&
      strcmp(ki->sname, ".") && strcmp(ki->sname, "..")) {
      qu = (ki->sde->DIR_FstClusHI * 65536U + ki->sde->DIR_FstClusLO);
      if (getFAT32Entry(fs, qu, &value) == -1) {
        myerror("Failed to get FAT32 entry!");
        return -1;
      }

      strncpy(newpath, (char *) path, PATH_MAX - strlen(newpath));
      newpath[PATH_MAX] = 0;
      if (ki->lname && ki->lname[0]) {
        strncat(newpath, ki->lname, PATH_MAX - strlen(newpath));
        newpath[PATH_MAX] = 0;
        strncat(newpath, "/", PATH_MAX - strlen(newpath));
        newpath[PATH_MAX] = 0;
      }
      else {
        strncat(newpath, ki->sname, PATH_MAX - strlen(newpath));
        newpath[PATH_MAX] = 0;
        strncat(newpath, "/", PATH_MAX - strlen(newpath));
        newpath[PATH_MAX] = 0;
      }

      if (sortClusterChain(fs, qu,
          (const char (*)[PATH_MAX + 1]) newpath) == -1) {
        myerror("Failed to sort cluster chain!");
        return -1;
      }

    }
    ki = ki->next;
  }

  return 0;
}

int sortClusterChain(struct sFileSystem *fs, unsigned cluster,
  const char (*path)[PATH_MAX + 1]) {
  /*
   * sorts directory entries in a cluster
   */

  int direntries, clen, match;
  struct sClusterChain *ClusterChain;
  struct sDirEntryList *list;

  match =
    matchesDirPathLists(OPT_INCL_DIRS, OPT_INCL_DIRS_REC, OPT_EXCL_DIRS,
    OPT_EXCL_DIRS_REC, path);

  ClusterChain = newClusterChain();
  if (!ClusterChain) {
    myerror("Failed to generate new ClusterChain!");
    return -1;
  }

  list = newDirEntryList();
  if (!list) {
    myerror("Failed to generate new dirEntryList!");
    freeClusterChain(ClusterChain);
    return -1;
  }

  if (match) {
    clen = getClusterChain(fs, cluster, ClusterChain);
    if (clen == -1) {
      myerror("Failed to get cluster chain!");
      freeDirEntryList(list);
      freeClusterChain(ClusterChain);
      return -1;
    }

    if (OPT_LIST) {
      if (strcmp((char *) path, "/"))
        puts("");
      printf("%s\n", (char *) path);
      if (OPT_MORE_INFO) {
        printf("Start cluster: %08d, length: %d (%d bytes)\n", cluster, clen,
          clen * (int) fs->clusterSize);
      }
    }
    else {
      printf(OPT_RANDOM ? "Random sorting directory %s\n" :
        "Sorting directory %s\n", (char *) path);
      if (OPT_MORE_INFO) {
        printf("Start cluster: %08d, length: %d (%d bytes)\n", cluster, clen,
          clen * (int) fs->clusterSize);
      }
    }

    if (parseClusterChain(fs, ClusterChain, list, &direntries) == -1) {
      myerror("Failed to parse cluster chain!");
      freeDirEntryList(list);
      freeClusterChain(ClusterChain);
      return -1;
    }

    // sort directory if it is selected
    if (!OPT_LIST) {

      if (OPT_RANDOM)
        randomizeDirEntryList(list, direntries);

      if (writeClusterChain(fs, list, ClusterChain) == -1) {
        myerror("Failed to write cluster chain!");
        freeDirEntryList(list);
        freeClusterChain(ClusterChain);
        return -1;
      }
    }

    freeClusterChain(ClusterChain);

    // sort subdirectories
    if (sortSubdirectories(fs, list, path) == -1) {
      myerror("Failed to sort subdirectories!");
      return -1;
    }
  }

  freeDirEntryList(list);

  return 0;
}

int sortFileSystem(char *filename) {
  /*
   * sort FAT32 file system
   */

  struct sFileSystem fs;

  if (openFileSystem(filename, OPT_LIST ? "rb" : "r+b", &fs)) {
    myerror("Failed to open file system!");
    return -1;
  }

  if (checkFAT32s(&fs)) {
    myerror("FAT32s don't match! Please repair file system!");
    closeFileSystem(&fs);
    return -1;
  }

  if (fs.FSType == -1) {
    myerror("File system not FAT32!");
    closeFileSystem(&fs);
    return -1;
  }
  /*
   * root directory lies in cluster chain, so sort it like all other
   * directories
   */
  if (sortClusterChain(&fs, fs.bs.BS_RootClus,
      (const char (*)[PATH_MAX + 1]) "/") == -1) {
    myerror("Failed to sort first cluster chain!");
    closeFileSystem(&fs);
    return -1;
  }

  closeFileSystem(&fs);

  return 0;
}
