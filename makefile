ifdef INCLUDE
  CC = include-what-you-use
  CFLAGS = -isystem C:/cygwin64/usr/x86_64-w64-mingw32/sys-root/mingw/include \
  -w -ferror-limit=1 -Xiwyu --mapping_file=t/mingw64.imp
else
  CC = x86_64-w64-mingw32-gcc
  CFLAGS = -O -Wall -Wextra -Wconversion -pedantic -std=c11 \
  -fdiagnostics-color -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

WINDRES = x86_64-w64-mingw32-windres
LDFLAGS = -static -s
LDLIBS = -liconv
.RECIPEPREFIX +=

VR = $(shell ./rosso -v)

rosso: rosso.coff FAT32.o fileio.o entrylist.o errors.o options.o \
  clusterchain.o sort.o natstrcmp.o stringlist.o

%.coff:
  $(WINDRES) rosso.rc $@

release: zip
  hub release create -a rosso-$(VR).zip $(VR)

zip: rosso readme.txt
  zip rosso-$(VR).zip $<.exe readme.txt

%.txt:
  ln -f $*.md $@

clean:
  $(RM) -v *.exe *.o *.zip *.coff *.txt
