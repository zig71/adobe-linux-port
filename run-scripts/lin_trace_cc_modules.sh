#!/bin/bash
# Trace whether the Creative Cloud installer attempts its WebView2 path now that
# the runtime is staged and registered, or still falls back to ieframe.
#
# The environment creation itself was proven to work under Wine by
# probe_wv2_internal (with the correct 5-argument ABI), so if the installer
# still stops the divergence is in how the installer drives it, not in Wine's
# ability to load and initialise the runtime.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
OUT=/tmp/cc-wv2-trace.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT"

cd /home/kubuntu/Downloads
WINEDEBUG=+loaddll timeout 150 "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1
echo "exit=$? lines=$(wc -l < "$OUT")"
echo
echo "=== WebView2 runtime modules loaded? ==="
grep -a 'embeddedbrowserwebview\|msedge\|EBWebView\|webview2' "$OUT" | grep -a 'Loaded' | head -15
echo "(end)"
echo
echo "=== ieframe / mshtml loaded? ==="
grep -a 'ieframe\|mshtml' "$OUT" | grep -a 'Loaded' | head -10
echo "(end)"
echo
echo "=== Adobe modules loaded ==="
grep -a 'Loaded' "$OUT" | grep -aiE 'Adobe|Acc|Ngl|Core' | head -10
echo "(end)"
echo
echo "=== all modules loaded, last 25 ==="
grep -a 'Loaded' "$OUT" | tail -25
echo TRACE_DONE
