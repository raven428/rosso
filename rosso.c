/*
 * This file contains the main function of rosso.
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
"  -m    Print more information\n" \
"  -n    Natural order sorting\n" \
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
"  DEVICE must be a FAT32 file system.\n" \
"  WARNING: THE FILESYSTEM MUST BE CONSISTENT, OTHERWISE YOU MAY DAMAGE IT!\n" \
"  IF SOMEONE ELSE HAS ACCESS TO THE DEVICE HE MIGHT EXPLOIT ROSSO WITH A\n" \
"  FORGED CORRUPT FILESYSTEM! USE THIS PROGRAM AT YOUR OWN RISK!\n"

int32_t printFSInfo(char *filename) {
  /*
   * print file system information
   */

  assert(filename != NULL);

  uint32_t value;

  struct sFileSystem fs;

  if (openFileSystem(filename, FS_MODE_RO, &fs)) {
    myerror("Failed to open file system!");
    return -1;
  }

  printf("Device: %s\nType: FAT%d\nSector size: %d bytes\n"
    "FAT size: %d sectors (%d bytes)\nNumber of FATs: %d %s\n"
    "Cluster size: %d bytes\nMax. cluster chain length: %d clusters\n"
    "Data clusters: %d\nFS size: %d MiBytes\n", fs.path, fs.FATType,
    fs.sectorSize, fs.FATSize, fs.FATSize * fs.sectorSize, fs.bs.BS_NumFATs,
    checkFATs(&fs) ? "different" : "same", fs.clusterSize,
    fs.maxClusterChainLength, fs.clusters, (int) (fs.FSSize >> 20));

  if (fs.FATType == FATTYPE_FAT32) {
    if (getFATEntry(&fs, fs.bs.BS_RootClus, &value) == -1) {
      myerror("Failed to get FAT entry!");
      closeFileSystem(&fs);
      return -1;
    }

    printf("FAT32 root first cluster: 0x%x\nFirst cluster data offset: 0x%x\n"
      "First cluster FAT entry: 0x%x\n", fs.bs.BS_RootClus,
      (int) getClusterOffset(&fs, fs.bs.BS_RootClus), value);
  }

  closeFileSystem(&fs);

  return 0;

}

int main(int argc, char *argv[]) {
  /*
   * parse arguments and options and start sorting
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
  }
  else if (OPT_VERSION) {
    printf("%d.%d.%d\n", MAJOR, MINOR, PATCH);
    return 0;
  }
  else if (optind < argc - 1) {
    myerror("Too many arguments!");
    myerror("Use -h for more help.");
    return -1;
  }
  else if (optind == argc) {
    myerror("Device must be given!");
    myerror("Use -h for more help.");
    return -1;
  }

  filename = argv[optind];

  if (OPT_INFO) {
    if (printFSInfo(filename) == -1) {
      myerror("Failed to print file system information");
      return -1;
    }
  }
  else {
    if (sortFileSystem(filename) == -1) {
      myerror("Failed to sort file system!");
      return -1;
    }
  }

  freeOptions();

  return 0;
}
