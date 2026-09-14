#!/bin/bash
# Capture the biggest msedgewebview2 content window by ID.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
lab_x_sanity || exit 1
id=$(xwininfo -root -children 2>/dev/null | grep -a 'msedgewebview2.exe.*760x501' | head -1 | grep -a -o -E '0x[0-9a-f]+' | head -1)
echo "target=$id"
if [ -n "$id" ]; then
  xwd -id "$id" -silent 2>/dev/null | convert xwd:- /tmp/wvcontent.png 2>/dev/null && echo SHOT_OK
  ls -la /tmp/wvcontent.png
fi
echo CAPDONE
