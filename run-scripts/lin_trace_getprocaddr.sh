#!/bin/bash
# Name the function pointer Chromium calls through when creating the WebView2
# environment.
#
# EmbeddedBrowserWebView.dll faults with eip=0, i.e. a call through a function
# pointer that was never populated. In Chromium that pattern almost always comes
# from GetProcAddress() returning NULL for an export the loader then invokes
# without checking. Recording every GetProcAddress and LoadLibrary result that
# comes back NULL should name the missing capability directly, rather than
# leaving it to inference.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
OUT=/tmp/wv2-relay.out

cd /home/kubuntu/adobe-wine-lab/webview2
rm -f "$OUT"

# Filter as it streams: relay output is enormous and the fault is early.
WINEDEBUG=+relay timeout 200 "$W" ./probe_wv2_internal.exe 2>&1 \
  | grep -aE 'GetProcAddress|LoadLibrary' \
  > "$OUT"

echo "captured=$(wc -l < "$OUT")"

echo
echo "=== GetProcAddress calls whose result was NULL ==="
awk '
/^[0-9a-f]+:Call .*GetProcAddress/ {
    line = $0
    tid = $0; sub(/:.*/, "", tid)
    # module handle is arg1, name is the last quoted string
    name = ""
    if (match(line, /"[^"]+"/)) name = substr(line, RSTART, RLENGTH)
    pending[tid] = name
    next
}
/^[0-9a-f]+:Ret  .*GetProcAddress/ {
    tid = $0; sub(/:.*/, "", tid)
    rv = ""
    if (match($0, /retval=[0-9a-f]+/)) rv = substr($0, RSTART+7, 8)
    if (tid in pending) {
        if (rv == "00000000") print "NULL  " pending[tid]
        delete pending[tid]
    }
    next
}
' "$OUT" | sort | uniq -c | sort -rn | head -40

echo
echo "=== LoadLibrary calls whose result was NULL ==="
awk '
/^[0-9a-f]+:Call .*LoadLibrary/ {
    line = $0
    tid = $0; sub(/:.*/, "", tid)
    name = ""
    if (match(line, /"[^"]+"/)) name = substr(line, RSTART, RLENGTH)
    else if (match(line, /,[0-9a-f]+\)/)) name = substr(line, RSTART, RLENGTH)
    pending2[tid] = name " || " line
    next
}
/^[0-9a-f]+:Ret  .*LoadLibrary/ {
    tid = $0; sub(/:.*/, "", tid)
    rv = ""
    if (match($0, /retval=[0-9a-f]+/)) rv = substr($0, RSTART+7, 8)
    if (tid in pending2) {
        if (rv == "00000000") print "FAILED " pending2[tid]
        delete pending2[tid]
    }
    next
}
' "$OUT" | sed 's/^[0-9a-f]*:Call //' | sort | uniq -c | sort -rn | head -30

echo
echo "=== GetProcAddress calls that succeeded, most frequent ==="
awk '
/^[0-9a-f]+:Call .*GetProcAddress/ {
    tid = $0; sub(/:.*/, "", tid)
    name = ""
    if (match($0, /"[^"]+"/)) name = substr($0, RSTART, RLENGTH)
    pending3[tid] = name
    next
}
/^[0-9a-f]+:Ret  .*GetProcAddress/ {
    tid = $0; sub(/:.*/, "", tid)
    rv = ""
    if (match($0, /retval=[0-9a-f]+/)) rv = substr($0, RSTART+7, 8)
    if (tid in pending3) {
        if (rv != "00000000") print pending3[tid]
        delete pending3[tid]
    }
}
' "$OUT" | sort | uniq -c | sort -rn | head -30

echo RELAY_DONE
