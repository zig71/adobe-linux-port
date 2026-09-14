#!/bin/bash
# Get a real backtrace for the WebView2 fault.
#
# The fault is "execute access to 00000000" with eip=0 -- a call through a NULL
# function pointer. Tracing GetProcAddress showed no NULL results, so the
# pointer does not come from a missing export; it comes from somewhere else and
# the call site has to be inspected directly.
#
# winedbg is driven from a command file: continue past the initial loader
# breakpoint, let the fault stop execution, then dump the backtrace and
# registers of the faulting thread.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
DBG=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/winedbg
OUT=/tmp/wv2-bt.out

cd /home/kubuntu/adobe-wine-lab/webview2

cat > /tmp/wv2-dbg.cmd <<'EOF'
cont
bt
info registers
info threads
quit
EOF

timeout 240 "$DBG" ./probe_wv2_internal.exe < /tmp/wv2-dbg.cmd > "$OUT" 2>&1
echo "exit=$? lines=$(wc -l < "$OUT")"
echo

echo "=== backtrace and registers ==="
grep -avE 'fixme|WARNING: dzn|libEGL' "$OUT" | tail -60
echo
echo "=== any frames naming a module ==="
grep -aE '^\s+[0-9]+ 0x' "$OUT" | head -40
echo BT_DONE
