#!/bin/bash
# Click Download on the error dialog, capture what it tries to fetch.
set -u
export DISPLAY=:99
unset XAUTHORITY
echo "=== conns before ==="
ss -tnp 2>/dev/null | grep -a -E 'cc-api|adobe|oobesaas' | head -5 || echo "(none)"
echo "=== clicking Download at 820,543 ==="
xdotool mousemove 820 543 click 1 2>&1
sleep 15
echo "=== conns after ==="
ss -tnp 2>/dev/null | grep -a -E 'cc-api|adobe|oobesaas' | head -8 || echo "(none)"
echo "=== new processes (browser opened?) ==="
pgrep -af 'msedgewebview[2]|iexplore|firefox|chrome' | head -6
echo "=== new files in Downloads ==="
ls -lat ~/Downloads/ | head -5
echo "=== screenshot ==="
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/afterdl.png 2>/dev/null && echo SHOT_OK
echo CLICKDONE
