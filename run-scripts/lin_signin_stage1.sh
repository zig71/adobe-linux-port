#!/bin/bash
# Stage 1: enter Adobe email, click Continue, capture password step.
# Reads /tmp/cc-creds (EMAIL=...). Run once; leaves session for stage 2 / 2FA.
set -u
export DISPLAY=:99
unset XAUTHORITY
[ -f /tmp/cc-creds ] || { echo NO_CREDS; exit 1; }
. /tmp/cc-creds
[ -n "${EMAIL:-}" ] || { echo NO_EMAIL; exit 1; }
WIN=0xa00001
eval $(xdotool getwindowgeometry --shell $WIN 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
FX=$((X + WIDTH / 2)); FY=$((Y + HEIGHT * 30 / 100))
xdotool mousemove $FX $FY click 1 2>/dev/null
sleep 2
xdotool key ctrl+a BackSpace 2>/dev/null
sleep 1
xdotool type --delay 80 "$EMAIL" 2>/dev/null
sleep 2
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-email-entered.png && echo EMAIL_SHOT
# click Continue (right side, ~40% height)
CX=$((X + WIDTH * 68 / 100)); CY=$((Y + HEIGHT * 40 / 100))
xdotool mousemove $CX $CY click 1 2>/dev/null
sleep 12
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-after-continue.png && echo CONTINUE_SHOT
echo STAGE1_DONE
