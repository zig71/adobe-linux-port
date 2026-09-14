#!/bin/bash
# Rebuild build64 with the Wine test suite enabled (only needed once).
set -e
SRC=/home/kubuntu/adobe-wine-lab/src/wine
BUILD=$SRC/build64
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-upstream
cd "$BUILD"
"$SRC/configure" --enable-win64 --prefix="$PREFIX" \
    CFLAGS="-O2 -g -fno-omit-frame-pointer" > configure-tests.log 2>&1
echo "CONFIGURE_DONE rc=$?"
make -j"$(nproc)" > make-tests.log 2>&1
echo "MAKE_DONE rc=$?"
make install > install-tests.log 2>&1
echo "INSTALL_DONE rc=$?"
"$PREFIX/bin/wine" --version
echo TESTS_BUILD_COMPLETE
