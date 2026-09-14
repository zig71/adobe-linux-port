#!/bin/bash
# Pair each registry Call with its matching Ret (by return address) and report
# only the pairs that failed, for calls made by the installer itself.
set -u
O=${1:-/tmp/regrelay.out}
awk '
/^[0-9a-f]+:Call advapi32\.Reg(OpenKeyExW|QueryValueExW|GetValueW)/ {
    line=$0
    # return address is the trailing ret=<hex>
    ret=""
    if (match(line, /ret=[0-9a-f]+$/)) ret=substr(line, RSTART+4)
    if (ret ~ /^00[4-7][0-9a-f]{5}$/) {
        pending[ret]=line
    }
    next
}
/^[0-9a-f]+:Ret  advapi32\.Reg(OpenKeyExW|QueryValueExW|GetValueW)/ {
    ret=""
    if (match($0, /ret=[0-9a-f]+$/)) ret=substr($0, RSTART+4)
    if (!(ret in pending)) next
    rv=""
    if (match($0, /retval=[0-9a-f]+/)) rv=substr($0, RSTART+7, 8)
    if (rv != "00000000") {
        key="(no key)"
        if (match(pending[ret], /L"[^"]*"/)) key=substr(pending[ret], RSTART, RLENGTH)
        printf "FAIL ret=%s rv=%s key=%s\n", ret, rv, key
    }
    delete pending[ret]
    next
}
' "$O" | sort | uniq -c | sort -rn | head -40
