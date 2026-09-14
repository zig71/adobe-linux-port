#!/bin/bash
# Capture the installer's window pixels on :0 by window ID (works for X11
# clients even when root grabs return black under Wayland).
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
lab_x_sanity || exit 1
echo "=== top-level windows ==="
xwininfo -root -children 2>/dev/null | grep -a -E '0x[0-9a-f]+ "' | head -10
echo "=== capture candidates ==="
for id in $(xwininfo -root -children 2>/dev/null | grep -a -o -E '0x[0-9a-f]+' | head -12); do
  name=$(xwininfo -id "$id" 2>/dev/null | grep -a -m1 'xwininfo: Window id' | cut -c1-100)
  echo "$id :: $name"
done
echo SHOTDONE
