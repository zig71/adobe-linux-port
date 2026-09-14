#!/bin/bash
# Provide a real Vulkan device for the GPU-P passthrough GPU, via Mesa's dzn
# (Vulkan on D3D12). /usr/lib/wsl/lib/libd3d12.so already talks to the GPU
# through dxgkrnl, so dzn is the path that makes the passthrough GPU usable for
# Vulkan -- and therefore the prerequisite for Wine's DXVK/vkd3d and for any
# GPU-backed browser engine.
set -e
SRC=/home/kubuntu/adobe-wine-lab/src
MESA=$SRC/mesa

as_root() { echo ${LAB_PW} | sudo -S -p '' "$@"; }

echo "== dependencies =="
as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
    directx-headers-dev python3-mako python3-yaml python3-packaging \
    libexpat1-dev libzstd-dev zlib1g-dev libelf-dev \
    wayland-protocols libwayland-dev \
    libxcb-dri3-dev libxcb-present-dev libxcb-sync-dev libxcb-xfixes0-dev \
    libxcb-shm0-dev libxcb-randr0-dev libxcb-glx0-dev libxcb1-dev x11proto-dev \
    libxshmfence-dev libxcb-keysyms1-dev \
    glslang-tools spirv-tools libvulkan-dev \
    >/tmp/apt-dzn.log 2>&1
echo "APT_DONE rc=$?"

if [ ! -d "$MESA/.git" ]; then
  echo "== fetching mesa =="
  git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git "$MESA" >/tmp/mesa-clone.log 2>&1
fi
echo "MESA_HEAD=$(cd $MESA && git rev-parse HEAD)"

cd "$MESA"
rm -rf build
echo "== configuring dzn only =="
meson setup build \
    --prefix=/usr/local \
    --buildtype=release \
    -Dvulkan-drivers=microsoft-experimental \
    -Dgallium-drivers= \
    -Dopengl=false \
    -Dglx=disabled \
    -Degl=disabled \
    -Dgles1=disabled \
    -Dgles2=disabled \
    -Dgbm=disabled \
    -Dllvm=disabled \
    -Dplatforms=x11 \
    -Dbuild-tests=false \
    > /tmp/meson-setup.log 2>&1
echo "MESON_SETUP rc=$?"
grep -iE 'dzn|error' /tmp/meson-setup.log | tail -10

if [ ! -f build/build.ninja ]; then
  echo "MESON_FAILED"; tail -40 /tmp/meson-setup.log; exit 1
fi

ninja -C build > /tmp/ninja-dzn.log 2>&1
echo "NINJA rc=$?"

as_root ninja -C build install > /tmp/ninja-install.log 2>&1
echo "INSTALL rc=$?"
as_root ldconfig

find / -xdev -name 'libvulkan_dzn*' 2>/dev/null

echo "== writing ICD manifest =="
as_root tee /usr/share/vulkan/icd.d/dzn_icd.x86_64.json >/dev/null <<'EOF'
{
    "ICD": {
        "api_version": "1.3.0",
        "library_path": "libvulkan_dzn.so"
    },
    "file_format_version": "1.0.1"
}
EOF

echo "== enumeration =="
vulkaninfo --summary 2>&1 | head -30
echo "DZN_BUILD_COMPLETE"
