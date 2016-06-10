/*
  This file contains the main function of rosso.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <time.h>

// project includes
#include "FAT_fs.h"
#include "options.h"
#include "errors.h"
#include "sort.h"
#include "clusterchain.h"
#include "misc.h"
#include "rosso.h"

// program information
#define INFO_OPTION_HELP \
"SYNOPSIS\n" \
"  rosso [OPTIONS] DEVICE\n" \
"\n" \
"DESCRIPTION\n" \
"  Rosso sorts directory structures of FAT file systems. Many hardware\n" \
"  players do not sort files automatically but play them in the order they\n" \
"  were transferred to the device. Rosso can help here.\n" \
"\n" \
"OPTIONS\n" \
"  -a    Use ASCIIbetical order for sorting\n" \
"  -c    Ignore case of file names\n" \
"  -i    Print file system information only\n" \
"  -l    Print current order of files only\n" \
"  -n    Natural order sorting\n" \
"  -q    Be quiet\n" \
"  -r    Sort in reverse order\n" \
"  -R    Sort in random order\n" \
"  -t    Sort by last modification date and time\n" \
"  -h, --help    Print some help\n" \
"  -v, --version    Print version information\n" \
"  -I=PFX    Ignore file name PFX\n" \
"  -o=FLAG    Sort order of files where FLAG is one of:\n" \
"    d    Directories first (default)\n" \
"    f    Files first\n" \
"    a    Files and directories are not differentiated\n" \
"\n" \
"  The following options can be specified multiple times:\n" \
"\n" \
"  -d=DIR    Sort directory DIR only\n" \
"  -D=DIR    Sort directory DIR and all subdirectories\n" \
"  -x=DIR    Do not sort directory DIR\n" \
"  -X=DIR    Do not sort directory DIR and its subdirectories\n" \
"\n" \
"EXAMPLES\n" \
"  rosso -l F:\n" \
"  rosso F:\n" \
"\n" \
"NOTES\n" \
"  DEVICE must be a FAT12, FAT16 or FAT32 file system.\n" \
"  WARNING: THE FILESYSTEM MUST BE CONSISTENT, OTHERWISE YOU MAY DAMAGE IT!\n" \
"  IF SOMEONE ELSE HAS ACCESS TO THE DEVICE HE MIGHT EXPLOIT ROSSO WITH A\n" \
"  FORGED CORRUPT FILESYSTEM! USE THIS PROGRAM AT YOUR OWN RISK!\n"

int32_t printFSInfo(char *filename) {
/*
  print file system information
*/

  assert(filename != NULL);

  uint32_t value, clen;
  int32_t usedClusters, badClusters;
  int32_t i;
  struct sClusterChain *chain;

  struct sFileSystem fs;

  printf("\t- File system information -\n");

  if (openFileSystem(filename, FS_MODE_RO, &fs)) {
    myerror("Failed to open file system!");
    return -1;
  }

  usedClusters=0;
  badClusters=0;
  for (i=2; i<fs.clusters+2; i++) {
    getFATEntry(&fs, i, &value);
    if ((value & 0x0FFFFFFF) != 0) usedClusters++;
    if (fs.FATType == FATTYPE_FAT32) {
      if ((value & 0x0FFFFFFF) == 0x0FFFFFF7) badClusters++;
    } else if (fs.FATType == FATTYPE_FAT16) {
      if (value == 0x0000FFF7) badClusters++;
    } else if (fs.FATType == FATTYPE_FAT12) {
      if (value == 0x00000FF7) badClusters++;
    }
  }

  printf("Device:\t\t\t\t\t%s\n", fs.path);
  printf("Type:\t\t\t\t\tFAT%d\n", (int) fs.FATType);
  printf("Sector size:\t\t\t\t%d bytes\n", (int) fs.sectorSize);
  printf("FAT size:\t\t\t\t%d sectors (%d bytes)\n", (int) fs.FATSize,
    (int) (fs.FATSize * fs.sectorSize));
  printf("Number of FATs:\t\t\t\t%d %s\n", fs.bs.BS_NumFATs,
    (checkFATs(&fs) ? "- WARNING: FATs are different!" : ""));
  printf("Cluster size:\t\t\t\t%d bytes\n", (int) fs.clusterSize);
  printf("Max. cluster chain length:\t\t%d clusters\n",
    (int) fs.maxClusterChainLength);
  printf("Data clusters (total / used / bad):\t%d / %d / %d\n",
    (int) fs.clusters, (int) usedClusters, (int) badClusters);
  printf("FS size:\t\t\t\t%.2f MiBytes\n", (float) fs.FSSize / (1024.0*1024));
  if (fs.FATType == FATTYPE_FAT32) {
    if (getFATEntry(&fs, fs.bs.FATxx.FAT32.BS_RootClus, &value) == -1) {
      myerror("Failed to get FAT entry!");
      closeFileSystem(&fs);
      return -1;
    }
    printf("FAT32 root first cluster:\t\t0x%x\n"
    "First cluster data offset:\t\t0x%lx\nFirst cluster FAT entry:\t\t0x%x\n",
    (unsigned int) fs.bs.FATxx.FAT32.BS_RootClus,
    (unsigned long) getClusterOffset(&fs, fs.bs.FATxx.FAT32.BS_RootClus),
    (unsigned int) value);
  } else if (fs.FATType == FATTYPE_FAT12) {
    printf("FAT12 root directory Entries:\t\t%u\n", fs.bs.BS_RootEntCnt);
  } else if (fs.FATType == FATTYPE_FAT16) {
    printf("FAT16 root directory Entries:\t\t%u\n", fs.bs.BS_RootEntCnt);
  }

  if (OPT_MORE_INFO) {
    printf("\n\t- FAT -\n");
    printf("Cluster \tFAT entry\tChain length\n");
    for (i=0; i<fs.clusters+2; i++) {
      getFATEntry(&fs, i, &value);

      clen=0;
      if ((value & 0x0FFFFFFF ) != 0) {
        if ((chain=newClusterChain()) == NULL) {
          myerror("Failed to generate new ClusterChain!");
          return -1;
        }
        clen=getClusterChain(&fs, i, chain);
        freeClusterChain(chain);
      }

      printf("%08x\t%08x\t%u\n", i, value, clen);

    }

  }

  closeFileSystem(&fs);

  return 0;

}

int main(int argc, char *argv[]) {
/*
  parse arguments and options and start sorting
*/

  // initialize rng
  srand(time(0));

  // use locale from environment
  if (setlocale(LC_ALL, "") == NULL) {
    myerror("Could not set locale!");
    return -1;
  }

  char *filename;

  if (parse_options(argc, argv) == -1) {
    myerror("Failed to parse options!");
    return -1;
  }

  if (OPT_HELP) {
    printf(INFO_OPTION_HELP);
    return 0;
  } else if (OPT_VERSION) {
    printf("%d.%d.%d\n", MAJOR, MINOR, PATCH);
    return 0;
  } else if (optind < argc -1) {
    myerror("Too many arguments!");
    myerror("Use -h for more help.");
    return -1;
  } else if (optind == argc) {
    myerror("Device must be given!");
    myerror("Use -h for more help.");
    return -1;
  }

  filename=argv[optind];

  if (OPT_INFO) {
    //infomsg(INFO_HEADER "\n\n");
    if (printFSInfo(filename) == -1) {
      myerror("Failed to print file system information");
      return -1;
    }
  } else {
    //infomsg(INFO_HEADER "\n\n");
    if (sortFileSystem(filename) == -1) {
      myerror("Failed to sort file system!");
      return -1;
    }
  }
  
  freeOptions();

  return 0;
}
