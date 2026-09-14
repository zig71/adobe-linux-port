#!/bin/bash
# Dump DXVK-related env of CEF GPU processes.
set -u
for p in $(pgrep -f 'type=gpu-process' 2>/dev/null | head -3); do
  echo "== pid $p =="
  tr '\0' '\n' < "/proc/$p/environ" 2>/dev/null | grep -aE 'DXVK|WINEPREFIX' | head -5
done
echo ENV_DONE
