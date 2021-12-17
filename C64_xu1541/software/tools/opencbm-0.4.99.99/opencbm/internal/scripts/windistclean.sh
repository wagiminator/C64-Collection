#! /bin/bash
[ -e sys/libcommon/cbmlog.h ] || exit 1

# delete single files
for A in sys/libcommon/cbmlog.rc sys/libcommon/cbmlog.h sys/libcommon/msg00001.bin sys/libcommon/msg00002.bin mnib36/mnib1541.a65 mnib36/mnib1571.a65 nibtools/nibtools_1541.a65 nibtools/nibtools_1571.a65; do
	[ -f $A ] && rm -v $A
done

# delete files matching a pattern

for A in 0 `find . -name \*.inc|grep -iv makefile` `find . -name \*.plg` `find . -name \*.bsc` WINDOWS/cbm4win-vice.ncb WINDOWS/cbm4win-vice.opt WINDOWS/cbm4win.ncb WINDOWS/cbm4win.opt build*.log; do
	[ -f $A ] && rm -v $A
done


# delete complete directories

for A in `find  . -name objchk\*` `find . -name objfre\*` bin/ Debug/ Release/; do
	[ -d $A ] && rm -rv $A
done
