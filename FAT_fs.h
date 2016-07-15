/*
 * This file contains/describes functions that are used to read, write,
 * check, and use FAT filesystems.
 */

#ifndef __FAT_fs_h__
#define __FAT_fs_h__

// file attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM \
  | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM \
  | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)
// constants for the LDIR structure
#define DE_FREE 0xe5
#define DE_FOLLOWING_FREE 0x00
#define LAST_LONG_ENTRY 0x40U

#define DIR_ENTRY_SIZE 32U

// maximum path len on FAT file systems (above specification)
#define MAX_PATH_LEN 512

// maximum file len
// (specification: file < 4GB which is
// maximum clusters in chain * cluster size)
#define MAX_FILE_LEN 0xFFFFFFFF
#define MAX_DIR_ENTRIES 65536

#include <stdio.h>
#include <iconv.h>

#include <stdint.h>

// Directory entry structures
// Structure for long directory names
struct sLongDirEntry {
  uint8_t LDIR_Ord; // Order of entry in sequence
  char LDIR_Name1[10]; // Chars 1-5 of long name
  uint8_t LDIR_Attr; // Attributes (ATTR_LONG_NAME must be set)
  uint8_t LDIR_Type; // Type
  uint8_t LDIR_Checksum; // Short name checksum
  char LDIR_Name2[12]; // Chars 6-11 of long name
  uint16_t LDIR_FstClusLO; // Zero
  char LDIR_Name3[4]; // Chars 12-13 of long name
};

// Structure for old short directory names
struct sShortDirEntry {
  char DIR_Name[11]; // Short name
  uint8_t DIR_Atrr; // File attributes
  uint8_t DIR_NTRes; // Reserved for NT
  uint8_t DIR_CrtTimeTenth; // Time of creation in ms
  uint16_t DIR_CrtTime; // Time of creation
  uint16_t DIR_CrtDate; // Date of creation
  uint16_t DIR_LstAccDate; // Last access date
  uint16_t DIR_FstClusHI; // Hiword of first cluster
  uint16_t DIR_WrtTime; // Time of last write
  uint16_t DIR_WrtDate; // Date of last write
  uint16_t DIR_FstClusLO; // Loword of first cluster
  uint32_t DIR_FileSize; // file size in bytes
};

union sDirEntry {
  struct sShortDirEntry ShortDirEntry;
  struct sLongDirEntry LongDirEntry;
};

#pragma pack(1)
// First sector = boot sector
struct sBootSector {
  // Jump instruction (to skip over header on boot)
  uint8_t BS_JmpBoot[3];
  char BS_OEMName[8]; // OEM Name (padded with spaces)
  uint16_t BS_BytesPerSec; // Bytes per sector
  uint8_t BS_SecPerClus; // Sectors per cluster
  // Reserved sector count (including boot sector)
  uint16_t BS_RsvdSecCnt;
  uint8_t BS_NumFATs; // Number of file allocation tables
  uint16_t BS_RootEntCnt; // Number of root directory entries
  uint16_t BS_TotSec16; // Total sectors (bits 0-15)
  uint8_t BS_Media; // Media descriptor
  uint16_t BS_FATSz16; // Sectors per file allocation table
  uint16_t BS_SecPerTrk; // Sectors per track
  uint16_t BS_NumHeads; // Number of heads
  uint32_t BS_HiddSec; // Hidden sectors
  uint32_t BS_TotSec32; // Total sectors (bits 16-47)
  uint32_t BS_FATSz32; // Sectors per FAT
  uint16_t BS_ExtFlags; // Flags
  uint16_t BS_FSVer; // Version
  uint32_t BS_RootClus; // Root Directory Cluster
  uint16_t BS_FSInfo; // Sector of FSInfo structure
  // Sector number of the BS copy in reserved sectors
  uint16_t BS_BkBootSec;
  char BS_Reserved[12]; // for future expansion
  char BS_DrvNum; // Physical drive number
  char BS_Reserved1; // Current head
  char BS_BootSig; // ...
  uint32_t BS_VolID;
  char BS_VolLab[11];
  char BS_FilSysType[8];
  uint8_t unused[420]; // unused space in bootsector
  uint16_t BS_EndOfBS; // marks end of bootsector
};
#pragma pack()

// FAT32 FSInfo structure
struct sFSInfo {
  uint32_t FSI_LeadSig;
  uint8_t FSI_Reserved1[480];
  uint32_t FSI_StrucSig;
  uint32_t FSI_Free_Count;
  uint32_t FSI_Nxt_Free;
  uint8_t FSI_Reserved2[12];
  uint32_t FSI_TrailSig;
};

// holds information about the file system
struct sFileSystem {
  FILE *fd;
  uint32_t mode;
  char path[MAX_PATH_LEN + 1];
  struct sBootSector bs;
  int32_t FSType;
  uint32_t clusterCount;
  uint16_t sectorSize;
  uint32_t totalSectors;
  uint32_t clusterSize;
  int32_t clusters;
  uint32_t FATSize;
  uint64_t FSSize;
  uint32_t maxDirEntriesPerCluster;
  uint32_t maxClusterChainLength;
  uint32_t firstDataSector;
  iconv_t cd;
};

// functions

// opens file system and calculates file system information
int32_t openFileSystem(char *path, char *mode, struct sFileSystem *fs);

// sync file system
int32_t syncFileSystem(struct sFileSystem *fs);

// closes file system
int32_t closeFileSystem(struct sFileSystem *fs);

// lazy check if this is really a FAT bootsector
int32_t check_bootsector(struct sBootSector *bs);

// retrieves FAT entry for a cluster number
int32_t getFATEntry(struct sFileSystem *fs, uint32_t cluster, uint32_t *data);

// returns the offset of a specific cluster in the data region of the FS
int32_t getClusterOffset(struct sFileSystem *fs, uint32_t cluster);

// parses one directory entry
int32_t parseEntry(union sDirEntry *de);

// calculate checksum for short dir entry name
int32_t calculateChecksum(char *sname);

// checks whether all FATs have the same content
int32_t checkFATs(struct sFileSystem *fs);

#endif // __FAT_fs_h__
