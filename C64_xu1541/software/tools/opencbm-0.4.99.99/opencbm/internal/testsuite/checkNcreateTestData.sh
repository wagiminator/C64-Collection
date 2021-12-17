#!/bin/bash
#
# set -x

# omitting char that may disturb CVS
# ELIMCHARS="\000\a\b\n\r\\$"

# omitting all control chars as well as the ones disturbig CVS
# ELIMCHARS="[:cntrl:]\\$"

ELIMCHARS='""'
TESTFILEDIR=cbmcopy_files_fill


if ! [ -d $TESTFILEDIR ]
then
    mkdir $TESTFILEDIR
fi

pushd $TESTFILEDIR
if [ _13 != _`ls [0-9][0-9][0-9][0-9]-set-*--.prg 2>/dev/null | wc -l` ]
then
    echo "re-creating cbmcopy test fileset for 1001/8250, 1581, 1571, 1541 (40 tracks as well as 35)"
    rm -f 00diskfill_15?1*.d??
    rm -f [0-9][0-9][0-9][0-9]-set-*--.prg

    echo "creating 1001 only file"
    tr -d $ELIMCHARS < /dev/random | dd bs=256 count=965 of=1001-set-1------.prg 2> /dev/null

    echo "creating 1001/1581 only file"
    tr -d $ELIMCHARS < /dev/random | dd bs=256 count=1817 of=1002-set-18-----.prg 2> /dev/null

    echo "creating 1001/1581/1571 only file"
    tr -d $ELIMCHARS < /dev/random | dd bs=256 count=574 of=1003-set-187----.prg 2> /dev/null

    echo "creating 1001/1581/1571/1541(40) only file"
    tr -d $ELIMCHARS < /dev/random | dd bs=256 count=84 of=1004-set-1874---.prg 2> /dev/null

    echo "creating 1001/1581/1571/1541(40/35) files"
    tr -d $ELIMCHARS < /dev/random | dd bs=256 count=405 of=1005-set-18743--.prg 2> /dev/null

    echo "creating 1001/1581/1571/1541(40/35) files with increasing block sizes"
    for (( i=1, c=1006 ; i<255 ; i*=2, c++ ))
    do
        tr -d $ELIMCHARS < /dev/random | dd bs=$i count=254 of="$c"-set-18743--.prg 2> /dev/null
    done


    c1541 -format 00diskfill_15413,35 d64 00diskfill_1541-35.d64 | fgrep -v 'GetProcAddress cbm_'
    dd bs=1k count=192 if=/dev/zero of=00diskfill_1541-40.d64 2> /dev/null
    c1541 -attach 00diskfill_1541-40.d64 -format 00diskfill_15414,40 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskfill_1571,70 d71 00diskfill_1571.d71 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskfill_1581,80 d81 00diskfill_1581.d81 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskfill_1001,82 d82 00diskfill_1001.d82 | fgrep -v 'GetProcAddress cbm_'


    cat <(echo attach 00diskfill_1001.d82)    <(ls [0-9][0-9][0-9][0-9]-set-1????--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskfill_1581.d81)    <(ls [0-9][0-9][0-9][0-9]-set-?8???--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskfill_1571.d71)    <(ls [0-9][0-9][0-9][0-9]-set-??7??--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskfill_1541-40.d64) <(ls [0-9][0-9][0-9][0-9]-set-???4?--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskfill_1541-35.d64) <(ls [0-9][0-9][0-9][0-9]-set-????3--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
fi
popd
