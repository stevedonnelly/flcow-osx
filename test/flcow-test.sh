#!/bin/sh
mac_lib=$(pwd)/../fl-cow/libflcow.dylib
lnx_lib=$(pwd)/../fl-cow/libflcow.so

export DYLD_INSERT_LIBRARIES=${mac_lib}
export DYLD_FORCE_FLAT_NAMESPACE=1
export LD_PRELOAD=${lnx_lib}

./flcow-test

export FLCOW_PATH="^`pwd`"
export FLCOW_EXCLUDE="\.noflcow$"

if [ `uname -s` == "Linux" ]
then
    LINKCHECK="-c %h"
elif [ `uname -s` == "Darwin" ]
then
    LINKCHECK="-f %l"
else
    echo "unknown platform: `uname -s`"
fi

fallocate -l 3G "./large_test_file"
if [ $? -ne 0 ]
then
    ( cat /dev/zero > "./large_test_file" ) & sleep 15 ; kill $!
fi
ln "./large_test_file" "./large_test_link"
ln "./large_test_file" "./large_test_link.noflcow"
stat $LINKCHECK "./large_test_file" "./large_test_link" "./large_test_link.noflcow" > "./link_counts"
links=`echo \`cat "./link_counts"\` | sed s/\ //g`
echo "link counts: [$links], expected: [333]"
if [ $links != "333" ]
then
    echo "FAILED"
fi
touch "./large_test_link"
touch "./large_test_link.noflcow"
stat $LINKCHECK "./large_test_file" "./large_test_link" "./large_test_link.noflcow" > "./link_counts"
links=`echo \`cat "./link_counts"\` | sed s/\ //g`
echo "link counts: [$links], expected: [212]"
if [ $links != "212" ]
then
    echo "FAILED"
fi
rm "./large_test_file" "./large_test_link" "./large_test_link.noflcow" "./link_counts"





