#!/bin/bash
# Register a staged Edge WebView2 runtime in a Wine prefix, the way the
# runtime's own installer does on Windows, so applications can find it.
set -eu
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
VER=${VER:-153.0.4234.32}
GUID='{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
APPDIR='C:\Program Files (x86)\Microsoft\EdgeWebView\Application'

DEST="$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView/Application"
mkdir -p "$DEST"
ln -sfn "/home/kubuntu/adobe-wine-lab/webview2/$VER" "$DEST/$VER"

for view in "" "WOW6432Node\\"; do
  KEY="HKLM\\SOFTWARE\\${view}Microsoft\\EdgeUpdate\\Clients\\$GUID"
  "$W" reg add "$KEY" /v pv       /t REG_SZ /d "$VER" /f >/dev/null
  "$W" reg add "$KEY" /v name     /t REG_SZ /d "Microsoft Edge WebView2 Runtime" /f >/dev/null
  "$W" reg add "$KEY" /v location /t REG_SZ /d "$APPDIR" /f >/dev/null
  "$W" reg query "$KEY" | head -5
done
echo WEBVIEW2_REGISTERED
