#!/bin/bash
#
# set -x

function error_info {
	echo "d64rwtest.sh <drivenumber> <cmpbytes> [<d64copy parameters>]" 1>&2
	echo  1>&2
	echo "drivenumber: device ID of the 1541/1571 disk drive to test with" 1>&2
	echo "cmpbytes:    maximum number of bytes to do the compare with" 1>&2
	exit 1
	}

if [ $# -lt 2 ]
then
	error_info
fi

DRIVENO=$1
CMPBYTES=$2

shift
shift

echo $$ > shelltst.pid
rm -f readtest.d64
echo executing: d64copy $* filleddk.d64 $DRIVENO
d64copy $* filleddk.d64 $DRIVENO
echo executing: d64copy $* $DRIVENO readtest.d64
d64copy $* $DRIVENO readtest.d64
  # do only compare up to CMPBYTES bytes
cmp -n $CMPBYTES filleddk.d64 readtest.d64
rm -f readtest.d64
rm -f shelltst.pid
