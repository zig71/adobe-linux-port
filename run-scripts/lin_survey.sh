#!/bin/bash
# Full survey: every X child window with its size, class and map state.
set -u
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
xwininfo -root -children 2>/dev/null | grep -a '0x' | head -25 | while read -r line; do
  w=$(echo "$line" | grep -aoE '0x[0-9a-f]+' | head -1)
  g=$(echo "$line" | grep -aoE '[0-9]+x[0-9]+\+[0-9]+\+[0-9]+' | head -1)
  [ -z "$w" ] && continue
  st=$(xwininfo -id "$w" 2>/dev/null | grep 'Map State' | grep -aoE 'Is[A-Za-z]+')
  cls=$(xwininfo -id "$w" 2>/dev/null | grep -a 'Window id' | grep -aoE '"[^"]*"' | head -1)
  echo "$w $g map=$st cls=$cls"
done
echo SURVEY_DONE
