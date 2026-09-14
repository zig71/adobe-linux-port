#!/bin/bash
# Hunt poisoned OS fingerprints in the persistent WebView2 profile.
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView
echo "=== OS fingerprints in Adobe profile ==="
timeout 90 grep -a -r -l -E 'Windows NT 6\.1|6\.1\.7601|Windows 7|19045|Windows 10 Pro' "$P" 2>/dev/null | head -10
echo "(end)"
echo "=== UA-related strings in Preferences ==="
timeout 30 grep -a -o -E '.{40}Windows NT.{40}' "$P/Default/Preferences" 2>/dev/null | head -5
echo "(end prefs)"
echo PROFHUNT_DONE
