#!/bin/bash
# Clone upstream Wine + Wine Staging into the lab tree, record exact commits.
set -e
SRC=/home/kubuntu/adobe-wine-lab/src
mkdir -p "$SRC"
cd "$SRC"

if [ ! -d wine/.git ]; then
  echo "== cloning upstream wine =="
  git clone --filter=blob:none https://gitlab.winehq.org/wine/wine.git wine
fi

if [ ! -d wine-staging/.git ]; then
  echo "== cloning wine-staging =="
  git clone --filter=blob:none https://github.com/wine-staging/wine-staging.git wine-staging
fi

cd wine
git fetch --all --tags --quiet || true
echo "WINE_HEAD=$(git rev-parse HEAD)"
echo "WINE_DESCRIBE=$(git describe --tags --always 2>/dev/null || echo none)"
git log -1 --format='WINE_COMMIT_DATE=%ci'

cd "$SRC/wine-staging"
git fetch --all --tags --quiet || true
echo "STAGING_HEAD=$(git rev-parse HEAD)"
git log -1 --format='STAGING_COMMIT_DATE=%ci'

echo CLONE_DONE
