#!/bin/bash
# List ALL X11 top-level windows with geometry (no name filter).
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
lab_x_sanity || exit 1
xwininfo -root -children 2>/dev/null | grep -a -E '^[[:space:]]+0x' | head -30
echo LISTDONE
