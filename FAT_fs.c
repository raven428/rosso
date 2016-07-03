/*
 * This file contains/describes functions that are used to read, write,
 * check, and use FAT filesystems.
 */

#include "FAT_fs.h"

#include <iconv.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "errors.h"
#include "fileio.h"

int32_t check_bootsector(struct sBootSector *bs) {
  /*
   * lazy check if this is really a FAT bootsector
   */

  if (!(bs->BS_JmpBoot[0] == 0xeb && bs->BS_JmpBoot[2] == 0x90) &&
    bs->BS_JmpBoot[0] != 0xe9) {
    // boot sector does not begin with specific instruction
    myerror("Boot sector does not begin with jump instruction!");
    return -1;
  }
  else if (bs->BS_EndOfBS != 0xaa55) {
    // end of bootsector marker is missing
    myerror("End of boot sector marker is missing!");
    return -1;
  }
  else if (!bs->BS_BytesPerSec) {
    myerror("Sectors have a size of zero!");
    return -1;
  }
  else if (bs->BS_BytesPerSec % 512) {
    myerror("Sector size is not a multiple of 512 (%u)!", bs->BS_BytesPerSec);
    return -1;
  }
  else if (!bs->BS_SecPerClus) {
    myerror("Cluster size is zero!");
    return -1;
  }
  else if (bs->BS_SecPerClus * bs->BS_BytesPerSec > 32768) {
    myerror("Cluster size is larger than 32k!");
    return -1;
  }
  else if (!bs->BS_RsvdSecCnt) {
    myerror("Reserved sector count is zero!");
    return -1;
  }
  else if (!bs->BS_NumFATs) {
    myerror("Number of FATs is zero!");
    return -1;
  }
  else if (bs->BS_RootEntCnt % DIR_ENTRY_SIZE) {
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

  // seek to beginning of fs
  if (fs_seek(fd, 0, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  if (!fs_read(bs, sizeof(struct sBootSector), 1, fd)) {
    if (feof(fd)) {
      myerror("Boot sector is too short!");
    }
    else
      myerror("Failed to read from file!");
    return -1;
  }

  if (check_bootsector(bs)) {
    myerror("This is not a FAT boot sector or sector is damaged!");
    return -1;
  }

  return 0;
}

int32_t getCountOfClusters(struct sBootSector *bs) {
  /*
   * calculates count of clusters
   */

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

int32_t checkFATs(struct sFileSystem *fs) {
  /*
   * checks whether all FATs have the same content
   */

  uint32_t FATSizeInBytes;
  int32_t result = 0;
  int32_t i;

  off_t BSOffset;

  char *FAT1, *FATx;

  // if there is just one FAT, we don't have to check anything
  if (fs->bs.BS_NumFATs < 2)
    return 0;

  FATSizeInBytes = fs->FATSize * fs->sectorSize;

  FAT1 = malloc(FATSizeInBytes);
  if (!FAT1) {
    stderror();
    return -1;
  }
  FATx = malloc(FATSizeInBytes);
  if (!FATx) {
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
  if (!fs_read(FAT1, FATSizeInBytes, 1, fs->fd)) {
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
    if (!fs_read(FATx, FATSizeInBytes, 1, fs->fd)) {
      myerror("Failed to read from file!");
      free(FAT1);
      free(FATx);
      return -1;
    }

    result = memcmp(FAT1, FATx, FATSizeInBytes);
    if (result) {
      break; // FATs don't match
    }

  }

  free(FAT1);
  free(FATx);

  return result;
}

int32_t getFATEntry(struct sFileSystem *fs, uint32_t cluster, uint32_t *data) {
  /*
   * retrieves FAT entry for a cluster number
   */

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
    if (!fs_read(go, fs->sectorSize, 1, fs->fd)) {
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

  return ((cluster - 2) * fs->bs.BS_SecPerClus +
    fs->firstDataSector) * fs->sectorSize;

}

int32_t parseEntry(union sDirEntry *de) {
  /*
   * parses one directory entry
   */

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
  for (len = 11; len != 0; len--)
    sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *sname++;
  return sum;
}


int32_t openFileSystem(char *path, uint32_t mode, struct sFileSystem *fs) {
  /*
   * opens file system and assemlbes file system information into data
   * structure
   */

  strcpy(fs->path, "\\\\.\\");
  strcpy(fs->path + 4, path);

  switch (mode) {
  case FS_MODE_RO:
    fs->fd = fs_open(fs->path, GENERIC_READ);
    if (!fs->fd) {
      stderror();
      return -1;
    }
    break;
  case FS_MODE_RW:
    fs->fd = fs_open(fs->path, GENERIC_READ | GENERIC_WRITE);
    if (!fs->fd) {
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

  if (!fs->bs.BS_TotSec32) {
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

  if (fs->FATType == FATTYPE_FAT32 && !fs->bs.BS_FATSz32) {
    myerror("32-bit count of FAT sectors must not be zero for FAT32!");
    fs_close(fs->fd);
    return -1;
  }

  fs->FATSize = fs->bs.BS_FATSz32;

  // check whether count of root dir entries is ok for given FAT type
  if (fs->FATType == FATTYPE_FAT32 && fs->bs.BS_RootEntCnt) {
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

  fs_close(fs->fd);
  iconv_close(fs->cd);

  return 0;
}
