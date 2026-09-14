#!/bin/bash
# Rebuild the fork after a source change and re-run the advapi32 registry test.
set -e
LABEL="${1:-run}"
WINE=/home/kubuntu/adobe-wine-lab/install/wine-upstream/bin/wine
BUILD=/home/kubuntu/adobe-wine-lab/src/wine/build64
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-fork2
export WINEDEBUG=-all
export WINEDLLOVERRIDES='mscoree,mshtml='
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif

cd "$BUILD"
make -j"$(nproc)" > /tmp/make-$LABEL.log 2>&1
make install > /tmp/install-$LABEL.log 2>&1
cd "$BUILD/dlls/advapi32/tests"
timeout 600 "$WINE" ./x86_64-windows/advapi32_test.exe registry 2>&1 \
  | grep -E 'Test failed|succeeded inside todo|tests executed' > "/tmp/regtest-$LABEL.txt"
echo "=== $LABEL"
grep -E 'tests executed' "/tmp/regtest-$LABEL.txt"
echo "failures: $(grep -c 'Test failed' /tmp/regtest-$LABEL.txt)"
echo "todo_succeeded: $(grep -c 'succeeded inside todo' /tmp/regtest-$LABEL.txt)"
grep -E 'Test failed|succeeded inside todo' "/tmp/regtest-$LABEL.txt" | sed -n '1,50p'
echo REGTEST_DONE
