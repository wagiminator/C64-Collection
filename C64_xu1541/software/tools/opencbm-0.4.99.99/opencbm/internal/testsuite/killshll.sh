#!/bin/bash
#
# set -x

echo "Doing reset of the xum1541 firmware including IEC bus"
avrdude -c xu1541asp -p m8 -P usb -nuqq &

if [ -f shelltst.pid ]
then
  UXPID=`cat shelltst.pid`

# The following lines need a special utility for Windows

  WINPID=`ps ux | fgrep $UXPID | tail -n 1 | cut -c26-36`
  if [ -n $WINPID ]
  then
    echo "Sending SIGINT (CTRL-C) to Windows console process $WINPID (as background process)"
    cmd.exe /c "sendCTRL-C-1.0rb16.exe $WINPID"
    sleep 5
  fi

  ps ux | fgrep $UXPID > /dev/null
  if [ $? -eq 0 ]
  then
    echo "Doing SIGKILL on process group ID $UXPID"
    kill -9 -$UXPID
    sleep 3
  fi
  rm -f shelltst.pid
fi

# let the xum1541 resurrect, it may not answer for the next command otherweise
sleep 3
