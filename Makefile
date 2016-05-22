MAJOR = 1
MINOR = 0
PATCH = 1
CC = x86_64-w64-mingw32-gcc -fdiagnostics-color
LD = x86_64-w64-mingw32-gcc
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

OBJ=rosso.o FAT_fs.o fileio.o endianness.o entrylist.o errors.o options.o \
  clusterchain.o sort.o misc.o natstrcmp.o stringlist.o

rosso.exe: $(OBJ) $(DEBUG_OBJ)
  ${LD} $(OBJ) ${LDFLAGS} $(DEBUG_OBJ) -o $@

rosso.o: rosso.c endianness.h FAT_fs.h platform.h options.h \
 stringlist.h errors.h sort.h clusterchain.h misc.h mallocv.h ver.h
  $(CC) ${CFLAGS} -c $< -o $@

FAT_fs.o: FAT_fs.c FAT_fs.h platform.h errors.h endianness.h fileio.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

fileio.o: fileio.c fileio.h platform.h
  $(CC) ${CFLAGS} -c $< -o $@

endianness.o: endianness.c endianness.h mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

entrylist.o: entrylist.c entrylist.h FAT_fs.h platform.h options.h \
 stringlist.h errors.h natstrcmp.h mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

errors.o: errors.c errors.h mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

options.o: options.c options.h platform.h FAT_fs.h stringlist.h errors.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

clusterchain.o: clusterchain.c clusterchain.h platform.h errors.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

sort.o: sort.c sort.h FAT_fs.h platform.h clusterchain.h entrylist.h \
 errors.h options.h stringlist.h endianness.h misc.h fileio.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

misc.o: misc.c misc.h options.h platform.h FAT_fs.h stringlist.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

natstrcmp.o: natstrcmp.c natstrcmp.h mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

stringlist.o: stringlist.c stringlist.h platform.h FAT_fs.h errors.h \
 mallocv.h
  $(CC) ${CFLAGS} -c $< -o $@

mallocv.o: mallocv.c mallocv.h errors.h
  $(CC) ${CFLAGS} -c $< -o $@

ver.h: Makefile
  ./ver.sed $< > $@

release: rosso-$(MAJOR).$(MINOR).$(PATCH).zip
  hub release create -a $< $(MAJOR).$(MINOR).$(PATCH)

%.zip: rosso.exe
  zip $@ $< readme.md

clean:
  rm -f *.exe *.o *.zip ver.h
