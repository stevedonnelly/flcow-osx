
COMPILER = gcc
CFLAGS = -g -O2 -D_LARGEFILE64_SOURCE

all: libs bins
	

objs: fl-cow/fl-cow.c test/flcow-test.c
	${COMPILER} -fPIC ${CFLAGS} -c fl-cow/fl-cow.c -o fl-cow/fl-cow.o
	${COMPILER} -fPIC ${CFLAGS} -c test/flcow-test.c -o test/flcow-test.o

libs: objs
	${COMPILER} -shared -Wl,-soname,libflcow.so.0 fl-cow/fl-cow.o -o fl-cow/libflcow.so -lc -ldl
	ln -f fl-cow/libflcow.so fl-cow/libflcow.dylib

bins: objs
	${COMPILER} fl-cow/fl-cow.o test/flcow-test.o -o test/flcow-test -ldl

test: all
	pushd "test"; ./flcow-test.sh; popd


