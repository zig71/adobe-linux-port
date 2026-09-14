#!/bin/bash
# Click the hamburger menu in the CC window and capture the result.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
CCW=$(xwininfo -root -children 2>/dev/null | grep -a 'Creative Cloud Desktop' | grep -aoE '0x[0-9a-f]+' | head -1)
echo "ccwin=$CCW"
[ -z "$CCW" ] && { echo NO_CC_WINDOW; exit 1; }
eval $(xdotool getwindowgeometry --shell "$CCW" 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
echo "geom=${X},${Y} ${WIDTH}x${HEIGHT}"
HX=$((X + 28)); HY=$((Y + 55))
echo "click=$HX,$HY"
xdotool mousemove $HX $HY click 1 2>/dev/null
sleep 6
spectacle -b -n -o /tmp/cc-menu.png 2>/dev/null; ls -la /tmp/cc-menu.png 2>/dev/null
echo MENU_DONE
