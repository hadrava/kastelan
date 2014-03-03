#!/bin/bash

# Load config
. ../config
prepare_fetch

robot_read() {
  echo lsz $1 | nc "$ROBOT_IP" "$ROBOT_PORT" | while read -N1 i; do killall nc 2> /dev/null; done
}

[ -d logs ] || mkdir logs
cd logs/
old_aktual=`cat aktual`
robot_read "/mnt/logs/aktual"
rz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT"

aktual=`cat aktual`
from=`expr "$old_aktual" "+" "1"`

files_to_fetch="`seq $from $aktual | sed 's/^/\/mnt\/logs\/log_/g'`"
robot_read "$files_to_fetch"
rz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT"
echo
