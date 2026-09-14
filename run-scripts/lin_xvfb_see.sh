#!/bin/bash
# List Xvfb windows + screenshot root.
set -u
export DISPLAY=:99
unset XAUTHORITY
xwininfo -root -children 2>/dev/null | grep -a -E '^[[:space:]]+0x' | head -20
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/xvfb-see.png 2>/dev/null && echo SHOT_OK
ls -la /tmp/xvfb-see.png 2>/dev/null
echo SEEDONE
