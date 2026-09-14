#!/bin/bash
# Configure + build the staging tree (both archs), install to install-staging.
set -u
SRC=/home/kubuntu/adobe-wine-lab/src/wine-staging-work
BUILD=$SRC/build-staging
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-staging
mkdir -p "$BUILD"
cd "$BUILD"
if [ ! -f Makefile ]; then
  "$SRC/configure" \
      --enable-win64 \
      --enable-archs=i386,x86_64 \
      --prefix="$PREFIX" \
      --disable-tests \
      CFLAGS="-O2 -g -fno-omit-frame-pointer" > configure.log 2>&1
fi
echo "CONFIGURE_DONE rc=$?"
make -j"$(nproc)" > make.log 2>&1
echo "MAKE_RC=$?"
make install > install.log 2>&1
echo "INSTALL_RC=$?"
"$PREFIX/bin/wine" --version 2>&1 | head -2
echo STAGING_BUILD_DONE
