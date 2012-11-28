
COMPILER = gcc
CFLAGS = -g -O2

all: libs
	

objs: fl-cow/fl-cow.c
	${COMPILER} -fPIC ${CFLAGS} -c fl-cow/fl-cow.c -o fl-cow/fl-cow.o

libs: objs
	${COMPILER} -shared -Wl,-soname,libflcow.so.0 fl-cow/fl-cow.o -o fl-cow/libflcow.so -lc -ldl

