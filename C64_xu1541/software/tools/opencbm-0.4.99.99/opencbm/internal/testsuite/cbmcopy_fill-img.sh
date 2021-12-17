#!/bin/bash
#
# set -x

function error_info {
    echo "cbmcopy_fill-img.sh <testfileset> <drivetype> <adapter>" 1>&2
    echo  1>&2
    echo "testfileset: mass | fill"  1>&2
    echo  1>&2
    echo "drivetype:   8250, 1581, 1571, 1541"  1>&2
    echo  1>&2
    echo "adapter:     e.g. xa1541, xum1541 or xum1541:03"  1>&2
    rm -f shelltst.pid
    exit 1
    }

if [ $# -lt 3 ]
then
    error_info
fi

case "$1" in
    mass)
        ./checkNcreateMassData.sh
        ;;
    fill)
        ./checkNcreateTestData.sh
        ;;
    *)
        echo 1>&2
        echo testfileset unknown 1>&2
        echo 1>&2
        error_info
        ;;
esac

ADAPTER="$3"
echo Detecting drive bus ID for required drive type: $2
DRIVENO=`cbmctrl -@"$ADAPTER" detect | fgrep " $2 " | cut -d: -f1 | tail -n 1 | tr -d "[:space:]"`
if [ _ != _$DRIVENO ]
then
    echo $$ > shelltst.pid
    echo using drive $DRIVENO
    TESTDIR="cbmcopy_files_$1"

    case "$2" in
        8250)
            echo "SFD-1001 or 8250 disk drive is not supported yet."
            ;;
        1581)
            echo "1581 disk drive is not supported yet."
            ;;
        1571)
            cbmctrl -@"$ADAPTER" command $DRIVENO "UJ"
            cbmctrl -@"$ADAPTER" command $DRIVENO "U0>H0"
            cbmctrl -@"$ADAPTER" command $DRIVENO "U0>M1"
            # Execute cbmforng in 1571 mode as a dummy to fix some problem
            # with previously executed cbmforng commands in 1541 mode
            cbmforng -@"$ADAPTER" -b 18 -e 18 -s $DRIVENO unformat,uf
            cbmctrl -@"$ADAPTER" command $DRIVENO I
            cbmctrl -@"$ADAPTER" command $DRIVENO "N:LONG1571REFORMAT,LR"
            d64copy -@"$ADAPTER" -2 "$TESTDIR"/00disk"$1"_1571.d71 $DRIVENO
            ;;
        1541)
            cbmforng -@"$ADAPTER" -x -v -o $DRIVENO LONG1541FORMAT,LF
            cbmctrl -@"$ADAPTER" command $DRIVENO I
            BLOCKSFREE=`cbmctrl -@"$ADAPTER" dir $DRIVENO | egrep "^[0-9]+ blocks free[.] *.$" | cut -d" " -f1`
            case $BLOCKSFREE in
                749)    # 40 tracks format (SpeedDOS and alike)
                    FILESPEC="$TESTDIR"/00disk"$1"_1541-40.d64
                    ;;
                664)    # 35 tracks standard format
                    FILESPEC="$TESTDIR"/00disk"$1"_1541-35.d64
                    ;;
                *)      # unknown 1541 disk format
                    echo 1>&2
                    echo 1541 drivetype disk format unkown with $BLOCKSFREE blocks free 1>&2
                    rm -f shelltst.pid
                    exit 1
                    ;;
            esac
            d64copy -@"$ADAPTER" "$FILESPEC" $DRIVENO
            ;;
        *)
            echo 1>&2
            echo drivetype unknown 1>&2
            echo 1>&2
            error_info
            ;;
    esac
    cbmctrl -@"$ADAPTER" status $DRIVENO
    echo transfer ended.
else
    echo drivetype not found or not found beeing active on the bus.
fi
