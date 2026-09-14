#!/bin/bash
# Dump visited URLs from the WebView2 profile History.
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Default
cp -f "$P/History" /tmp/Hist_copy.db 2>/dev/null
python3 - /tmp/Hist_copy.db <<'EOF'
import sqlite3, sys
db = sqlite3.connect(sys.argv[1])
try:
    rows = db.execute("SELECT url, visit_count, last_visit_time FROM urls ORDER BY last_visit_time DESC LIMIT 12").fetchall()
    for u, c, t in rows:
        print(f"{c}x {u[:130]}")
except Exception as e:
    print(f"QUERY_FAIL {e}")
EOF
echo HISTDONE
