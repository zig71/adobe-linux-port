#!/bin/bash
# Map-check + hamburger click on the CC content window + capture.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
W=0x1800004
st=$(xwininfo -id "$W" 2>/dev/null | grep 'Map State' | grep -aoE 'Is[A-Za-z]+')
echo "state=$st"
eval $(xdotool getwindowgeometry --shell "$W" 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
echo "geom=${X},${Y} ${WIDTH}x${HEIGHT}"
HX=$((X + 24)); HY=$((Y + 27))
xdotool mousemove $HX $HY click 1 2>/dev/null
echo "clicked=$HX,$HY"
sleep 8
spectacle -b -n -o /tmp/cc-burger.png 2>/dev/null; ls -la /tmp/cc-burger.png 2>/dev/null
echo BURGER_DONE
