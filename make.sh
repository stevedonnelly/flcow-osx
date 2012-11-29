#!/bin/bash

if [ `uname -s` == "Linux" ]
then
    COMPILER="gcc"
    CFLAGS="-g -fPIC -O2 -ldl"
    DYLIBFLAGS="-shared -Wl,-soname,libflcow.so.0 -lc -ldl"
elif [ `uname -s` == "Darwin" ]
then
    COMPILER="clang"
    CFLAGS="-g -O2"
    DYLIBFLAGS="-dynamiclib -current_version 1.0 -compatibility_version 1.0 -fvisibility=hidden"
else
    echo "unknown platform: `uname -s`"
fi


function makeObjs {
	$COMPILER $CFLAGS -c fl-cow/fl-cow.c -o fl-cow/fl-cow.o
	$COMPILER $CFLAGS -c test/flcow-test.c -o test/flcow-test.o
}

function makeLibs {
	$COMPILER $DYLIBFLAGS fl-cow/fl-cow.o -o fl-cow/libflcow.so
	ln -f fl-cow/libflcow.so fl-cow/libflcow.dylib
}

function makeBins {
	$COMPILER $CFLAGS fl-cow/fl-cow.o test/flcow-test.o -o test/flcow-test
}

function makeAll {
    makeObjs
    makeLibs
    makeBins
}

function makeTest {
    makeAll
	pushd "test"
	./flcow-test.sh
	popd
}

function makeClean {
	rm -f */*.o
	rm -f fl-cow/libflcow.*
	rm -f test/flcow-test
	rm -f *~
	rm -f */*~
}

case $1 in
    "all" )
        makeAll ;;
    "objs" )
        makeObjs ;;
    "lib" )
        makeObjs
        makeLibs ;;
    "test" )
        makeTest ;;
    "clean" )
        makeClean ;;
esac




