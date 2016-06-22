/*
 * This file contains/describes functions for sorting of FAT filesystems.
 */

#include "sort.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include "entrylist.h"
#include "errors.h"
#include "options.h"
#include "clusterchain.h"
#include "signal.h"
#include "fileio.h"
#include "stringlist.h"

int32_t
parseLongFilenamePart(struct sLongDirEntry *lde, char *str, iconv_t cd) {
  /*
   * retrieves a part of a long filename from a directory entry (thanks to M$
   * for this ugly hack...)
   */

  assert(lde != NULL);
  assert(str != NULL);

  size_t incount;
  size_t outcount;
  char *outptr = &(str[0]);
  char utf16str[28];
  char *inptr = &(utf16str[0]);
  size_t ret;

  str[0] = '\0';

  memcpy(utf16str, (&lde->LDIR_Ord + 1), 10);
  memcpy(utf16str + 10, (&lde->LDIR_Ord + 14), 12);
  memcpy(utf16str + 22, (&lde->LDIR_Ord + 28), 4);
  memset(utf16str + 26, 0, 2);

  incount = 26;
  outcount = MAX_PATH_LEN;

  int i;
  for (i = 0; i < 12; i++) {
    if ((utf16str[i * 2] == '\0') && (utf16str[i * 2 + 1] == '\0')) {
      incount = i * 2;
      break;
    }
  }

  while (incount != 0) {
    if ((ret =
        iconv(cd, &inptr, &incount, &outptr, &outcount)) == (size_t) -1) {
      stderror();
      myerror("iconv failed! %d", ret);
      return -1;
    }
  }
  outptr[0] = '\0';

  return 0;
}

void
parseShortFilename(struct sShortDirEntry *sde, char *str) {
  /*
   * parses short name of a file
   */

  assert(sde != NULL);
  assert(str != NULL);

  char *s;
  strncpy(str, sde->DIR_Name, 8);
  str[8] = '\0';
  s = strchr(str, ' ');
  if (s != NULL)
    s[0] = '\0';
  if ((char) (*(sde->DIR_Name + 8)) != ' ') {
    strcat(str, ".");
    strncat(str, sde->DIR_Name + 8, 3);
    str[12] = '\0';
  }
}

int32_t
checkLongDirEntries(struct sDirEntryList *list) {
  /*
   * does some integrity checks on LongDirEntries
   */
  assert(list != NULL);

  uint8_t calculatedChecksum;
  uint32_t i;
  uint32_t nr;
  struct sLongDirEntryList *tmp;

  if (list->entries > 1) {
    calculatedChecksum = calculateChecksum(list->sde->DIR_Name);
    if ((list->ldel->lde->LDIR_Ord != DE_FREE) && // ignore deleted entries
      !(list->ldel->lde->LDIR_Ord & LAST_LONG_ENTRY)) {
      myerror("LongDirEntry should be marked as last long dir entry but "
        "isn't!");
      return -1;
    }

    tmp = list->ldel;

    for (i = 0; i < list->entries - 1; i++) {
      if (tmp->lde->LDIR_Ord != DE_FREE) { // ignore deleted entries
        nr = tmp->lde->LDIR_Ord & ~LAST_LONG_ENTRY; // index of long dir entry
        if (nr != (list->entries - 1 - i)) {
          myerror("LongDirEntry number is 0x%x (0x%x) but should be 0x%x!",
            nr, tmp->lde->LDIR_Ord, list->entries - 1 - i);
          return -1;
        }
        else if (tmp->lde->LDIR_Checksum != calculatedChecksum) {
          myerror("Checksum for LongDirEntry is 0x%x but should be 0x%x!",
            tmp->lde->LDIR_Checksum, calculatedChecksum);
          return -1;
        }
      }
      tmp = tmp->next;
    }
  }

  return 0;
}

