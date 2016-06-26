/*
 * This file contains/describes functions that are used to read, write,
 * check, and use FAT filesystems.
 */

#include "FAT_fs.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <iconv.h>

#include "errors.h"
#include "fileio.h"

int32_t check_bootsector(struct sBootSector *bs) {
  /*
   * lazy check if this is really a FAT bootsector
   */

  assert(bs != NULL);

  if (!((bs->BS_JmpBoot[0] == 0xeb) && (bs->BS_JmpBoot[2] == 0x90)) &&
    !(bs->BS_JmpBoot[0] == 0xe9)) {
    // boot sector does not begin with specific instruction
    myerror("Boot sector does not begin with jump instruction!");
    return -1;
  }
  else if (bs->BS_EndOfBS != 0xaa55) {
    // end of bootsector marker is missing
    myerror("End of boot sector marker is missing!");
    return -1;
  }
  else if (bs->BS_BytesPerSec == 0) {
    myerror("Sectors have a size of zero!");
    return -1;
  }
  else if (bs->BS_BytesPerSec % 512 != 0) {
    myerror("Sector size is not a multiple of 512 (%u)!", bs->BS_BytesPerSec);
    return -1;
  }
  else if (bs->BS_SecPerClus == 0) {
    myerror("Cluster size is zero!");
    return -1;
  }
  else if (bs->BS_SecPerClus * bs->BS_BytesPerSec > 32768) {
    myerror("Cluster size is larger than 32k!");
    return -1;
  }
  else if (bs->BS_RsvdSecCnt == 0) {
    myerror("Reserved sector count is zero!");
    return -1;
  }
  else if (bs->BS_NumFATs == 0) {
    myerror("Number of FATs is zero!");
    return -1;
  }
  else if (bs->BS_RootEntCnt % DIR_ENTRY_SIZE != 0) {
    myerror("Count of root directory entries must be zero or "
      "a multiple or 32! (%u)", bs->BS_RootEntCnt);
    return -1;
  }

  return 0;
}

