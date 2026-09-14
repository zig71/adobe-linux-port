#!/bin/bash
# Register a staged WebView2 runtime the way its own installer does on Windows.
#
# Two keys matter, and the second is the one that was missing:
#
#   EdgeUpdate\Clients\{F3017226-...}       pv / name / location
#   EdgeUpdate\ClientState\{F3017226-...}   EBWebView = <Application\version>
#
# The oracle carries EBWebView under ClientState pointing at the versioned
# Application directory; that is the value an application reads to locate the
# runtime it will load EmbeddedBrowserWebView.dll from. Without it the installer
# falls back to its IE path without ever attempting WebView2, which is what the
# module trace showed.
set -eu
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
VER=${VER:-153.0.4234.32}
GUID='{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
APPDIR='C:\Program Files (x86)\Microsoft\EdgeWebView\Application'

DEST="$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView/Application"
mkdir -p "$DEST"
ln -sfn "/home/kubuntu/adobe-wine-lab/webview2/$VER" "$DEST/$VER"

for view in "" "WOW6432Node\\\\"; do
  CKEY="HKLM\\SOFTWARE\\${view}Microsoft\\EdgeUpdate\\Clients\\$GUID"
  SKEY="HKLM\\SOFTWARE\\${view}Microsoft\\EdgeUpdate\\ClientState\\$GUID"

  "$W" reg add "$CKEY" /v pv       /t REG_SZ /d "$VER" /f >/dev/null
  "$W" reg add "$CKEY" /v name     /t REG_SZ /d "Microsoft Edge WebView2 Runtime" /f >/dev/null
  "$W" reg add "$CKEY" /v location /t REG_SZ /d "$APPDIR" /f >/dev/null

  "$W" reg add "$SKEY" /v EBWebView /t REG_SZ /d "$APPDIR\\$VER" /f >/dev/null
  "$W" reg add "$SKEY" /v pv        /t REG_SZ /d "$VER" /f >/dev/null
  "$W" reg add "$SKEY" /v InstallSource /t REG_SZ /d "windows" /f >/dev/null

  echo "--- $CKEY"
  "$W" reg query "$CKEY" 2>&1 | sed -n '3,6p'
  echo "--- $SKEY"
  "$W" reg query "$SKEY" /v EBWebView 2>&1 | sed -n '3,5p'
done

echo "=== does the runtime path in EBWebView exist inside the prefix? ==="
ls -d "$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView/Application/$VER" >/dev/null 2>&1 \
  && echo "EBWebView path resolves: yes" || echo "EBWebView path resolves: NO"

echo WEBVIEW2_REGISTERED
