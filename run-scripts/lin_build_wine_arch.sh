#!/bin/bash
# Build Wine for both i386 and x86_64 PE targets from one 64-bit tree
# (Wine's "new WoW64"), so 32-bit installers and helpers run.
set -e
SRC=/home/kubuntu/adobe-wine-lab/src/wine
BUILD=$SRC/build-arch
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-arch

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
grep -iE 'i386|x86_64|wow64' configure.log | tail -10
make -j"$(nproc)" > make.log 2>&1
echo "MAKE_DONE rc=$?"
make install > install.log 2>&1
echo "INSTALL_DONE rc=$?"
"$PREFIX/bin/wine" --version
echo ARCH_BUILD_COMPLETE
