#!/bin/bash
# Identify which Adobe processes burn CPU (20s window) with exe names.
set -u
snap() {
  for p in $(pgrep -f '[A]dobe|Creative Clou[d]|CoreSyn[c]|NGL' 2>/dev/null | head -12); do
    t=$(awk '{print $14+$15}' /proc/$p/stat 2>/dev/null)
    n=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null | grep -aoE '[A-Za-z][A-Za-z .]*\.exe' | head -1)
    echo "$p|$t|$n"
  done
}
echo "--- t0 ---"; snap > /tmp/cpu0.txt; cat /tmp/cpu0.txt
sleep 20
echo "--- t1 ---"; snap > /tmp/cpu1.txt; cat /tmp/cpu1.txt
echo CPU_DONE
