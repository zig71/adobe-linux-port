#!/bin/bash
# Inspect the faulting indirect call precisely.
#
# The backtrace puts the fault at embeddedbrowserwebview+0x31772, whose call
# site is:
#
#     1015a52c  mov  eax,[esi]                 ; esi = object, eax = vtable
#     1015a52e  mov  ecx,[eax]                 ; ecx = vtable[0]
#     1015a530  call DWORD PTR ds:0x1053c3d8   ; __guard_dispatch_icall_fptr (CFG)
#     1015a540  call ecx
#
# eip=0 results either from the guard dispatch pointer at 0x1053c3d8 being NULL,
# or from ecx (the vtable entry) being NULL. The module is CFG-enabled
# (DllCharacteristics GUARD_CF) and Wine implements no CFG dispatch handling, so
# the pointer is the prime suspect -- but this measures it rather than assuming.
#
# The DLL loads at 0x6cfc0000 in the observed run, so its RVAs are read there.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
export WINEDEBUG=-all
DBG=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/winedbg
OUT=/tmp/wv2-mem.out

cd /home/kubuntu/adobe-wine-lab/webview2

cat > /tmp/wv2-mem.cmd <<'EOF'
cont
info registers
x/4x 0x6d4fc3d8
x/4x 0x6cfc0000+0x53c3d8
bt
quit
EOF

timeout 240 "$DBG" ./probe_wv2_internal.exe < /tmp/wv2-mem.cmd > "$OUT" 2>&1
echo "exit=$? lines=$(wc -l < "$OUT")"
echo
grep -avE 'fixme|WARNING: dzn|libEGL' "$OUT" | tail -50
echo MEM_DONE
