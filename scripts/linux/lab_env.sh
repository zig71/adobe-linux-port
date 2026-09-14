#!/bin/bash
# Shared environment for driving Wine on the Kubuntu guest.
#
# XAUTHORITY must not be hardcoded: the Xwayland auth cookie is named
# /run/user/1000/xauth_<random> and the random suffix changes on every boot.
# A stale name makes X connections fail, which stops Wine creating windows and
# silently cripples anything that needs one (the WebView2 controller, for
# instance), while leaving early initialisation looking healthy.
#
# Usage:  . /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh

export WINEPREFIX="${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}"
export DISPLAY="${DISPLAY:-:0}"
export WINEDEBUG="${WINEDEBUG:--all}"

if [ -z "${XAUTHORITY:-}" ] || [ ! -r "${XAUTHORITY:-/nonexistent}" ]; then
    candidate=$(ls -1t /run/user/1000/xauth_* 2>/dev/null | head -1)
    if [ -n "$candidate" ]; then
        export XAUTHORITY="$candidate"
    fi
fi

WINE=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
export WINE

# Fail loudly if the display really is unreachable, rather than producing a
# healthy-looking log from a program that never drew anything.
lab_x_sanity() {
    if ! command -v xdpyinfo >/dev/null 2>&1; then
        return 0
    fi
    if ! timeout 10 xdpyinfo >/dev/null 2>&1; then
        echo "lab_env: WARNING - X display $DISPLAY not reachable with XAUTHORITY=${XAUTHORITY:-unset}" >&2
        return 1
    fi
    return 0
}
