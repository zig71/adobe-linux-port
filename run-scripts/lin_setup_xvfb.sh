#!/bin/bash
# Install and start a headless X server for driving Wine.
#
# The Plasma session on this guest occupies a large share of its ~5.9 GB of RAM.
# Running the Creative Cloud installer with the real WebView2 runtime needs Wine
# plus several Chromium processes including a 332 MB msedge.dll, which pushes
# the machine into global OOM; the kernel then kills session processes
# (kded6, kdeconnectd, ksecretd), Plasma dies and Xwayland goes with it, taking
# the installer's window connection away mid-run.
#
# A headless server removes the desktop from the memory budget and from the
# blast radius, and makes screenshots possible again (xwd against Xvfb works,
# unlike against Xwayland on this Wayland session).
set -eu
echo ${LAB_PW} | sudo -S -p '' true 2>/dev/null

echo "=== free the machine before measuring ==="
pkill -9 -f 'msedgewebview2' 2>/dev/null || true
sleep 2
free -m | head -2

echo
echo "=== install xvfb and capture tools ==="
echo ${LAB_PW} | sudo -S -p '' env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y xvfb x11-utils x11-apps >/tmp/apt-xvfb.log 2>&1
echo "apt rc=$?"
for t in Xvfb xvfb-run xdpyinfo xwd import xdotool wmctrl; do
    printf '%-10s %s\n' "$t" "$(command -v "$t" || echo MISSING)"
done

echo
echo "=== start Xvfb on :99 ==="
pkill -f 'Xvfb :99' 2>/dev/null || true
sleep 1
Xvfb :99 -screen 0 1280x960x24 -nolisten tcp >/tmp/xvfb.log 2>&1 &
sleep 3

if DISPLAY=:99 timeout 10 xdpyinfo >/dev/null 2>&1; then
    echo "Xvfb :99 is up"
    DISPLAY=:99 xdpyinfo | grep -E 'dimensions|depth of root' | head -3
else
    echo "Xvfb FAILED to start"
    tail -20 /tmp/xvfb.log
    exit 1
fi

echo
echo "=== memory with Xvfb instead of Plasma ==="
free -m | head -2
echo XVFB_READY
