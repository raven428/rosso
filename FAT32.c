/*
 * This file contains/describes functions that are used to read, write,
 * check, and use FAT32 filesystems.
 */

#include "FAT32.h"

#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "fileio.h"

int check_bootsector(struct sBootSector *bs) {
  /*
   * lazy check if this is really a FAT32 bootsector
   */

  if (!(bs->BS_JmpBoot[0] == 0xeb && bs->BS_JmpBoot[2] == 0x90) &&
    bs->BS_JmpBoot[0] != 0xe9) {
    // boot sector does not begin with specific instruction
    myerror("Boot sector does not begin with jump instruction!");
    return -1;
  }
  if (bs->BS_EndOfBS != 0xaa55) {
    // end of bootsector marker is missing
    myerror("End of boot sector marker is missing!");
    return -1;
  }
  if (!bs->BS_BytesPerSec) {
    myerror("Sectors have a size of zero!");
    return -1;
  }
  if (bs->BS_BytesPerSec % 512) {
    myerror("Sector size is not a multiple of 512 (%u)!", bs->BS_BytesPerSec);
    return -1;
  }
  if (!bs->BS_SecPerClus) {
    myerror("Cluster size is zero!");
    return -1;
  }
  if (bs->BS_SecPerClus * bs->BS_BytesPerSec > 32768) {
    myerror("Cluster size is larger than 32k!");
    return -1;
  }
  if (!bs->BS_RsvdSecCnt) {
    myerror("Reserved sector count is zero!");
    return -1;
  }
  if (!bs->BS_NumFAT32s) {
    myerror("Number of FAT32s is zero!");
    return -1;
  }
  if (bs->BS_RootEntCnt % DIR_ENTRY_SIZE) {
    myerror("Count of root directory entries must be zero or "
      "a multiple or 32! (%u)", bs->BS_RootEntCnt);
    return -1;
  }

  return 0;
}

int read_bootsector(FILE *fd, struct sBootSector *bs) {
  /*
   * reads bootsector
   */

  // seek to beginning of fs
  if (fs_seek(fd, 0, SEEK_SET) == -1) {
    stderror();
    return -1;
  }

  if (!fs_read(bs, 1, sizeof(struct sBootSector), fd)) {
    if (feof(fd)) {
      myerror("Boot sector is too short!");
    }
    else
      myerror("Failed to read from file!");
    return -1;
  }

  if (check_bootsector(bs)) {
    myerror("This is not a FAT32 boot sector or sector is damaged!");
    return -1;
  }

  return 0;
}

int getCountOfClusters(struct sBootSector *bs) {
  /*
   * calculates count of clusters
   */

  unsigned RootDirSectors, DataSec;
  int retvalue;

  RootDirSectors =
    (bs->BS_RootEntCnt * DIR_ENTRY_SIZE + bs->BS_BytesPerSec -
    1) / bs->BS_BytesPerSec;

  DataSec =
    bs->BS_TotSec32 - bs->BS_RsvdSecCnt - bs->BS_NumFAT32s * bs->BS_FAT32Sz -
    RootDirSectors;

  retvalue = (int) DataSec / bs->BS_SecPerClus;
  if (retvalue <= 0) {
    myerror("Failed to calculate count of clusters!");
    return -1;
  }
  return retvalue;
}

int checkFSType(struct sBootSector *bs) {
  /*
   * retrieves FAT32 type from bootsector
   */

  int CountOfClusters;

  CountOfClusters = getCountOfClusters(bs);
  if (CountOfClusters == -1) {
    myerror("Failed to get count of clusters!");
    return -1;
  }
  if (CountOfClusters <= 65536) {
    myerror("Count of clusters not FAT32!");
    return -1;
  }
  return 0; // FAT32!
}

