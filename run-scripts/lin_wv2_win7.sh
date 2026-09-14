#!/bin/bash
# Try AdobeOnLinux's win7 workaround for the WebView2 browser process,
# then test the exact installer path (probe_wv2_internal).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== win7 mode for msedgewebview2.exe ==="
"$W" reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' /v Version /d win7 /f 2>&1 | tail -1
"$W" reg query 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' 2>/dev/null

echo
echo "=== probe_wv2_internal (installer's exact path) ==="
cd /home/kubuntu/adobe-wine-lab/webview2
timeout 90 "$W" ./probe_wv2_internal.exe 2>&1 | head -20
echo "PROBE_RC=$?"
echo WV2WIN7_DONE
