#!/bin/sh
#
# Copyright (C) 2006 Wolfgang Moser
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
#
# distclean_by_cvsignore.sh - Search the OpenCBM file structure
#                             for .cvsignore files and delete
#                             all files and directories listed there.
#
# This is a pretty dangerous job, so watch out!!!
#

	# parameter 1 has to be a path to a .cvsignore file
function parse_cvsignore {
	/bin/echo "updwntst.sh <drivenumber>" 1>&2
	/bin/echo  1>&2
	/bin/echo "drivenumber: device ID of the 1541/1571 disk drive to test with"  1>&2
	exit 1
	}


if ( [ $# -eq 1 ] && [ $1==.ignore ] )
then
	SUBPATH=`echo $1 | sed "s/^\(.*\)\.cvsignore$/\1/g"`
	# echo handling $1 in $SUBPATH with:
	# /bin/cat $1 | /bin/tr -d "\r" | /bin/xargs -i /bin/find "$SUBPATH" -maxdepth 1 -name '{}' | /bin/sort -u
	/bin/cat $1 | /bin/tr -d "\r" | /bin/xargs -i /bin/find "$SUBPATH" -maxdepth 1 -name '{}' | /bin/sort -u | /bin/grep "^\.\./\.\./"| /bin/xargs /bin/rm -rv
else
	if [ $# -eq 0 ]
	then
		/bin/find ../../ -name '.cvsignore' | /bin/xargs -l1 $0
	else
		echo $0
		echo
		echo Searches all .cvsignore files from ../../ and deletes files
		echo as well as directories listed within. No parameters allowed.
	fi
fi