int32_t
parseClusterChain(struct sFileSystem *fs, struct sClusterChain *chain,
  struct sDirEntryList *list, uint32_t *direntries) {
  /*
   * parses a cluster chain and puts found directory entries to list
   */

  assert(fs != NULL);
  assert(chain != NULL);
  assert(list != NULL);
  assert(direntries != NULL);

  uint32_t j;
  int32_t ret;
  uint32_t entries = 0;
  union sDirEntry de;
  struct sDirEntryList *lnde;
  struct sLongDirEntryList *llist;
  char tmp[MAX_PATH_LEN + 1], dummy[MAX_PATH_LEN + 1],
    sname[MAX_PATH_LEN + 1], lname[MAX_PATH_LEN + 1];

  *direntries = 0;

  chain = chain->next; // head element

  llist = NULL;
  lname[0] = '\0';
  char *q = alloca(fs->clusterSize);
  while (chain != NULL) {
    fs_seek(fs->fd, getClusterOffset(fs, chain->cluster), SEEK_SET);
    fs_read(q, 1, fs->clusterSize, fs->fd);
    for (j = 0; j < fs->maxDirEntriesPerCluster; j++) {
      memcpy(&de, q, DIR_ENTRY_SIZE);
      q += DIR_ENTRY_SIZE;
      entries++;
      ret = parseEntry(fs, &de);

      switch (ret) {
      case -1:
        myerror("Failed to parse directory entry!");
        return -1;
      case 0: // current dir entry and following dir entries are free
        if (llist != NULL) {
          // short dir entry is still missing!
          myerror("ShortDirEntry is missing after LongDirEntries "
            "(cluster: %08lx, entry %u)!", chain->cluster, j);
          return -1;
        }
        else {
          return 0;
        }
      case 1: // short dir entry
        parseShortFilename(&de.ShortDirEntry, sname);

        if (OPT_LIST && strcmp(sname, ".") && strcmp(sname, "..") &&
          (((uint8_t) sname[0]) != DE_FREE) &&
          !(de.ShortDirEntry.DIR_Atrr & ATTR_VOLUME_ID)) {

          if (!OPT_MORE_INFO) {
            printf("%s\n", (lname[0] != '\0') ? lname : sname);
          }
          else {
            printf("%s (%s)\n", (lname[0] != '\0') ? lname : "n/a", sname);
          }
        }

        lnde = newDirEntry(sname, lname, &de.ShortDirEntry, llist, entries);
        if (lnde == NULL) {
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
        llist = NULL;
        lname[0] = '\0';
        break;
      case 2: // long dir entry
        if (parseLongFilenamePart(&de.LongDirEntry, tmp, fs->cd)) {
          myerror("Failed to parse long filename part!");
          return -1;
        }

        // insert long dir entry in list
        llist = insertLongDirEntryList(&de.LongDirEntry, llist);
        if (llist == NULL) {
          myerror("Failed to insert LongDirEntry!");
          return -1;
        }

        strncpy(dummy, tmp, MAX_PATH_LEN);
        dummy[MAX_PATH_LEN] = '\0';
        strncat(dummy, lname, MAX_PATH_LEN - strlen(dummy));
        dummy[MAX_PATH_LEN] = '\0';
        strncpy(lname, dummy, MAX_PATH_LEN);
        dummy[MAX_PATH_LEN] = '\0';
        break;
      default:
        myerror("Unhandled return code!");
        return -1;
      }

    }
    chain = chain->next;
  }

  if (llist != NULL) {
    // short dir entry is still missing!
    myerror("ShortDirEntry is missing after LongDirEntries "
      "(root directory entry %d)!", j);
    return -1;
  }

  return 0;
}

int32_t
writeList(struct sFileSystem *fs, struct sDirEntryList *list) {
  /*
   * writes directory entries to file
   */

  assert(fs != NULL);
  assert(list != NULL);

  struct sLongDirEntryList *tmp;

  while (list->next != NULL) {
    tmp = list->next->ldel;
    while (tmp != NULL) {
      if (fs_write(tmp->lde, DIR_ENTRY_SIZE, 1, fs->fd) < 1) {
        stderror();
        return -1;
      }
      tmp = tmp->next;
    }
    if (fs_write(list->next->sde, DIR_ENTRY_SIZE, 1, fs->fd) < 1) {
      stderror();
      return -1;
    }
    list = list->next;
  }

  return 0;
}

int32_t
getClusterChain(struct sFileSystem *fs, uint32_t startCluster,
  struct sClusterChain *chain) {
  /*
   * retrieves an array of all clusters in a cluster chain starting with
   * startCluster
   */

  assert(fs != NULL);
  assert(chain != NULL);

  int32_t cluster;
  uint32_t data, i = 0;

  cluster = startCluster;

  switch (fs->FATType) {
  case FATTYPE_FAT32:
    do {
      if (i == fs->maxClusterChainLength) {
        myerror("Cluster chain is too long!");
        return -1;
      }
      if ((cluster & 0x0fffffff) >= fs->clusters + 2) {
        myerror("Cluster %08x does not exist!", data);
        return -1;
      }
      if (insertCluster(chain, cluster) == -1) {
        myerror("Failed to insert cluster!");
        return -1;
      }
      i++;
      if (getFATEntry(fs, cluster, &data)) {
        myerror("Failed to get FAT entry");
        return -1;
      }
      if ((data & 0x0fffffff) == 0) {
        myerror("Cluster %08x is marked as unused!", cluster);
        return -1;
      }
      cluster = data;
    } while (((cluster & 0x0fffffff) != 0x0ff8fff8) &&
      ((cluster & 0x0fffffff) < 0x0ffffff8)); // end of cluster
    break;
  case -1:
  default:
    myerror("Failed to get FAT type!");
    return -1;
  }

  return i;
}

int32_t
writeClusterChain(struct sFileSystem *fs, struct sDirEntryList *list,
  struct sClusterChain *chain) {
  /*
   * writes all entries from list to the cluster chain
   */

  assert(fs != NULL);
  assert(list != NULL);
  assert(chain != NULL);

  uint32_t i = 0, entries = 0;
  struct sLongDirEntryList *tmp;
  struct sDirEntryList *p = list->next;

  chain = chain->next; // we don't need to look at the head element

  if (fs_seek(fs->fd, getClusterOffset(fs, chain->cluster), SEEK_SET) == -1) {
    myerror("Seek error!");
    return -1;
  }

  char *ya = alloca(fs->clusterSize);
  char *zu = ya;
  fs_read(ya, 1, fs->clusterSize, fs->fd);
  fs_seek(fs->fd, -fs->clusterSize, SEEK_CUR);
  while (p != NULL) {
    if (entries + p->entries <= fs->maxDirEntriesPerCluster) {
      tmp = p->ldel;
      for (i = 1; i < p->entries; i++) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      memcpy(zu, p->sde, DIR_ENTRY_SIZE);
      zu += DIR_ENTRY_SIZE;
      entries += p->entries;
    }
    else {
      tmp = p->ldel;
      for (i = 1; i <= fs->maxDirEntriesPerCluster - entries; i++) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      chain = chain->next;
      // next cluster
      entries = p->entries - (fs->maxDirEntriesPerCluster - entries);
      if (fs_seek(fs->fd, getClusterOffset(fs, chain->cluster),
          SEEK_SET) == -1) {
        myerror("Seek error!");
        return -1;
      }
      while (tmp != NULL) {
        memcpy(zu, tmp->lde, DIR_ENTRY_SIZE);
        zu += DIR_ENTRY_SIZE;
        tmp = tmp->next;
      }
      memcpy(zu, p->sde, DIR_ENTRY_SIZE);
      zu += DIR_ENTRY_SIZE;
    }
    p = p->next;
  }
  if (entries < fs->maxDirEntriesPerCluster) {
    memset(zu, 0, DIR_ENTRY_SIZE);
    if (fs_write(ya, fs->clusterSize, 1, fs->fd) < 1) {
      stderror();
      return -1;
    }
  }

  return 0;

}

int32_t
sortSubdirectories(struct sFileSystem *fs, struct sDirEntryList *list,
  const char (*path)[MAX_PATH_LEN + 1]) {
  /*
   * sorts sub directories in a FAT file system
   */
  assert(fs != NULL);
  assert(list != NULL);
  assert(path != NULL);

  struct sDirEntryList *p;
  char newpath[MAX_PATH_LEN + 1] = { 0 };
  uint32_t c, value;

  // sort sub directories
  p = list->next;
  while (p != NULL) {
    if ((p->sde->DIR_Atrr & ATTR_DIRECTORY) &&
      ((uint8_t) p->sde->DIR_Name[0] != DE_FREE) &&
      !(p->sde->DIR_Atrr & ATTR_VOLUME_ID) && (strcmp(p->sname, ".")) &&
      strcmp(p->sname, "..")) {

      c = (p->sde->DIR_FstClusHI * 65536 + p->sde->DIR_FstClusLO);
      if (getFATEntry(fs, c, &value) == -1) {
        myerror("Failed to get FAT entry!");
        return -1;
      }

      strncpy(newpath, (char *) path, MAX_PATH_LEN - strlen(newpath));
      newpath[MAX_PATH_LEN] = '\0';
      if ((p->lname != NULL) && (p->lname[0] != '\0')) {
        strncat(newpath, p->lname, MAX_PATH_LEN - strlen(newpath));
        newpath[MAX_PATH_LEN] = '\0';
        strncat(newpath, "/", MAX_PATH_LEN - strlen(newpath));
        newpath[MAX_PATH_LEN] = '\0';
      }
      else {
        strncat(newpath, p->sname, MAX_PATH_LEN - strlen(newpath));
        newpath[MAX_PATH_LEN] = '\0';
        strncat(newpath, "/", MAX_PATH_LEN - strlen(newpath));
        newpath[MAX_PATH_LEN] = '\0';
      }

      if (sortClusterChain(fs, c,
          (const char (*)[MAX_PATH_LEN + 1]) newpath) == -1) {
        myerror("Failed to sort cluster chain!");
        return -1;
      }

    }
    p = p->next;
  }

  return 0;
}

int32_t
sortClusterChain(struct sFileSystem *fs, uint32_t cluster,
  const char (*path)[MAX_PATH_LEN + 1]) {
  /*
   * sorts directory entries in a cluster
   */

  assert(fs != NULL);
  assert(path != NULL);

  uint32_t direntries;
  int32_t clen;
  struct sClusterChain *ClusterChain;
  struct sDirEntryList *list;

  uint32_t match;

  match =
    matchesDirPathLists(OPT_INCL_DIRS, OPT_INCL_DIRS_REC, OPT_EXCL_DIRS,
    OPT_EXCL_DIRS_REC, path);

  if ((ClusterChain = newClusterChain()) == NULL) {
    myerror("Failed to generate new ClusterChain!");
    return -1;
  }

  if ((list = newDirEntryList()) == NULL) {
    myerror("Failed to generate new dirEntryList!");
    freeClusterChain(ClusterChain);
    return -1;
  }

  if ((clen = getClusterChain(fs, cluster, ClusterChain)) == -1) {
    myerror("Failed to get cluster chain!");
    freeDirEntryList(list);
    freeClusterChain(ClusterChain);
    return -1;
  }

  if (!OPT_LIST) {
    if (match) {
      printf("Sorting directory %s\n", (char *) path);
      if (OPT_MORE_INFO)
        printf("Start cluster: %08d, length: %d (%d bytes)\n", cluster, clen,
          clen * fs->clusterSize);
    }
  }
  else {
    printf("%s\n", (char *) path);
    if (OPT_MORE_INFO)
      printf("Start cluster: %08d, length: %d (%d bytes)\n", cluster, clen,
        clen * fs->clusterSize);
  }

  if (parseClusterChain(fs, ClusterChain, list, &direntries) == -1) {
    myerror("Failed to parse cluster chain!");
    freeDirEntryList(list);
    freeClusterChain(ClusterChain);
    return -1;
  }

  if (!OPT_LIST) {
    // sort directory if selected
    if (match) {

      if (OPT_RANDOM)
        randomizeDirEntryList(list, direntries);

      if (writeClusterChain(fs, list, ClusterChain) == -1) {
        myerror("Failed to write cluster chain!");
        freeDirEntryList(list);
        freeClusterChain(ClusterChain);
        return -1;
      }
    }
  }
  else {
    printf("\n");
  }

  freeClusterChain(ClusterChain);

  // sort subdirectories
  if (sortSubdirectories(fs, list, path) == -1) {
    myerror("Failed to sort subdirectories!");
    return -1;
  }

  freeDirEntryList(list);

  return 0;
}

int32_t
sortFileSystem(char *filename) {
  /*
   * sort FAT file system
   */

  assert(filename != NULL);

  uint32_t mode;

  struct sFileSystem fs;

  if (OPT_LIST) {
    mode = FS_MODE_RO;
  }
  else {
    mode = FS_MODE_RW;
  }

  if (openFileSystem(filename, mode, &fs)) {
    myerror("Failed to open file system!");
    return -1;
  }

  if (checkFATs(&fs)) {
    myerror("FATs don't match! Please repair file system!");
    closeFileSystem(&fs);
    return -1;
  }

  switch (fs.FATType) {
  case FATTYPE_FAT32:
    // FAT32
    // root directory lies in cluster chain,
    // so sort it like all other directories
    printf("File system: FAT32.\n\n");
    if (sortClusterChain(&fs, fs.bs.BS_RootClus,
        (const char (*)[MAX_PATH_LEN + 1]) "/") == -1) {
      myerror("Failed to sort first cluster chain!");
      closeFileSystem(&fs);
      return -1;
    }
    break;
  default:
    myerror("Failed to get FAT type!");
    closeFileSystem(&fs);
    return -1;
  }

  closeFileSystem(&fs);

  return 0;
}
