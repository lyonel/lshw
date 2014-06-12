#!/bin/bash

arch=`arch`
FAILED=0

# CPU data
cputest="./testdata/cpu/$arch"

# check if we have CPU test data for $arch yet
if [[ ! -d "$cputest" ]]; then
    echo "No CPU tests for $arch"
    exit 0
fi

# execute lshw in an appropriate chroot for each of the tests
for dir in $cputest/*; do
    testdir=`basename $dir`
    mkdir -p $cputest/$testdir/usr/sbin/
    ln -s `pwd`/src/lshw $cputest/$testdir/usr/sbin/lshw
    fakeroot fakechroot -e null chroot $cputest/$testdir \
        /usr/sbin/lshw -xml -C cpu > $cputest/$testdir/cpu.xml
    # strip off the header
    sed -i '/^<!--/ d' $cputest/$testdir/cpu.xml
    diff $cputest/$testdir/expected-cpu.xml \
        $cputest/$testdir/cpu.xml
    if [[ $? -eq 0 ]]; then
        echo "`arch` CPU - ($testdir): Test passed"
        rm $cputest/$testdir/cpu.xml
    else
        echo "`arch` CPU - ($testdir): Test failed"
        FAILED=1
    fi

    # clean up
    rm -r $cputest/$testdir/usr/
    # next test
done

# Disks
disktest="./testdata/disks/$arch"
# check if we have disk test data for $arch yet
if [[ ! -d "$disktest" ]]; then
    echo "No disk tests for $arch"
    exit 0
fi

for dir in $disktest/*; do
    testdir=`basename $dir`
    mkdir -p $disktest/$testdir/usr/sbin/
    ln -s `pwd`/src/lshw $disktest/$testdir/usr/sbin/lshw
    fakeroot fakechroot -e null chroot $disktest/$testdir \
        /usr/sbin/lshw -xml -C disk > $disktest/$testdir/disk.xml
    # strip off the header
    sed -i '/^<!--/ d' $disktest/$testdir/disk.xml
    diff $disktest/$testdir/expected-disk.xml \
        $disktest/$testdir/disk.xml
    if [[ $? -eq 0 ]]; then
        echo "`arch` disk - ($testdir): Test passed"
        rm $disktest/$testdir/disk.xml
    else
        echo "`arch` disk - ($testdir): Test failed"
        FAILED=1
    fi

    # clean up
    rm -r $disktest/$testdir/usr/
    # next test
done

if [[ "$FAILED" -eq 1 ]]; then
    echo "Failed tests"
    exit 1
fi
