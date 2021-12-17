#!/bin/bash
#
# set -x

DRIVE=8
COMMAND=./d64copy

cbmctrl reset

# for trfer in o p "p -w" s1 "s1 -w" s2 "s2 -w"
# for trfer in p "p -w" s1 "s1 -w" s2 "s2 -w"
for trfer in o
do
  for drctn in "writetst.d64 $DRIVE" "$DRIVE readtest.d64"
  do
    for (( i=1 ; i<18; i=i+1 ))
    do
      SUM=0
      for sz in 17 18 25 31
      do
        echo -e -n " Executing: \"$COMMAND $drctn -t$trfer -i$i -s$sz -e$sz\""
        STARTTIME=`date +%s.%N`
        $COMMAND $drctn -t$trfer -i$i -s$sz -e$sz > /dev/null 2> /dev/null
        END_TIME=`date +%s.%N`
        TIMERES=`echo -e " scale=2 \n ( $END_TIME - $STARTTIME )/1 " | bc`
        echo -e ", transfer time: $TIMERES""s"
        case $sz in
          17)
            SUM=`echo "$SUM + $TIMERES * 17" | bc`
            ;;
          18)
            SUM=`echo "$SUM + $TIMERES * 7" | bc`
            ;;
          25)
            SUM=`echo "$SUM + $TIMERES * 6" | bc`
            ;;
          31)
            SUM=`echo "$SUM + $TIMERES * 5" | bc`
            ;;
        esac    
      done
      echo -e "35 tracks transfer time (approx.) for \"$COMMAND $drctn -t$trfer -i$i\" : $SUM""s"
    done
  done
done
