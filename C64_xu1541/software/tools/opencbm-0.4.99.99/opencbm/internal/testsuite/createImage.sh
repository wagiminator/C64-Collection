#!/bin/bash
#
# set -x

function error_info {
	echo "createImage.sh <testfileset> <drivetype> [<dotransfer>]" 1>&2
	echo  1>&2
	echo "testfileset: mass | fill"  1>&2
	echo  1>&2
	echo "drivetype:   1581, 1571, 1541"  1>&2
	echo  1>&2
	echo "dotransfer:  any non-empty string enables disk imge transfer to drive"  1>&2
	rm -f shelltst.pid
	exit 1
	}

function createFileset {
	case $1 in
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
	}

if [ $# -lt 2 ]
then
	error_info
fi


which c1541 2> /dev/null | fgrep c1541 > /dev/null
if [ 0 -ne $? ]
then
	which cbmconvert 2> /dev/null | fgrep cbmconvert > /dev/null
	if [ 0 -ne $? ]
	then
		echo "VICE's ``c1541´´ or Marko Mäkelä's ``cbmconvert´´ is a prerequisite"  1>&2
		echo "needed (in the search path) for generating the test set as an image file"  1>&2
		exit 1
	else
		createFileset $1
		# create image with cbmconvert
	    case $2 in
	        1581)
				cbmconvert -n -D8o tstimg_rcmp_1581.d81 cbmcopy_files/1[0-9][0-9][0-9]-1581???????.prg
	            ;;
	        1571)
				cbmconvert -n -D7o tstimg_rcmp_1571.d71 cbmcopy_files/1[0-9][0-9][0-9]-158171?????.prg
	            ;;
	        1541)
				cbmconvert -n -D4o tstimg_rcmp_1541.d64 cbmcopy_files/1[0-9][0-9][0-9]-15817141435.prg
	            ;;
	           *)
				echo 1>&2
	           	echo drivetype unknown 1>&2
				echo 1>&2
				error_info
	            ;;
	    esac
	fi
else
	createFileset $1
	# create image with c1541
	cd cbmcopy_files/
    case $2 in
        1581)
			c1541 -format testimage,81 d81 ../tstimg_rcmp_1581.d81 | fgrep -v "GetProcAddress cbm_"
			ls 1[0-9][0-9][0-9]-1581???????.prg | sed "s/^/write /g" | c1541 ../tstimg_rcmp_1581.d81 | fgrep -v "GetProcAddress cbm_"
            ;;
        1571)
			c1541 -format testimage,71 d71 ../tstimg_rcmp_1571.d71 | fgrep -v "GetProcAddress cbm_"
			ls 1[0-9][0-9][0-9]-158171?????.prg | sed "s/^/write /g" | c1541 ../tstimg_rcmp_1571.d71 | fgrep -v "GetProcAddress cbm_"
            ;;
        1541)
			c1541 -format testimage,41 d64 ../tstimg_rcmp_1541.d64 | fgrep -v "GetProcAddress cbm_"
        	ls 1[0-9][0-9][0-9]-15817141435.prg | sed "s/^/write /g" | c1541 ../tstimg_rcmp_1541.d64 | fgrep -v "GetProcAddress cbm_"
            ;;
           *)
			echo 1>&2
           	echo drivetype unknown 1>&2
			echo 1>&2
			error_info
            ;;
    esac
    cd ../
fi


if [ -n "$3" ]
then
	echo $$ > shelltst.pid
	DRIVENO=`cbmctrl detect | fgrep " $2 " | cut -d: -f1 | tail -n 1 | tr -d "[:space:]"`
	case $2 in
	    1581)
			echo "Uuups, 1581 disk images (d81) are currently not supported..."  1>&2
			echo 1>&2
			error_info
	        ;;
	    1571)
			d64copy -2 tstimg_rcmp_1571.d71 $DRIVENO
	        ;;
	    1541)
			d64copy tstimg_rcmp_1541.d64 $DRIVENO
	        ;;
	       *)
			echo 1>&2
	      	echo drivetype unknown 1>&2
			echo 1>&2
			error_info
	        ;;
	esac
	rm -f shelltst.pid
fi
