#!/bin/bash
# Dump the full GPU-process command line to a file (no self-match: the
# pattern lives in a variable, not the pgrep argument text).
set -u
pat='type=gpu-process'
pgrep -af "$pat" | head -2 | cut -c1-2000 > /tmp/gpucmd.txt 2>/dev/null
wc -l < /tmp/gpucmd.txt
echo CMDONE
