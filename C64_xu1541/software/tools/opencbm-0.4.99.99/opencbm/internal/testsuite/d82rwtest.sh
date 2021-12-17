#!/bin/bash
#
# set -x

function error_info {
	echo "d82rwtest.sh <drivenumber> <cmpbytes> [<d82copy parameters>]" 1>&2
	echo  1>&2
	echo "drivenumber: device ID of the 8250/8050/1001 disk drive to test with" 1>&2
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
rm -f readtest.d82
echo executing: d82copy $* filleddk.d82 $DRIVENO
d82copy $* filleddk.d82 $DRIVENO
echo executing: d82copy $* $DRIVENO readtest.d82
d82copy $* $DRIVENO readtest.d82
  # do only compare up to CMPBYTES bytes
cmp -n $CMPBYTES filleddk.d82 readtest.d82
rm -f readtest.d82
rm -f shelltst.pid
