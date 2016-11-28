ifdef INCLUDE
  CC = include-what-you-use
  CFLAGS = -isystem C:/cygwin64/usr/x86_64-w64-mingw32/sys-root/mingw/include \
  -w -ferror-limit=1 -Xiwyu --mapping_file=t/mingw64.imp
else
  CC = x86_64-w64-mingw32-gcc
  CFLAGS = -O -Wall -Wextra -Wconversion -pedantic -std=c11 \
  -fdiagnostics-color
endif

WINDRES = x86_64-w64-mingw32-windres
LDFLAGS = -static -s
LDLIBS = -liconv
.RECIPEPREFIX +=

rosso: rosso.coff FAT32.o fileio.o entrylist.o errors.o options.o \
  clusterchain.o sort.o natstrcmp.o stringlist.o

%.coff:
  $(WINDRES) $*.rc $@
