#!/bin/bash
# Test Wine bug 58921's documented workaround for WebView2 under Wine:
# forcing the browser process to report Windows 8, which makes Chromium take a
# path that does not require DirectComposition.
#
# Applied to a staged WebView2 runtime prefix, then the Creative Cloud installer
# is run to see whether the UI path changes (a webview2 user-data directory, or
# an msedgewebview2 process, would indicate it does).
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== baseline: is HKCU\\Software\\Wine present? ==="
"$W" reg query 'HKCU\Software\Wine' 2>&1 | head -8

echo
echo "=== apply workaround ==="
"$W" reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' /v Version /t REG_SZ /d win8 /f 2>&1 | tail -2
"$W" reg add 'HKCU\Software\Wine\AppDefaults\Creative_Cloud_Set-Up.exe' /v Version /t REG_SZ /d win8 /f 2>&1 | tail -2

echo
echo "=== verify ==="
"$W" reg query 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' 2>&1 | head -6
"$W" reg query 'HKCU\Software\Wine\AppDefaults\Creative_Cloud_Set-Up.exe' 2>&1 | head -6

echo
echo "=== run installer with the workaround in place ==="
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-win8.out 2>&1 < /dev/null &
sleep 150
echo "--- log lines: $(wc -l < "$LOG" 2>/dev/null || echo 0)"
echo "--- states reached ---"
grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | sort -u; echo "(end)"
echo "--- onWindowResize seen? ---"
grep -c 'onWindowResize' "$LOG" 2>/dev/null || echo 0
echo "--- webview2 user-data dir created? ---"
find "$WINEPREFIX/drive_c/users" -ipath '*webview2*' -maxdepth 8 2>/dev/null | head -5; echo "(end)"
echo "--- msedgewebview2 processes ---"
pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0
echo "--- dcomp errors still present? ---"
grep -aiE 'dcomp|DirectComposition|E_NOTIMPL|not implemented' /tmp/cc-win8.out | head -8
echo WIN8_WORKAROUND_DONE
