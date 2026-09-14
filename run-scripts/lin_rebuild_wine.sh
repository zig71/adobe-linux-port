#!/bin/bash
# Incremental rebuild of the fork after a source change, then install.
set -e
BUILD=/home/kubuntu/adobe-wine-lab/src/wine/build64
PREFIX=/home/kubuntu/adobe-wine-lab/install/wine-upstream
cd "$BUILD"
make -j"$(nproc)" > make-inc.log 2>&1
echo "MAKE_DONE rc=$?"
make install > install-inc.log 2>&1
echo "INSTALL_DONE rc=$?"
"$PREFIX/bin/wine" --version
echo REBUILD_COMPLETE
