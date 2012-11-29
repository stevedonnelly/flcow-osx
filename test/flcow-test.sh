#!/bin/sh
mac_lib=$(pwd)/../fl-cow/libflcow.dylib
lnx_lib=$(pwd)/../fl-cow/libflcow.so

export DYLD_INSERT_LIBRARIES=${mac_lib}
export DYLD_FORCE_FLAT_NAMESPACE=1
export LD_PRELOAD=${lnx_lib}

./flcow-test

export FLCOW_PATH="^`pwd`"
export FLCOW_EXCLUDE="\.noflcow$"

fallocate -l 3G "./large_test_file"
if [ $? -ne 0 ]
then
    timeout 15 cat /dev/zero > "./large_test_link"
fi
ln "./large_test_file" "./large_test_link"
ln "./large_test_file" "./large_test_link.noflcow"
stat -c %h "./large_test_file" "./large_test_link" "./large_test_link.noflcow" > "./link_counts"
links=`echo \`cat "./link_counts"\` | sed s/\ //g`
echo "link counts: [$links], expected: [333]"
if [ $links != "333" ]
then
    echo "FAILED"
fi
touch "./large_test_link"
touch "./large_test_link.noflcow"
stat -c %h "./large_test_file" "./large_test_link" "./large_test_link.noflcow" > "./link_counts"
links=`echo \`cat "./link_counts"\` | sed s/\ //g`
echo "link counts: [$links], expected: [212]"
if [ $links != "212" ]
then
    echo "FAILED"
fi
rm "./large_test_file" "./large_test_link" "./large_test_link.noflcow" "./link_counts"





