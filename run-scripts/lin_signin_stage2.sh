#!/bin/bash
# Stage 2: enter Adobe password on the password step, submit, capture result.
# Reads /tmp/cc-creds (PASS=...). Only run when the password field is showing.
set -u
export DISPLAY=:99
unset XAUTHORITY
[ -f /tmp/cc-creds ] || { echo NO_CREDS; exit 1; }
. /tmp/cc-creds
[ -n "${PASS:-}" ] || { echo NO_PASS; exit 1; }
WIN=0xa00001
eval $(xdotool getwindowgeometry --shell $WIN 2>/dev/null | grep -E 'WIDTH|HEIGHT|X|Y')
# password field approx same position as email field was
FX=$((X + WIDTH / 2)); FY=$((Y + HEIGHT * 32 / 100))
xdotool mousemove $FX $FY click 1 2>/dev/null
sleep 2
xdotool type --delay 80 "$PASS" 2>/dev/null
sleep 2
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-pass-entered.png && echo PASS_SHOT
xdotool key Return 2>/dev/null
sleep 15
xwd -id $WIN -silent 2>/dev/null | convert xwd:- /tmp/cc-after-signin.png && echo SIGNIN_SHOT
shred -u /tmp/cc-creds 2>/dev/null || rm -f /tmp/cc-creds
echo STAGE2_DONE
