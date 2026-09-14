#!/bin/bash
# Sample CPU ticks of Creative Cloud processes twice, 20s apart.
set -u
snap() {
  for p in $(pgrep -f 'Creative Cloud' 2>/dev/null | head -5); do
    t=$(awk '{print $14+$15}' /proc/$p/stat 2>/dev/null)
    echo "$p $t"
  done
}
echo "--- t0 ---"
snap
sleep 20
echo "--- t1 ---"
snap
echo CPU_DONE
