#!/bin/bash
# Find which processes hold GrowthSDK files open.
set -u
for d in /proc/[0-9]*/fd; do
  p=${d#/proc/}; p=${p%/fd}
  for f in "$d"/*; do
    t=$(readlink "$f" 2>/dev/null)
    case "$t" in
      *GrowthSDK*) echo "pid=$p exe=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null | grep -aoE '[A-Za-z][A-Za-z .]*\.exe' | head -1)"; break;;
    esac
  done
done 2>/dev/null | head -8
echo HOLDER_DONE
