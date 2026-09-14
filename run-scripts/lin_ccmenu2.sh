#!/bin/bash
# Click the hamburger in the largest mapped Wine window; capture result.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
CCW=$(xwininfo -root -children 2>/dev/null | grep -a 'exe"' | grep -aoE '0x[0-9a-f]+ +\(has no name\).* [0-9]{3,}x[0-9]{3,}' | grep -aoE '0x[0-9a-f]+' | head -1)
echo "ccwin=$CCW"
[ -z "$CCW" ] && { echo NO_CANDIDATE; exit 1; }
eval $(xdotool getwindowgeometry --shell "$CCW" 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
echo "geom=${X},${Y} ${WIDTH}x${HEIGHT}"
HX=$((X + 28)); HY=$((Y + 55))
xdotool mousemove $HX $HY click 1 2>/dev/null
sleep 6
spectacle -b -n -o /tmp/cc-menu.png 2>/dev/null; ls -la /tmp/cc-menu.png 2>/dev/null
echo MENU_DONE
