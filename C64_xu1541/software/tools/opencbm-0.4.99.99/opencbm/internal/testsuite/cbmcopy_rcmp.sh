#!/bin/bash
#
# set -x

function error_info {
    echo "cbmcopy_rcmp.sh <testfileset> <drivetype> <adapter> [<cbmcopy parameters>]" 1>&2
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
            XFILEPAT='[0-9][0-9][0-9][0-9]-set-1....--\.prg'
            ;;
        1581)
            XFILEPAT='[0-9][0-9][0-9][0-9]-set-.8...--\.prg'
            ;;
        1571)
            cbmctrl -@"$ADAPTER" command $DRIVENO "UJ"
            cbmctrl -@"$ADAPTER" command $DRIVENO "U0>H0"
            cbmctrl -@"$ADAPTER" command $DRIVENO "U0>M1"
            XFILEPAT='[0-9][0-9][0-9][0-9]-set-..7..--\.prg'
            ;;
        1541)
            BLOCKSFREE=`cbmctrl -@"$ADAPTER" dir $DRIVENO | \
                        egrep '^[0-9]+ *(.["].+["].*)|(blocks free\.) *.$' | \
                        cut -d" " -f1 | \
                        tr "\r\n" " +" | \
                        sed "s/+$/\n/g" | \
                        bc`

            case $BLOCKSFREE in
                749)    # 40 tracks format (SpeedDOS and alike)
                    XFILEPAT='[0-9][0-9][0-9][0-9]-set-...4.--\.prg'
                    ;;
                664)    # 35 tracks standard format
                    XFILEPAT='[0-9][0-9][0-9][0-9]-set-....3--\.prg'
                    ;;
                *)      # unknown 1541 disk format
                    echo 1>&2
                       echo 1541 drivetype disk format unkown with $BLOCKSFREE blocks free 1>&2
                    rm -f shelltst.pid
                    exit 1
                    ;;
            esac
            ;;
        *)
            echo 1>&2
            echo drivetype unknown 1>&2
            echo 1>&2
            error_info
            ;;
    esac

    shift
    shift
    shift

    echo beginning transfer
    TMPCBMCOPYDIR=`mktemp -d -p ./ cbmcopy_rcmp.XXXXXXXXXXXX`
    cd "$TMPCBMCOPYDIR"
    cbmctrl -@"$ADAPTER" dir $DRIVENO | egrep 'prg *.?$' | cut -d'"' -f2 | xargs cbmcopy -@"$ADAPTER" -r $* $DRIVENO
    cd ..
    cbmctrl -@"$ADAPTER" command $DRIVENO "UJ"
    echo transfer ended
    echo beginning comparison
    ls "$TESTDIR" | grep -v "$XFILEPAT" | diff -nqrs --binary -X - "$TESTDIR" "$TMPCBMCOPYDIR"
    echo comparison ended
    echo cleaning up temporary directory
    rm -f "$TMPCBMCOPYDIR"/*.prg
    rmdir "$TMPCBMCOPYDIR"
    echo cleaning finished
    rm -f shelltst.pid
else
    echo drivetype not found or not found beeing active on the bus.
fi