int checkFAT32s(struct sFileSystem *fs) {
  /*
   * checks whether all FAT32s have the same content
   */

  unsigned FAT32SizeInBytes;
  int i, BSOffset, result = 0;

  char *FS1, *FSx;

  // if there is just one FAT32, we don't have to check anything
  if (fs->bs.BS_NumFAT32s < 2)
    return 0;

  FAT32SizeInBytes = fs->FAT32Size * fs->sectorSize;

  FS1 = malloc(FAT32SizeInBytes);
  if (!FS1) {
    stderror();
    return -1;
  }
  FSx = malloc(FAT32SizeInBytes);
  if (!FSx) {
    stderror();
    free(FS1);
    return -1;
  }
  BSOffset = fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec;
  if (fs_seek(fs->fd, BSOffset, SEEK_SET) == -1) {
    myerror("Seek error!");
    free(FS1);
    free(FSx);
    return -1;
  }
  if (!fs_read(FS1, 1, FAT32SizeInBytes, fs->fd)) {
    myerror("Failed to read from file!");
    free(FS1);
    free(FSx);
    return -1;
  }

  for (i = 1; i < fs->bs.BS_NumFAT32s; i++) {
    if (fs_seek(fs->fd, BSOffset + (int) FAT32SizeInBytes, SEEK_SET) == -1) {
      myerror("Seek error!");
      free(FS1);
      free(FSx);
      return -1;
    }
    if (!fs_read(FSx, 1, FAT32SizeInBytes, fs->fd)) {
      myerror("Failed to read from file!");
      free(FS1);
      free(FSx);
      return -1;
    }

    result = memcmp(FS1, FSx, FAT32SizeInBytes);
    if (result) {
      break; // FAT32s do not match
    }

  }

  free(FS1);
  free(FSx);

  return result;
}

int getFAT32Entry(struct sFileSystem *fs, unsigned cluster, unsigned *data) {
  /*
   * retrieves FAT32 entry for a cluster number
   */

  int BSOffset, FAT32Offset;

  *data = 0;

  if (fs->FSType == -1) {
    myerror("File system not FAT32!");
    return -1;
  }

  BSOffset =
    fs->bs.BS_RsvdSecCnt * fs->bs.BS_BytesPerSec +
    (int) (cluster * sizeof cluster);
  FAT32Offset = BSOffset % (int) fs->sectorSize;
  if (fs_seek(fs->fd, BSOffset - FAT32Offset, SEEK_SET) == -1) {
    myerror("Seek error!");
    return -1;
  }
  char *go = malloc(fs->sectorSize);
  if (!fs_read(go, 1, fs->sectorSize, fs->fd)) {
    myerror("Failed to read from file!");
    return -1;
  }
  memcpy(data, go + FAT32Offset, sizeof *data);
  *data = *data & 0x0fffffff;
  return 0;

}

int getClusterOffset(struct sFileSystem *fs, unsigned cluster) {
  /*
   * returns the offset of a specific cluster in the data region of the file
   * system
   */

  return (int) (((cluster - 2) * fs->bs.BS_SecPerClus +
      fs->firstDataSector) * fs->sectorSize);

}

int parseEntry(union sDirEntry *de) {
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

int calculateChecksum(char *sname) {
  int len, sum = 0;
  for (len = 11; len != 0; len--)
    sum = ((sum & 1) ? 0x80 : 0 + (sum >> 1) + *sname++) & 0xFF;
  return sum;
}


int openFileSystem(char *path, char *mode, struct sFileSystem *fs) {
  /*
   * opens file system and assemlbes file system information into data
   * structure
   */

  fs->fd = fs_open(path, mode);
  if (!fs->fd) {
    stderror();
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

  fs->FSType = checkFSType(&(fs->bs));
  if (fs->FSType == -1) {
    myerror("File system not FAT32!");
    fs_close(fs->fd);
    return -1;
  }

  if (!fs->bs.BS_FAT32Sz) {
    myerror("32-bit count of sectors must not be zero for FAT32!");
    fs_close(fs->fd);
    return -1;
  }

  fs->FAT32Size = fs->bs.BS_FAT32Sz;

  // check whether count of root dir entries is ok for FAT32
  if (fs->bs.BS_RootEntCnt) {
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

  fs->clusterSize = (unsigned) fs->bs.BS_SecPerClus * fs->bs.BS_BytesPerSec;

  fs->FSSize = (size_t) fs->clusters * fs->clusterSize;

  fs->maxDirEntriesPerCluster = fs->clusterSize / DIR_ENTRY_SIZE;

  fs->maxClusterChainLength = MAX_FILE_LEN / fs->clusterSize;

  unsigned rootDirSectors =
    (fs->bs.BS_RootEntCnt * DIR_ENTRY_SIZE + fs->bs.BS_BytesPerSec -
    1) / fs->bs.BS_BytesPerSec;
  fs->firstDataSector =
    fs->bs.BS_RsvdSecCnt + (fs->bs.BS_NumFAT32s * fs->FAT32Size)
    + rootDirSectors;

  // convert utf 16 le to local charset
  fs->cd = iconv_open("", "UTF-16LE");
  if (fs->cd == (iconv_t) -1) {
    myerror("iconv_open failed!");
    return -1;
  }

  return 0;
}

int closeFileSystem(struct sFileSystem *fs) {
  /*
   * closes file system
   */

  fs_close(fs->fd);
  iconv_close(fs->cd);

  return 0;
}
