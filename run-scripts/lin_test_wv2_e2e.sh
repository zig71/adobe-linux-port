#!/bin/bash
# End-to-end test: does the genuine Microsoft Edge WebView2 runtime, brought
# over from the Windows oracle, make the Creative Cloud installer's WebView2
# path work under Wine?
#
# Two things are needed, and only one of them is new work:
#
#   1. The runtime files. Staged at /home/kubuntu/adobe-wine-lab/webview2.
#   2. The registration a normal Windows machine has, which is what the
#      installer's probe looks for:
#        HKLM\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-...}
#          pv       = <version>
#          name     = Microsoft Edge WebView2 Runtime
#          location = <Application dir>
#      On Windows this is written by the WebView2 runtime's own installer, so
#      reproducing it is reproducing a normal machine's state -- not an
#      application-specific workaround.
#
# The runtime is symlinked into the prefix rather than copied: it is 680 MB.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
SRC=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32
VER=153.0.4234.32
GUID='{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
APPDIR='C:\Program Files (x86)\Microsoft\EdgeWebView\Application'

echo "=== fresh prefix so nothing is left over ==="
rm -rf "$WINEPREFIX"
timeout 400 "$W" wineboot -i >/dev/null 2>&1
echo "wineboot=$?"

DEST="$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView/Application"
mkdir -p "$DEST"
ln -sfn "$SRC" "$DEST/$VER"
echo "=== staged in prefix ==="
ls -la "$DEST/" | head -5
ls "$DEST/$VER/msedgewebview2.exe" >/dev/null 2>&1 && echo "runtime reachable: yes" || echo "runtime reachable: NO"

echo
echo "=== register the runtime in both registry views ==="
for view in "" "WOW6432Node\\"; do
  KEY="HKLM\\SOFTWARE\\${view}Microsoft\\EdgeUpdate\\Clients\\$GUID"
  "$W" reg add "$KEY" /v pv       /t REG_SZ /d "$VER" /f >/dev/null 2>&1
  "$W" reg add "$KEY" /v name     /t REG_SZ /d "Microsoft Edge WebView2 Runtime" /f >/dev/null 2>&1
  "$W" reg add "$KEY" /v location /t REG_SZ /d "$APPDIR" /f >/dev/null 2>&1
  echo "--- $KEY"
  "$W" reg query "$KEY" 2>&1 | head -8
done

echo
echo "=== confirm the installer's own probe now succeeds ==="
cp /home/kubuntu/adobe-wine-lab/tests/probes/../../tests/build/probes/probe_webview2.exe /tmp/ 2>/dev/null
if [ -f /tmp/probe_webview2.exe ]; then
  timeout 120 "$W" /tmp/probe_webview2.exe 2>&1 | grep -aE 'HKLM_default_open|HKLM_default_pv|HKLM_32view_open|RUNTIME_DIR0|msedgewebview2_exe'
else
  echo "(probe_webview2.exe not built yet)"
fi

echo
echo "=== run the installer and watch which UI backend it picks ==="
cd /home/kubuntu/Downloads
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-wv2.out 2>&1 < /dev/null &
sleep 120
echo "--- WAM.log lines: $(wc -l < "$LOG" 2>/dev/null || echo 0)"
echo "--- did it reach the UI/window stage? ---"
grep -aE 'onWindowResize|Performing operations for state|WebView2|webview2' "$LOG" 2>/dev/null | head -15
echo "--- window classes created (from a msg trace would come later) ---"
echo "--- installer still alive: $(pgrep -c -f 'Set[-]Up.exe' || echo 0) ---"
echo "--- msedgewebview2 running under Wine? ---"
pgrep -af 'msedgewebview2' | head -3 || echo "(none)"
echo WV2_E2E_DONE
