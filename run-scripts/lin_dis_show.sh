#!/bin/bash
# Show the full disassembly window saved earlier.
set -u
sed -n '100,170p' /tmp/check_dis.txt
echo SHOW_DONE
