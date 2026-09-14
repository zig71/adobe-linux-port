#!/bin/bash
# Map-state + capture of every sizable Adobe window.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
for w in 0x1400001 0x2400005 0x2400003; do
  st=$(xwininfo -id "$w" 2>/dev/null | grep 'Map State' | grep -aoE 'Is[A-Za-z]+')
  echo "$w -> $st"
  if [ "$st" = "IsViewable" ]; then
    xwd -id "$w" -silent 2>/dev/null | convert xwd:- /tmp/cc-see-$w.png 2>/dev/null && ls -la /tmp/cc-see-$w.png
  fi
done
echo SEE_DONE
