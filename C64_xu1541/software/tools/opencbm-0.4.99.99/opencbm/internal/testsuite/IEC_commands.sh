#!/bin/bash
#
# set -x

echo $$ > shelltst.pid

cbmctrl $1 command $2 I0

cbmctrl $1 lock
cbmctrl $1 open $2 2 "$,S,R"
cbmctrl $1 talk $2 2
cbmctrl $1 read | tr -d "\0"
cbmctrl $1 untalk
cbmctrl $1 close $2 2
cbmctrl $1 unlock

echo -e "\n\nGetting status:"

cbmctrl $1 lock
cbmctrl $1 talk $2 15
cbmctrl $1 read | tr "\r" "\n"
cbmctrl $1 untalk
cbmctrl $1 unlock

rm -f shelltst.pid
