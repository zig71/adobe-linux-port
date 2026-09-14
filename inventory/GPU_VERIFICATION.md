# GPU verification — Adobe Wine Lab

Raw evidence that both guests see the same physical RTX 3080 through
Hyper-V GPU-P, and that the CUDA driver stack allocates a context on each.

## GPU-P assignment

| Property | Windows oracle (Win11-25H2) | Linux target (Kubuntu-26.04) |
|---|---|---|
| Device | NVIDIA GeForce RTX 3080 | NVIDIA GeForce RTX 3080 |
| GPU UUID | GPU-c125cbbd-f0a0-94ee-ef29-33f439bbc51b | GPU-c125cbbd-f0a0-94ee-ef29-33f439bbc51b |
| VBIOS | 94.02.26.40.fc | 94.02.26.40.fc |
| PCI BDF (guest view) | (WDDM adapter) | `0B:00.0` (nvml), lspci shows `4133:00:00.0` MS Basic Render Driver for the paravirtual node |
| Driver version | 595.95 (WDDM) | 595.95 (`nvidia-smi`), userspace 595.61 in `/usr/lib/wsl/lib` |
| CUDA version | 13.2 | 13.2 |
| Reported FB | 10240 MiB (10 GiB partition) | 10240 MiB (10 GiB partition) |
| cuDeviceTotalMem | 10736893952 bytes | 10736893952 bytes |
| Compute capability | 8.6 | 8.6 |

Identical UUID, VBIOS, driver version, VRAM size and CUDA runtime => the two
20/25% partitions are consistent. Full 3080 = 10240 MiB, so each partition is
the reported 10 GiB slice (Hyper-V GPU-P advertises the partition size as FB).

## CUDA context creation (accelerated compute path)

`tests/probe_gpu.c` dynamically loads `nvcuda.dll` / `libcuda.so.1`.

Windows (`tests/build/probe_gpu.exe`), exit 0:

    CUDA_LIB=nvcuda.dll LOAD=OK
    cuInit=0 (no error)
    cuDriverGetVersion=0 value=13020
    cuDeviceGetCount=0 count=1
    cuDeviceGetName=0 name=NVIDIA GeForce RTX 3080
    cuDeviceTotalMem=0 bytes=10736893952
    cuAttrCCMajor=0 value=8
    cuAttrCCMinor=0 value=6
    cuCtxCreate=0 ctx=non-null (no error)
    cuCtxDestroy=0
    PROBE_RESULT=OK

Linux (native `gcc`, `/usr/lib/wsl/lib/libcuda.so.1` via `/dev/dxg`), exit 0:

    CUDA_LIB=libcuda.so.1 LOAD=OK
    cuInit=0 (no error)
    cuDriverGetVersion=0 value=13020
    cuDeviceGetCount=0 count=1
    cuDeviceGetName=0 name=NVIDIA GeForce RTX 3080
    cuDeviceTotalMem=0 bytes=10736893952
    cuAttrCCMajor=0 value=8
    cuAttrCCMinor=0 value=6
    cuCtxCreate=0 ctx=non-null (no error)
    cuCtxDestroy=0
    PROBE_RESULT=OK

Byte-for-byte identical output. GPU acceleration is confirmed on both guests.

## Linux graphics stack details

- `/dev/dxg` present (`dxgkrnl` kernel module, DKMS `dxgkrnl/6.18-147941806`).
- `/usr/lib/wsl/lib/` holds the GPU-P userspace: `libcuda.so.1`, `libdxcore.so`,
  `libd3d12.so`, `libd3d12core.so`, `libnvidia-encode.so.1`, `libnvcuvid.so.1`,
  `libnvwgf2umx.so`, `libnvoptix.so.1`, `nvidia-smi`.
- `dxgkrnl` + `/usr/lib/wsl/*` are the WSL2-style GPU-P passthrough; this is
  what makes a *Vulkan* ICD for NVIDIA on Linux plausible (via `libdxcore.so`,
  as used by Mesa's `dzn`/D3D12 and by the `libnvidia-vulkan` WSL driver).

## Gap: no NVIDIA Vulkan ICD on Linux (as shipped)

    $ cat /usr/share/vulkan/icd.d/*.json   # only Mesa: lvp, nouveau, intel, radeon, virtio, asahi, gfxstream
    $ find / -xdev -name '*nvidia*icd*'    # nothing

`vulkaninfo --summary` therefore enumerates only `llvmpipe` (CPU). The NVIDIA
Windows guest has the full WDDM user-mode driver; the Linux guest currently
has CUDA + D3D12 (via dxcore) but **no Vulkan**.

Consequence for Wine: `winevulkan`/DXVK require a real Vulkan device. Installing
a Windows-Vulkan-ICD (VK_ICD_FILENAMES) or fixing the Linux NVIDIA Vulkan path
is a prerequisite for D3D11/D3D12 Adobe workloads, but **not** for the first
milestone (which targets a non-graphics behavioral divergence).
