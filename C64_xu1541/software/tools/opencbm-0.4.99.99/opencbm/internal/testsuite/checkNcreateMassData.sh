#!/bin/bash
#
# set -x

# omitting char that may disturb CVS
# ELIMCHARS="\000\a\b\n\r\\$"

# omitting all control chars as well as the ones disturbig CVS
# ELIMCHARS="[:cntrl:]\\$"

ELIMCHARS='""'
TESTFILEDIR=cbmcopy_files_mass


function createXblocksFile {
    # Params: <pathNfilename> <numofblocks>

    # this doesn't work, sometimes files get to small
    # COUNT=`echo "  $2 * 254 - 1 " | bc`
    # tr -d $ELIMCHARS < /dev/random | dd bs=$COUNT count=1 > "$1" 2> /dev/null

    COUNT=`echo " ( $2 * 254 - 1 ) / 512 " | bc`
    tr -d $ELIMCHARS < /dev/random | dd bs=512 count=$COUNT  > "$1" 2> /dev/null
    COUNT=`echo " ( $2 * 254 - 1 ) % 512 " | bc`
    tr -d $ELIMCHARS < /dev/random | dd   bs=1 count=$COUNT >> "$1" 2> /dev/null
    }

function createXsizedFile {
    # Params: <pathNfilename> <size>
    tr -d $ELIMCHARS < /dev/random | dd  bs=1 count=$2 > "$1" 2> /dev/null
    }

if ! [ -d $TESTFILEDIR ]
then
    mkdir $TESTFILEDIR
fi

pushd $TESTFILEDIR
if [ _624 != _`ls [0-9][0-9][0-9][0-9]-set-*--.prg 2>/dev/null | wc -l` ]
then
    echo "re-creating cbmcopy test fileset for 1001/8250, 1581, 1571, 1541 (40 tracks as well as 35)"
    rm -f 00diskmass_15?1*.d??
    rm -f [0-9][0-9][0-9][0-9]-set-*--.prg


    # Availability: DIRENTRIES BLOCKS     BLOCKS/DIRENTRIES
    # -----------------------------------------------
    # D82                  296   4133 = 102 + 204 * 19  BLOCKS
    #                                 ==>   3138/296
    #
    # D81                  296   3160 = 102 + 276 *  9  BLOCKS
    #                                 ==>   3138/296
    #
    # D71                  144   1328 = 102 + 124 *  9  BLOCKS
    #                                 ==>   1218/144
    #
    # D64-40               144    749 = 102 + 124 *  4  BLOCKS
    #                                 ==>    598/144
    #
    # D64-35               144    664 = 102 + 124 *  4  BLOCKS
    #                                 ==>    598/144


        # Test a block size of 0 bytes, which results in a cbmcopy error.


        # 1001 fill names: 1001-set-1------
    echo "creating 1001 only files"
    NINETEENBLOCKS=1001-set-1------.prg
    createXblocksFile $NINETEENBLOCKS 19

        # 1581/1571 fill names: 1002-set--87----
    echo "creating 1581/1571 only files"
    NINE____BLOCKS=1002-set--87----.prg
    createXblocksFile $NINE____BLOCKS 9

        # 1541(35/40) fill names: 1003-set----43--
    echo "creating 1541(35/40) only files"
    FOUR____BLOCKS=1003-set----43--.prg
    createXblocksFile $FOUR____BLOCKS  4



        # create zero length test file
    echo "creating zero length test file"
    # touch 1004-set-18743--.prg
    createXsizedFile 1004-set-18743--.prg 1

        # create off-by-one test files pattern
    echo "creating off-by-one test file"
    createXsizedFile 1005-set-18743--.prg 1

    size=126
    for (( i=1006; i<1024; ))
    do
        for (( j=0; j<3; j++ ))
        do
            createXsizedFile $i-set-18743--.prg `echo $size + $j | bc`
            i=`echo $i + 1 | bc`;
        done
        size=`echo " $size * 2 + 1 " | bc`
    done

        # 204 * 19 blocks for 1001 fillup
    echo "204 * 19 blocks for 1001 fillup"
    for (( i=1024; i<1227; i++ ))
    do
        cp $NINETEENBLOCKS $i-set-1------.prg
    done

        # 152 *  9 blocks for 1581-only fillup
    echo "152 *  9 blocks for 1581-only fillup"
    for (( i=1228; i<1380; i++ ))
    do
        cp $NINE____BLOCKS $i-set--8-----.prg
    done

        # 124 *  9 blocks for 1581/1571 fillup
    echo "124 *  9 blocks for 1581/1571 fillup"
    for (( i=1380; i<1503; i++ ))
    do
        cp $NINE____BLOCKS $i-set--87----.prg
    done

        # 124 *  4 blocks for 1541 (35/40) fillup
    echo "124 *  4 blocks for 1541 (35/40) fillup"
    for (( i=1503; i<1626; i++ ))
    do
        cp $FOUR____BLOCKS $i-set----43--.prg
    done

    c1541 -format 00diskmass_15413,35 d64 00diskmass_1541-35.d64 | fgrep -v 'GetProcAddress cbm_'
    dd bs=1k count=192 if=/dev/zero of=00diskmass_1541-40.d64 2> /dev/null
    c1541 -attach 00diskmass_1541-40.d64 -format 00diskmass_15414,40 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskmass_1571,70 d71 00diskmass_1571.d71 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskmass_1581,80 d81 00diskmass_1581.d81 | fgrep -v 'GetProcAddress cbm_'
    c1541 -format 00diskmass_1001,82 d82 00diskmass_1001.d82 | fgrep -v 'GetProcAddress cbm_'

    cat <(echo attach 00diskmass_1001.d82)    <(ls [0-9][0-9][0-9][0-9]-set-1????--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskmass_1581.d81)    <(ls [0-9][0-9][0-9][0-9]-set-?8???--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskmass_1571.d71)    <(ls [0-9][0-9][0-9][0-9]-set-??7??--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskmass_1541-40.d64) <(ls [0-9][0-9][0-9][0-9]-set-???4?--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
    cat <(echo attach 00diskmass_1541-35.d64) <(ls [0-9][0-9][0-9][0-9]-set-????3--.prg | sed 's/^/write /') | c1541 | fgrep -v 'GetProcAddress cbm_'
fi
popd
