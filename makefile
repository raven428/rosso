CC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
LDFLAGS = -static
LDLIBS = -liconv
.RECIPEPREFIX +=

ifdef DEBUG
CFLAGS += -g -DDEBUG=$(DEBUG) -fstack-protector-all
DEBUG_OBJ = mallocv.o
else
LDFLAGS += -s
endif

CFLAGS += -Wall -Wextra -fdiagnostics-color
override CFLAGS+= -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

OBJ = rosso.coff rosso.o FAT_fs.o fileio.o endianness.o entrylist.o errors.o \
  options.o clusterchain.o sort.o misc.o natstrcmp.o stringlist.o

VR = $(shell ./rosso -v)

rosso: $(OBJ) $(DEBUG_OBJ)

%.coff:
  $(WINDRES) rosso.rc $@

release: zip
  hub release create -a rosso-$(VR).zip $(VR)

zip: rosso.exe
  zip rosso-$(VR).zip $< readme.md

clean:
  rm -f *.exe *.o *.zip *.coff
