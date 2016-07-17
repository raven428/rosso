/*
 * This file contains the main function of rosso.
 */

#define __USE_MINGW_ANSI_STDIO 1
#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// project includes
#include "errors.h"
#include "FAT32.h"
#include "options.h"
#include "rosso.h"
#include "sort.h"

int printFSInfo(char *filename) {
  /*
   * print file system information
   */

  unsigned value;

  struct sFileSystem fs;

  if (openFileSystem(filename, "rb", &fs)) {
    myerror("Failed to open file system!");
    return -1;
  }

  printf("Sector size: %d bytes\n"
    "FAT32 size: %d sectors (%d bytes)\n"
    "Number of FAT32s: %d %s\n"
    "Cluster size: %d bytes\n"
    "Max. cluster chain length: %d clusters\n"
    "Data clusters: %d\n"
    "FS size: %llu MiBytes\n", fs.sectorSize, fs.FAT32Size,
    fs.FAT32Size * fs.sectorSize, fs.bs.BS_NumFAT32s,
    checkFAT32s(&fs) ? "different" : "same", fs.clusterSize,
    fs.maxClusterChainLength, fs.clusters, fs.FSSize >> 20);

  if (fs.FSType != -1) {
    if (getFAT32Entry(&fs, fs.bs.BS_RootClus, &value) == -1) {
      myerror("Failed to get FAT32 entry!");
      closeFileSystem(&fs);
      return -1;
    }

    printf("FAT32 root first cluster: %#x\n"
      "First cluster data offset: %#x\n"
      "First cluster FAT32 entry: %#x\n", fs.bs.BS_RootClus,
      getClusterOffset(&fs, fs.bs.BS_RootClus), value);
  }

  closeFileSystem(&fs);

  return 0;

}

int main(int argc, char *argv[]) {
  /*
   * parse arguments and options and start sorting
   */

  // initialize rng
  srand((unsigned) time(0));

  // use locale from environment
  if (!setlocale(LC_ALL, "")) {
    myerror("Could not set locale!");
    return -1;
  }

  char *filename;

  if (parse_options(argc, argv) == -1) {
    myerror("Failed to parse options!");
    return -1;
  }

  // program information
  if (OPT_HELP) {
    printf("SYNOPSIS\n"
      "  rosso [OPTIONS] DEVICE\n"
      "\n"
      "DESCRIPTION\n"
      "  Rosso sorts directory structures of FAT32 file systems. Many hardware\n"
      "  players do not sort files automatically but play them in the order they\n"
      "  were transferred to the device. Rosso can help here.\n"
      "\n"
      "OPTIONS\n"
      "  -a    Use ASCIIbetical order for sorting\n"
      "  -c    Ignore case of file names\n"
      "  -i    Print file system information only\n"
      "  -l    Print current order of files only\n"
      "  -m    Print more information\n"
      "  -n    Natural order sorting\n"
      "  -r    Sort in reverse order\n"
      "  -R    Sort in random order\n"
      "  -t    Sort by last modification date and time\n"
      "  -h, --help    Print some help\n"
      "  -v, --version    Print version information\n"
      "  -I PFX    Ignore file name PFX\n"
      "  -o FLAG    Sort order of files where FLAG is one of:\n"
      "    d    Directories first (default)\n"
      "    f    Files first\n"
      "    a    Files and directories are not differentiated\n"
      "\n"
      "  The following options can be specified multiple times:\n"
      "\n"
      "  -d DIR    Sort directory DIR only\n"
      "  -D DIR    Sort directory DIR and all subdirectories\n"
      "  -x DIR    Do not sort directory DIR\n"
      "  -X DIR    Do not sort directory DIR and its subdirectories\n"
      "\n"
      "EXAMPLES\n"
      "  rosso -l F:\n"
      "  rosso F:\n"
      "  rosso -l -d / F:\n"
      "  rosso -d / F:\n"
      "\n"
      "NOTES\n"
      "  DEVICE must be a FAT32 file system.\n"
      "  WARNING: THE FILESYSTEM MUST BE CONSISTENT, OTHERWISE YOU MAY DAMAGE IT!\n"
      "  IF SOMEONE ELSE HAS ACCESS TO THE DEVICE HE MIGHT EXPLOIT ROSSO WITH A\n"
      "  FORGED CORRUPT FILESYSTEM! USE THIS PROGRAM AT YOUR OWN RISK!\n");
    return 0;
  }
  if (OPT_VERSION) {
    printf("%d.%d.%d\n", MAJOR, MINOR, PATCH);
    return 0;
  }
  if (optind < argc - 1) {
    myerror("Too many arguments!");
    myerror("Use -h for more help.");
    return -1;
  }
  if (optind == argc) {
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
