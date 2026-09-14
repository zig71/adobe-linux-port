# Vulkan on the GPU-P passthrough GPU (solved)

## Problem

The GPU reaches this guest through Hyper-V GPU-P: `/dev/dxg` + the `dxgkrnl`
kernel module, with the user-mode stack in `/usr/lib/wsl/lib`
(`libd3d12.so`, `libd3d12core.so`, `libdxcore.so`, `libcuda.so.1.1`).

Ubuntu's `mesa-vulkan-drivers` ships no Vulkan driver that can use that path.
`vulkaninfo --summary` therefore reported only `llvmpipe` (CPU), so Wine had no
real Vulkan device for DXVK/vkd3d, and no GPU-backed browser engine could work.

There is no `libnvidia-vulkan*` on this system and no `nvidia_icd.json`: the
Linux NVIDIA userspace for GPU-P (the WSL driver set) does not include a Vulkan
driver. `/usr/lib/wsl/lib/libnvwgf2umx.so` is the D3D12 user-mode driver, not a
Vulkan one.

## Fix

Use Mesa's **Dozen** (`dzn`) — a Vulkan implementation layered on D3D12. Mesa
renamed the option to `microsoft-experimental`. Because
`/usr/lib/wsl/lib/libd3d12.so` already speaks to the GPU through `dxgkrnl`, Dozen
reaches the physical GPU with no driver change and no host modification.

```
meson setup build \
    -Dvulkan-drivers=microsoft-experimental \
    -Dgallium-drivers= -Dopengl=false -Dglx=disabled -Degl=disabled \
    -Dgles1=disabled -Dgles2=disabled -Dgbm=disabled -Dllvm=disabled \
    -Dplatforms=x11 -Dbuild-tests=false
ninja -C build && sudo ninja -C build install
```

Mesa `26.3.0-devel (git-fa9938acf2)`. Installed to
`/usr/local/lib/x86_64-linux-gnu/libvulkan_dzn.so` with Mesa's own manifest at
`/usr/local/share/vulkan/icd.d/dzn_icd.x86_64.json`.

## Verification

`vulkaninfo --summary`:

```
GPU0:
    apiVersion   = 1.2.362
    deviceType   = PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
    deviceName   = Microsoft Direct3D12 (NVIDIA GeForce RTX 3080)
    driverName   = Dozen
    driverInfo   = Mesa 26.3.0-devel (git-fa9938acf2)
GPU1:
    apiVersion   = 1.4.335
    deviceType   = PHYSICAL_DEVICE_TYPE_CPU
    deviceName   = llvmpipe (LLVM 21.1.8, 256 bits)
```

Instance and device creation on the physical GPU (throwaway program, `gcc`
against `-lvulkan`):

```
vkCreateInstance=0
physical_devices=2
  [0] Microsoft Direct3D12 (NVIDIA GeForce RTX 3080) | type=2 | api=1.2.362
  [1] llvmpipe (LLVM 21.1.8, 256 bits) | type=4 | api=1.4.335
vkCreateDevice=0
```

`deviceType=2` is `PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` — the passthrough 3080, as
a usable Vulkan device.

## Notes and limits

- API level is **Vulkan 1.2**, not 1.4. Dozen is newer and less complete than
  the native NVIDIA driver; features above 1.2 are unavailable. For DXVK
  (D3D9/10/11) that is generally workable; for vkd3d-proton (D3D12) it is tight
  and worth measuring per feature before relying on it.
- `-Dllvm=disabled` kept the build small; Dozen does not need LLVM.
- No host configuration was changed: no GPU-P reallocation, no Hyper-V settings,
  no driver replacement. The driver *is* still the host's, reached through
  D3D12/dxgkrnl.
- The ICD is in `/usr/local`, so it survives reboots but is not managed by apt.
  Rebuilding Mesa would need the same `meson` invocation.
