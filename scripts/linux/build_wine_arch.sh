#!/bin/bash
# Reconfigure the existing build tree IN PLACE (never rm -rf it) so the newly
# installed mingw C++ front-ends are picked up, then build incrementally.
# Reports the wall time of each phase so build cost is known, not guessed.
set -e
SRC=/home/kubuntu/adobe-wine-lab/src/wine
BUILD=$SRC/build-arch
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-arch

export CC="ccache gcc"
export CXX="ccache g++"

cd "$BUILD"

T0=$(date +%s)
"$SRC/configure" \
    --enable-win64 \
    --enable-archs=i386,x86_64 \
    --prefix="$PREFIX" \
    CFLAGS="-O2 -g -fno-omit-frame-pointer" > configure.log 2>&1
T1=$(date +%s)
echo "CONFIGURE_DONE rc=$? elapsed=$((T1-T0))s"
grep -E "PE compiler supporting C\+\+17" configure.log && echo "CXX17_STILL_MISSING" || echo "CXX17_OK"

make -j"$(nproc)" > make.log 2>&1
T2=$(date +%s)
echo "MAKE_DONE rc=$? elapsed=$((T2-T1))s"

make install > install.log 2>&1
T3=$(date +%s)
echo "INSTALL_DONE rc=$? elapsed=$((T3-T2))s"
echo "TOTAL_ELAPSED=$((T3-T0))s"

"$PREFIX/bin/wine" --version
ccache -s | head -6
echo ARCH_REBUILD_COMPLETE
