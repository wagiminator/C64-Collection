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
# obfuscate_@_in_files.sh - Bash script for exchanging standard
#                           formatted email addresses:
#                                   <name@domain.example>
#                           in a set of given files within a directory
#                           structure


case "$#" in
    "1" )
	TMPFILE=`mktemp obfuscate.XXXXXXXXXXXXXXXXXXXXXXXX`
	if [ -d $1 ] ; then
	    echo \"$1\" exists and it is a directory
	    egrep -r "<[^@<>]+@[-A-Za-z0-9.]+[.][-A-Za-z0-9]+>" $1 |\
		fgrep -v templates.pot |	# sort out a template file \
		fgrep -v $0 |			# sort out ourself \
		cut -d: -f1 |			# only take the filenames \
		sort -u |			# sort out duplicates \
		xargs file |			# check the real type of a file \
		fgrep text |			# only take real text files into account \
		cut -d: -f1 |			# again get the filenames only \
		xargs -l1 $0 $TMPFILE		# call ourself recursively

		# DEBUG:  xargs -l1 echo Calling \"$0\" with:

	    echo -e \\nAdditional fixes may be needed in \(\*.bak files listing omitted\):\\n
	    egrep -r "[ <][-.A-Za-z0-9]+@[-.A-Za-z0-9]+[ >]" * | grep -v "^[^:]*.bak:"
	else
	    echo \"$1\" does not exist or it is no directory
	fi
	echo -e \\n
	rm --verbose -f $TMPFILE
	;;
    "2" )
	if [ ! -s $1 ] ; then
	    if [ -f $2 ] ; then
		echo argument file \"$2\" does exist and it is a regular file

		# Start searching for addresses to obfuscate,
		# put the result into the temporary file
		sed -r -e "s/<([^@<>]+)@([-A-Za-z0-9.]+)[.]([-A-Za-z0-9]+)>/<\1_(a)_\2_(o)_\3>/g" \
		    -i.bak $2  # get the input file and put the filter result to the output
	    else
		echo argument file \"$2\" does not exist or it is no regular file
	    fi
	else
	    echo temporary file \"$1\" does not exist or it is not empty
	fi
	;;
    "-h" | "--help" | * )
	echo $0 directory
	echo -e \\ndirectory  - start directory for the recursive search for addresses to obfuscate
	;;
esac

