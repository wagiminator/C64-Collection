#! /bin/sh

[ "$CC65PATH" = "" ] && CC65PATH=

[ "$CA65"     = "" ] && CA65=$CC65PATH\ca65
[ "$LD65"     = "" ] && LD65=$CC65PATH\ld65
[ "$OD"       = "" ] && OD=od
[ "$SED"      = "" ] && SED=sed
[ "$RM"       = "" ] && RM=rm

CA65_FLAGS="$CA65_FLAGS --feature labels_without_colons --feature pc_assignment --feature loose_char_term --include-dir .."

funcbuildinc()
{
WHICHFILE=`echo $1|$SED 's/\.\(a\|A\)65//'`

# assemble the file
$CA65 $CA65_FLAGS -o $WHICHFILE.o $WHICHFILE.a65

# link it
if test -s $WHICHFILE.o; then
	$LD65 -o $WHICHFILE.bin --target none $WHICHFILE.o
	$RM $WHICHFILE.o
fi

# convert it into #include-able form for C
if test -s $WHICHFILE.bin; then
	$OD -w8 -txC -v -An $WHICHFILE.bin|$SED 's/\([0-9a-f]\{2\}\) */0x\1,/g; $s/,$//' > $WHICHFILE.inc
	$RM $WHICHFILE.bin
fi
}
