#!/bin/bash

arch=`arch`

# check if we have test data for $arch yet
if [[ ! -d ./testdata/$arch ]]; then
    echo "No tests for $arch"
    exit 1
fi

FAILED=0
# execute lshw in an appropriate chroot for each of the tests
# CPU data

for dir in ./testdata/`arch`/*; do
    testdir=`basename $dir`
    mkdir -p ./testdata/`arch`/$testdir/usr/sbin/
    ln -s `pwd`/src/lshw ./testdata/`arch`/$testdir/usr/sbin/lshw
    fakeroot fakechroot -e null chroot ./testdata/$arch/$testdir \
        /usr/sbin/lshw -xml -C cpu > ./testdata/`arch`/$testdir/cpu.xml
    # strip off the header
    sed -i '/^<!--/ d' ./testdata/$arch/$testdir/cpu.xml
    diff ./testdata/$arch/$testdir/expected-cpu.xml \
        ./testdata/$arch/$testdir/cpu.xml
    if [[ $? -eq 0 ]]; then
        echo "`arch` ($testdir): Test passed"
        rm ./testdata/`arch`/$testdir/cpu.xml
    else
        echo "`arch` ($testdir): Test failed"
        FAILED=1
    fi

    # clean up
    rm -r ./testdata/`arch`/$testdir/usr/
    # next test
done

if [[ "$FAILED" -eq 1 ]]; then
    echo "Failed tests"
    exit 1
fi
