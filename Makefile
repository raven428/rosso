MAJOR = 1
MINOR = 0
PATCH = 2
CC = x86_64-w64-mingw32-gcc -fdiagnostics-color
LD = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
LDFLAGS = -liconv -static
.RECIPEPREFIX +=

ifdef DEBUG
CFLAGS += -g -DDEBUG=$(DEBUG) -fstack-protector-all
DEBUG_OBJ=mallocv.o
else
LDFLAGS += -s
endif

CFLAGS += -Wall -Wextra
override CFLAGS+= -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

OBJ=rosso.coff rosso.o FAT_fs.o fileio.o endianness.o entrylist.o errors.o \
  options.o clusterchain.o sort.o misc.o natstrcmp.o stringlist.o

rosso.exe: $(OBJ) $(DEBUG_OBJ)
  ${LD} $(OBJ) ${LDFLAGS} $(DEBUG_OBJ) -o $@

%.coff: ver.h
  $(WINDRES) rosso.rc $@

%.h:
  ./ver.sed Makefile > $@

release: rosso-$(MAJOR).$(MINOR).$(PATCH).zip
  hub release create -a $< $(MAJOR).$(MINOR).$(PATCH)

%.zip: rosso.exe
  zip $@ $< readme.md

clean:
  rm -f *.exe *.o *.zip *.coff
