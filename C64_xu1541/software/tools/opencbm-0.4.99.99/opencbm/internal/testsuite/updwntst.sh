#!/bin/bash
#
# set -x

function error_info {
	echo "updwntst.sh <OpenCBM plugin:port> <drivenumber>" 1>&2
	echo  1>&2
	echo "drivenumber: device ID of the 1541/1571 disk drive to test with"  1>&2
	exit 1
	}

if [ $# -lt 1 ]
then
	error_info
fi

PLUGINPORT=$1
DRIVENO=$2

shift

echo $$ > shelltst.pid

echo Doing up-/download test 1 at 0x300...0x5ff
rm -f dload300.bin
dd if=/dev/random bs=256 count=3 of=upload300.bin 2> /dev/null
cbmctrl $PLUGINPORT upload   $DRIVENO 0x0300 upload300.bin
cbmctrl $PLUGINPORT download $DRIVENO 0x0300 0x0300 dload300.bin
cmp upload300.bin dload300.bin
rm -f upload300.bin

# do a ROM up- and download with dummy data, don't
# check that transmitted data for validness

echo "Doing ROM upload test (faked) at 0x8000...0xffff"
dd if=/dev/random bs=512 count=64 2> /dev/null | cbmctrl $PLUGINPORT upload $DRIVENO 0x8000 -
echo "Doing ROM download test from 0x8000...0xffff"
cbmctrl $PLUGINPORT download $DRIVENO 0x8000 0x8000 - > /dev/null

# do the same with other content to ensure that
# we don't compatre against preconfigured data

echo Doing up-/download test 2 at 0x300...0x5ff
rm -f dload300.bin
dd if=/dev/random bs=256 count=3 of=u2load300.bin 2> /dev/null
cbmctrl $PLUGINPORT upload   $DRIVENO 0x0300        u2load300.bin
cbmctrl $PLUGINPORT download $DRIVENO 0x0300 0x0300  dload300.bin
cmp u2load300.bin dload300.bin
rm -f u2load300.bin
rm -f dload300.bin

rm -f shelltst.pid
