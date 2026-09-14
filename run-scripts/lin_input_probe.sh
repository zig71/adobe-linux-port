#!/bin/bash
# Interactivity probe: click email field, type, screenshot, clear, screenshot.
set -u
export DISPLAY=:99
unset XAUTHORITY
WIN=0xa00001
eval $(xdotool getwindowgeometry --shell $WIN 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
echo "geom=${X},${Y} ${WIDTH}x${HEIGHT}"
# email field approx center-x, 30% down (from cc-paint.png proportions)
FX=$((X + WIDTH / 2)); FY=$((Y + HEIGHT * 30 / 100))
echo "click=$FX,$FY"
xdotool mousemove $FX $FY click 1 2>/dev/null
sleep 2
xdotool type --delay 60 'adobe.test' 2>/dev/null
sleep 2
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-typed.png && echo TYPED_SHOT
sleep 1
xdotool key ctrl+a BackSpace 2>/dev/null
sleep 2
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-cleared.png && echo CLEARED_SHOT
echo PROBE_DONE
