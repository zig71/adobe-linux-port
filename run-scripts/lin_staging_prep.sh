#!/bin/bash
# Build a staging-based Wine (upstream base + staging patchset, NO local
# patches) as a pure experiment: does the installer progress further there?
set -u
SRC=/home/kubuntu/adobe-wine-lab/src
cd "$SRC" || exit 1
if [ ! -d wine-staging-work ]; then
  git clone -q -s wine wine-staging-work 2>&1 | head -2
  cd wine-staging-work
  git checkout -q -b staging-exp 788d90c4e1d628fab6672623f0c8094b984ea2fa 2>&1 | head -2
  git log --oneline -1
else
  cd wine-staging-work
fi
echo "=== applying staging patchset ==="
../wine-staging/staging/patchinstall.sh . --all > /tmp/staging-patch.log 2>&1
echo "PATCH_RC=$?"
grep -a -c -E '^Applying' /tmp/staging-patch.log 2>/dev/null
grep -a -E 'FAILED|Failed|failed' /tmp/staging-patch.log 2>/dev/null | head -10
echo STAGING_PREP_DONE
