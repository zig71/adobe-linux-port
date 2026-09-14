#!/bin/bash
# Report map state + geometry of candidate windows.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
for w in 0x1a00003 0x1a00005 0x3000001; do
  echo "== $w =="
  xwininfo -id "$w" 2>/dev/null | grep -E 'Map State|Width|Height' | head -4
done
echo MAPS_DONE
