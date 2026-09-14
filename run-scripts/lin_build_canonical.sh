#!/bin/bash
# Install the mingw C++ front-ends, then rebuild the canonical fork with both
# PE architectures (i386 + x86_64) and the Wine test suite.
#
# Without g++-mingw-w64, Wine's configure reports
#   "PE compiler supporting C++17 not found, some modules won't be built"
# and silently skips the C++ modules (the msvcp* runtime reimplementations).
set -e
export DEBIAN_FRONTEND=noninteractive

echo "== installing mingw C++ front-ends =="
apt-get install -y g++-mingw-w64-x86-64 g++-mingw-w64-i686 >/tmp/apt-cxx.log 2>&1
echo "APT_DONE rc=$?"
for c in i686-w64-mingw32-g++ x86_64-w64-mingw32-g++ i686-w64-mingw32-gcc-posix x86_64-w64-mingw32-gcc-posix; do
  printf '%-34s ' "$c"; command -v "$c" >/dev/null && "$c" --version | head -1 || echo MISSING
done

SRC=/home/kubuntu/adobe-wine-lab/src/wine
BUILD=$SRC/build-arch
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-arch

echo "== reconfigure + rebuild (i386,x86_64, tests) =="
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"
"$SRC/configure" \
    --enable-win64 \
    --enable-archs=i386,x86_64 \
    --prefix="$PREFIX" \
    CFLAGS="-O2 -g -fno-omit-frame-pointer" > configure.log 2>&1
echo "CONFIGURE_DONE rc=$?"
grep -E "PE compiler supporting C\+\+17" configure.log || echo "NO_CXX17_WARNING"
make -j"$(nproc)" > make.log 2>&1
echo "MAKE_DONE rc=$?"
make install > install.log 2>&1
echo "INSTALL_DONE rc=$?"
"$PREFIX/bin/wine" --version
echo CANONICAL_BUILD_COMPLETE
