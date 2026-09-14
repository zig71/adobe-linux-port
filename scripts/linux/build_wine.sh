#!/bin/bash
# Build upstream Wine (64-bit) from the lab source tree.
set -e
SRC=/home/kubuntu/adobe-wine-lab/src/wine
BUILD=/home/kubuntu/adobe-wine-lab/src/wine/build64
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-upstream

mkdir -p "$BUILD"
cd "$BUILD"
if [ ! -f Makefile ]; then
  "$SRC/configure" --enable-win64 --prefix="$PREFIX" --disable-tests \
    CFLAGS="-O2 -g -fno-omit-frame-pointer" > configure.log 2>&1
fi
echo "CONFIGURE_DONE rc=$?"
make -j"$(nproc)" > make.log 2>&1
echo "MAKE_DONE rc=$?"
make install > install.log 2>&1
echo "INSTALL_DONE rc=$?"
"$PREFIX/bin/wine" --version
echo WINE_BUILD_COMPLETE
