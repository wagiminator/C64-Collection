#!/bin/bash
#
# set -x

echo $$ > shelltst.pid
for (( i=1; i<=256; ++i )) 
do
	cbmctrl $1 status $2
done
rm -f shelltst.pid
