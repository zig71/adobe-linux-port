#!/bin/bash
# List Wine window geometries to find the big app window.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
for w in $(xwininfo -root -children 2>/dev/null | grep -a 'exe"' | grep -aoE '0x[0-9a-f]+' | head -14); do
  g=$(xdotool getwindowgeometry --shell "$w" 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y' | tr '\n' ' ')
  echo "$w $g"
done
echo GEOM_DONE
