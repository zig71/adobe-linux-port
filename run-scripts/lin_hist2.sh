#!/bin/bash
# Inspect the CURRENT run's Temp browser profile (not the stale Adobe one).
set -u
T=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp
D=$(ls -td "$T"/{*-*-*-*-*}/EBWebView 2>/dev/null | head -1)
echo "profile=$D"
ls "$D" 2>/dev/null | head -12
echo "=== history urls ==="
cp -f "$D/Default/History" /tmp/Hist2.db 2>/dev/null
python3 - /tmp/Hist2.db <<'EOF'
import sqlite3, sys
try:
    db = sqlite3.connect(sys.argv[1])
    rows = db.execute("SELECT url FROM urls ORDER BY last_visit_time DESC LIMIT 6").fetchall()
    for (u,) in rows:
        print(u[:140])
except Exception as e:
    print(f"QUERY_FAIL {e}")
EOF
echo HIST2DONE
