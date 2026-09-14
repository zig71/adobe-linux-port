#!/bin/bash
# Find which import owns IAT slot 0x1937110a0 by scanning the import dump
# for per-DLL FirstThunk ranges. Light: pure text processing.
set -u
F=/tmp/msedge_imports.txt
python3 - "$F" <<'EOF'
import re, sys
cur_dll = None
cur_first = None
hits = []
with open(sys.argv[1], errors="replace") as f:
    for line in f:
        m = re.match(r'\s*DLL Name: (\S+)', line)
        if m:
            cur_dll = m.group(1)
            continue
        m = re.match(r'\s*vma:\s+([0-9a-fA-F]+)\s+Hint\s+.*First\s*$', line)
        if m:
            cur_first = m.group(1)
            continue
        if '1937110a0' in line.lower():
            hits.append((cur_dll, cur_first, line.strip()[:100]))
for h in hits[:8]:
    print(h)
print("SCAN_DONE", len(hits))
EOF
