#!/bin/bash
# Kill everything installer-related + reset wineserver. Safe: patterns live
# in this file, not our cmdline (bracketed anyway).
set -u
pkill -9 -f '747[d]' 2>/dev/null
pkill -9 -f 'Set-U[p]' 2>/dev/null
pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 3
WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2 \
  /home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wineserver -k 2>/dev/null
sleep 4
echo "installers_left=$(pgrep -c -f '747[d]' 2>/dev/null || echo 0)"
echo "browsers_left=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)"
echo KILLALL_DONE
