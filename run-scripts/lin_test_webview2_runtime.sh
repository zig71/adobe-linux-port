#!/bin/bash
# Stage the Windows Edge WebView2 runtime for testing under Wine.
#
# Question being tested: can the genuine Microsoft WebView2 runtime, copied
# from the Windows oracle, run under Wine on the Linux target? If it can, the
# Creative Cloud installer's WebView2 path becomes available without any new
# implementation. If it cannot, the divergence is confirmed as architectural.
#
# Notes on the runtime layout:
#   - msedgewebview2.exe is x64-only (machine 0x8664). A 32-bit host app still
#     uses it, because WebView2 is always out-of-process.
#   - EBWebView/x86/EmbeddedBrowserWebView.dll is i386 (0x014C) and is the
#     in-process shim the 32-bit host loads.
#   - WebView2Loader.dll is not shipped in the runtime directory; applications
#     link the loader statically or ship their own copy.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
RT=/home/kubuntu/adobe-wine-lab/webview2

echo "=== staged runtime ==="
du -sh "$RT" 2>/dev/null
ls "$RT" 2>/dev/null | head -20

EXE="$RT/msedgewebview2.exe"
if [ ! -f "$EXE" ]; then echo "msedgewebview2.exe NOT STAGED"; exit 1; fi

echo
echo "=== 1. architecture of the pieces ==="
file "$EXE" | sed 's/^/  /'
file "$RT/EBWebView/x86/EmbeddedBrowserWebView.dll" 2>/dev/null | sed 's/^/  /'
file "$RT/EBWebView/x64/EmbeddedBrowserWebView.dll" 2>/dev/null | sed 's/^/  /'

echo
echo "=== 2. can the real browser process start under Wine? ==="
cd "$RT"
timeout 60 "$W" ./msedgewebview2.exe --version > /tmp/wv2-version.out 2>&1
echo "exit=$?"
echo "--- output ---"
head -30 /tmp/wv2-version.out

echo
echo "=== 3. does a fresh prefix see it? ==="
timeout 240 "$W" wineboot -i >/dev/null 2>&1
echo "wineboot=$?"

echo WV2_STAGE_DONE
