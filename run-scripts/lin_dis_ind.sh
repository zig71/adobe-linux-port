#!/bin/bash
# Indirect calls + data references in the CHECK window.
set -u
grep -a -E 'call +\*' /tmp/check_wide.txt | head -20
echo "=== rdata refs ==="
grep -a -o -E '# 0x19[0-9a-f]+' /tmp/check_wide.txt | sort -u | head -20
echo INDDONE
