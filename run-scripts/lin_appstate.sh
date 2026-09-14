#!/bin/bash
# Check whether the React app booted (Local Storage writes) or never ran.
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Default
echo "=== local storage ==="
ls "$P/Local Storage/leveldb/" 2>/dev/null | head -8
echo "=== session storage ==="
ls "$P/Session Storage/" 2>/dev/null | head -5
echo "=== indexeddb ==="
ls "$P/IndexedDB/" 2>/dev/null | head -8
echo "=== preferences (startup urls?) ==="
grep -a -o -E '"startup_urls":\[[^]]{0,200}' "$P/Preferences" 2>/dev/null | head -3
echo "=== GPUCache (compositing happened?) ==="
ls -la "$P/GPUCache/" 2>/dev/null | head -6
du -sh "$P/GPUCache" 2>/dev/null
echo APPSTATEDONE
