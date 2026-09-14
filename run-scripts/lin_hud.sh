#!/bin/bash
# DXVK HUD test: if presents reach the window, the HUD paints over the white.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export DXVK_HUD=1
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-hud.out 2>&1 < /dev/null &
sleep 60
export DISPLAY=:99; unset XAUTHORITY
WIN=$(xwininfo -root -children 2>/dev/null | grep -a 'Adobe Creative Cloud' | grep -aoE '0x[0-9a-f]+' | head -1)
echo "win=$WIN"
[ -n "$WIN" ] && xwd -id "$WIN" -silent 2>/dev/null | convert xwd:- /tmp/cc-hud.png && ls -la /tmp/cc-hud.png
echo HUD_DONE