int32_t read_bootsector(FILE *fd, struct sBootSector *bs) {
  /*
   * reads bootsector
   */

  assert(fd != NULL);
  assert(bs != NULL);

  // seek to beginning of fs
  if (fs_seek(fd, 0, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  if (fs_read(bs, sizeof(struct sBootSector), 1, fd) < 1) {
    if (feof(fd)) {
      myerror("Boot sector is too short!");
    }
    else {
      myerror("Failed to read from file!");
    }
    return -1;
  }

  if (check_bootsector(bs)) {
    myerror("This is not a FAT boot sector or sector is damaged!");
    return -1;
  }

  return 0;
}

int32_t writeBootSector(struct sFileSystem *fs) {
  /*
   * write boot sector
   */

  // seek to beginning of fs
  if (fs_seek(fs->fd, 0, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  // write boot sector
  if (fs_write(&(fs->bs), sizeof(struct sBootSector), 1, fs->fd) < 1) {
    stderror();
    return -1;
  }

  // update backup boot sector for FAT32 file systems
  if (fs->FATType == FATTYPE_FAT32) {
    // seek to beginning of backup boot sector
    if (fs_seek(fs->fd, fs->bs.BS_BkBootSec * fs->sectorSize, SEEK_SET) == -1) {
      stderror();
      return -1;
    }

    // write backup boot sector
    if (fs_write(&(fs->bs), sizeof(struct sBootSector), 1, fs->fd) < 1) {
      stderror();
      return -1;
    }
  }

  return 0;
}

int32_t readFSInfo(struct sFileSystem *fs, struct sFSInfo *fsInfo) {
  /*
   * reads FSInfo structure
   */

  assert(fs != NULL);
  assert(fsInfo != NULL);

  // seek to beginning of FSInfo structure
  if (fs_seek(fs->fd, fs->bs.BS_FSInfo * fs->sectorSize, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  if (fs_read(fsInfo, sizeof(struct sFSInfo), 1, fs->fd) < 1) {
    stderror();
    return -1;
  }

  return 0;
}

int32_t writeFSInfo(struct sFileSystem *fs, struct sFSInfo *fsInfo) {
  /*
   * write FSInfo structure
   */
  assert(fs != NULL);
  assert(fsInfo != NULL);

  // seek to beginning of FSInfo structure
  if (fs_seek(fs->fd, fs->bs.BS_FSInfo * fs->sectorSize, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  // write boot sector
  if (fs_write(fsInfo, sizeof(struct sFSInfo), 1, fs->fd) < 1) {
    stderror();
    return -1;
  }

  return 0;
}

int32_t getCountOfClusters(struct sBootSector *bs) {
  /*
   * calculates count of clusters
   */

  assert(bs != NULL);

  uint32_t RootDirSectors, DataSec;
  int32_t retvalue;

  RootDirSectors =
    (bs->BS_RootEntCnt * DIR_ENTRY_SIZE + bs->BS_BytesPerSec -
    1) / bs->BS_BytesPerSec;

  DataSec =
    bs->BS_TotSec32 - bs->BS_RsvdSecCnt - bs->BS_NumFATs * bs->BS_FATSz32 -
    RootDirSectors;

  retvalue = DataSec / bs->BS_SecPerClus;
  if (retvalue <= 0) {
    myerror("Failed to calculate count of clusters!");
    return -1;
  }
  return retvalue;
}

int32_t getFATType(struct sBootSector *bs) {
  /*
   * retrieves FAT type from bootsector
   */

  assert(bs != NULL);

  int32_t CountOfClusters;

  CountOfClusters = getCountOfClusters(bs);
  if (CountOfClusters == -1) {
    myerror("Failed to get count of clusters!");
    return -1;
  }
  else if (CountOfClusters <= 65536) {
    myerror("Count of clusters not FAT32!");
    return -1;
  }
  else { // FAT32!
    return FATTYPE_FAT32;
  }
}

uint16_t isFreeCluster(const uint32_t data) {
  /*
   * checks whether data marks a free cluster
   */

  return (data & 0x0FFFFFFF) == 0;
}

uint16_t isEOC(struct sFileSystem *fs, const uint32_t data) {
  /*
   * checks whether data marks the end of a cluster chain
   */

  assert(fs != NULL);

  if (fs->FATType == FATTYPE_FAT32) {
    if ((data & 0x0FFFFFFF) >= 0x0FFFFFF8)
      return 1;
  }

  return 0;
}

uint16_t isBadCluster(struct sFileSystem *fs, const uint32_t data) {
  /*
   * checks whether data marks a bad cluster
   */
  assert(fs != NULL);

  if (fs->FATType == FATTYPE_FAT32) {
    if ((data & 0x0FFFFFFF) == 0x0FFFFFF7)
      return 1;
  }

  return 0;
}


void *readFAT(struct sFileSystem *fs, uint16_t nr) {
  /*
   * reads a FAT from file system fs
   */

  assert(fs != NULL);
  assert(nr < fs->bs.BS_NumFATs);

  uint32_t FATSizeInBytes;
  off_t BSOffset;

  void *FAT;

  FATSizeInBytes = fs->FATSize * fs->sectorSize;

  if ((FAT = malloc(FATSizeInBytes)) == NULL) {
    stderror();
    return NULL;
  }
  BSOffset = (off_t) fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec;
  if (fs_seek(fs->fd, BSOffset + nr * FATSizeInBytes, SEEK_SET) == -1) {
    myerror("Seek error!");
    free(FAT);
    return NULL;
  }
  if (fs_read(FAT, FATSizeInBytes, 1, fs->fd) < 1) {
    myerror("Failed to read from file!");
    free(FAT);
    return NULL;
  }

  return FAT;

}

int32_t writeFAT(struct sFileSystem *fs, void *fat) {
  /*
   * write FAT to file system
   */

  assert(fs != NULL);
  assert(fat != NULL);

  uint32_t FATSizeInBytes, nr;
  off_t BSOffset;

  FATSizeInBytes = fs->FATSize * fs->sectorSize;

  BSOffset = (off_t) fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec;

  // write all FATs!
  for (nr = 0; nr < fs->bs.BS_NumFATs; nr++) {
    if (fs_seek(fs->fd, BSOffset + nr * FATSizeInBytes, SEEK_SET) == -1) {
      myerror("Seek error!");
      return -1;
    }
    if (fs_write(fat, FATSizeInBytes, 1, fs->fd) < 1) {
      myerror("Failed to read from file!");
      return -1;
    }
  }

  return 0;
}

int32_t checkFATs(struct sFileSystem *fs) {
  /*
   * checks whether all FATs have the same content
   */

  assert(fs != NULL);

  uint32_t FATSizeInBytes;
  int32_t result = 0;
  int32_t i;

  off_t BSOffset;

  char *FAT1, *FATx;

  // if there is just one FAT, we don't have to check anything
  if (fs->bs.BS_NumFATs < 2) {
    return 0;
  }

  FATSizeInBytes = fs->FATSize * fs->sectorSize;

  if ((FAT1 = malloc(FATSizeInBytes)) == NULL) {
    stderror();
    return -1;
  }
  if ((FATx = malloc(FATSizeInBytes)) == NULL) {
    stderror();
    free(FAT1);
    return -1;
  }
  BSOffset = (off_t) fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec;
  if (fs_seek(fs->fd, BSOffset, SEEK_SET) == -1) {
    myerror("Seek error!");
    free(FAT1);
    free(FATx);
    return -1;
  }
  if (fs_read(FAT1, FATSizeInBytes, 1, fs->fd) < 1) {
    myerror("Failed to read from file!");
    free(FAT1);
    free(FATx);
    return -1;
  }

  for (i = 1; i < fs->bs.BS_NumFATs; i++) {
    if (fs_seek(fs->fd, BSOffset + FATSizeInBytes, SEEK_SET) == -1) {
      myerror("Seek error!");
      free(FAT1);
      free(FATx);
      return -1;
    }
    if (fs_read(FATx, FATSizeInBytes, 1, fs->fd) < 1) {
      myerror("Failed to read from file!");
      free(FAT1);
      free(FATx);
      return -1;
    }

    result = memcmp(FAT1, FATx, FATSizeInBytes) != 0;
    if (result)
      break; // FATs don't match

  }

  free(FAT1);
  free(FATx);

  return result;
}

int32_t getFATEntry(struct sFileSystem *fs, uint32_t cluster, uint32_t *data) {
  /*
   * retrieves FAT entry for a cluster number
   */

  assert(fs != NULL);
  assert(data != NULL);

  off_t FATOffset;
  div_t BSOffset;

  *data = 0;

  switch (fs->FATType) {
  case FATTYPE_FAT32:
    FATOffset = cluster * 4;
    BSOffset =
      div(fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec + FATOffset,
      fs->sectorSize);
    if (fs_seek(fs->fd, BSOffset.quot * fs->sectorSize, SEEK_SET) == -1) {
      myerror("Seek error!");
      return -1;
    }
    char *go = alloca(fs->sectorSize);
    if (fs_read(go, fs->sectorSize, 1, fs->fd) < 1) {
      myerror("Failed to read from file!");
      return -1;
    }
    memcpy(data, go + BSOffset.rem, sizeof *data);
    *data = *data & 0x0fffffff;
    break;
  default:
    myerror("Failed to get FAT type!");
    return -1;
  }

  return 0;

}

off_t getClusterOffset(struct sFileSystem *fs, uint32_t cluster) {
  /*
   * returns the offset of a specific cluster in the data region of the file
   * system
   */

  assert(fs != NULL);
  assert(cluster > 1);

  return ((cluster - 2) * fs->bs.BS_SecPerClus +
    fs->firstDataSector) * fs->sectorSize;

}

void *readCluster(struct sFileSystem *fs, uint32_t cluster) {
  /*
   * read cluster from file system
   */
  void *dummy;

  if (fs_seek(fs->fd, getClusterOffset(fs, cluster), SEEK_SET) != 0) {
    stderror();
    return NULL;
  }

  if ((dummy = malloc(fs->clusterSize)) == NULL) {
    stderror();
    return NULL;
  }

  if ((fs_read(dummy, fs->clusterSize, 1, fs->fd) < 1)) {
    myerror("Failed to read cluster!");
    return NULL;
  }

  return dummy;
}

int32_t writeCluster(struct sFileSystem *fs, uint32_t cluster, void *data) {
  /*
   * write cluster to file systen
   */
  if (fs_seek(fs->fd, getClusterOffset(fs, cluster), SEEK_SET) != 0) {
    stderror();
    return -1;
  }

  if (fs_write(data, fs->clusterSize, 1, fs->fd) < 1) {
    stderror();
    return -1;
  }

  return 0;
}

int32_t parseEntry(struct sFileSystem *fs, union sDirEntry *de) {
  /*
   * parses one directory entry
   */

  assert(fs != NULL);
  assert(de != NULL);

  if (de->ShortDirEntry.DIR_Name[0] == DE_FOLLOWING_FREE)
    return 0; // no more entries

  // long dir entry
  if ((de->LongDirEntry.LDIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
    return 2;

  return 1; // short dir entry
}

uint8_t calculateChecksum(char *sname) {
  uint8_t len;
  uint8_t sum;

  sum = 0;
  for (len = 11; len != 0; len--) {
    sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *sname++;
  }
  return sum;
}


int32_t openFileSystem(char *path, uint32_t mode, struct sFileSystem *fs) {
  /*
   * opens file system and assemlbes file system information into data
   * structure
   */
  assert(path != NULL);
  assert(fs != NULL);

  strcpy(fs->path, "\\\\.\\");
  strcpy(fs->path + 4, path);

  switch (mode) {
  case FS_MODE_RO:
    if ((fs->fd = fs_open(fs->path, GENERIC_READ)) == NULL) {
      stderror();
      return -1;
    }
    break;
  case FS_MODE_RW:
    if ((fs->fd = fs_open(fs->path, GENERIC_READ | GENERIC_WRITE)) == NULL) {
      stderror();
      return -1;
    }
    break;
  default:
    myerror("Mode not supported!");
    return -1;
  }

  // read boot sector
  if (read_bootsector(fs->fd, &(fs->bs))) {
    myerror("Failed to read boot sector!");
    fs_close(fs->fd);
    return -1;
  }

  if (fs->bs.BS_TotSec32 == 0) {
    myerror("Count of total sectors must not be zero!");
    fs_close(fs->fd);
    return -1;
  }

  fs->FATType = getFATType(&(fs->bs));
  if (fs->FATType == -1) {
    myerror("Failed to get FAT type!");
    fs_close(fs->fd);
    return -1;
  }

  if ((fs->FATType == FATTYPE_FAT32) && (fs->bs.BS_FATSz32 == 0)) {
    myerror("32-bit count of FAT sectors must not be zero for FAT32!");
    fs_close(fs->fd);
    return -1;
  }

  fs->FATSize = fs->bs.BS_FATSz32;

  // check whether count of root dir entries is ok for given FAT type
  if ((fs->FATType == FATTYPE_FAT32) && (fs->bs.BS_RootEntCnt != 0)) {
    myerror("Count of root directory entries must be zero for FAT32 (%u)!",
      fs->bs.BS_RootEntCnt);
    fs_close(fs->fd);
    return -1;
  }

  fs->clusters = getCountOfClusters(&(fs->bs));
  if (fs->clusters == -1) {
    myerror("Failed to get count of clusters!");
    fs_close(fs->fd);
    return -1;
  }

  if (fs->clusters > 268435445) {
    myerror("Count of clusters should be less than 268435446, but is %d!",
      fs->clusters);
    fs_close(fs->fd);
    return -1;
  }

  fs->sectorSize = fs->bs.BS_BytesPerSec;

  fs->clusterSize = fs->bs.BS_SecPerClus * fs->bs.BS_BytesPerSec;

  fs->FSSize = (uint64_t) fs->clusters * fs->clusterSize;

  fs->maxDirEntriesPerCluster = fs->clusterSize / DIR_ENTRY_SIZE;

  fs->maxClusterChainLength = (uint32_t) MAX_FILE_LEN / fs->clusterSize;

  uint32_t rootDirSectors;

  rootDirSectors =
    ((fs->bs.BS_RootEntCnt * DIR_ENTRY_SIZE) + (fs->bs.BS_BytesPerSec -
      1)) / fs->bs.BS_BytesPerSec;
  fs->firstDataSector =
    fs->bs.BS_RsvdSecCnt + (fs->bs.BS_NumFATs * fs->FATSize)
    + rootDirSectors;

  // convert utf 16 le to local charset
  fs->cd = iconv_open("", "UTF-16LE");
  if (fs->cd == (iconv_t) -1) {
    myerror("iconv_open failed!");
    return -1;
  }

  return 0;
}

int32_t closeFileSystem(struct sFileSystem *fs) {
  /*
   * closes file system
   */
  assert(fs != NULL);

  fs_close(fs->fd);
  iconv_close(fs->cd);

  return 0;
}
