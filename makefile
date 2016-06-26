CC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
LDFLAGS = -static -s
LDLIBS = -liconv
.RECIPEPREFIX +=

CFLAGS = -O -Wall -Wextra -pedantic -std=c11 -fdiagnostics-color \
  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

VR = $(shell ./rosso -v)

rosso: rosso.coff rosso.o FAT_fs.o fileio.o entrylist.o errors.o options.o \
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
