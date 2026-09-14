# dcomp: implementing DirectComposition

## Why this module

Wine bug **58921** names DirectComposition as the reason WebView2 fails under
Wine — `DCompositionCreateDevice failed: Not implemented (0x80004001)` — and bug
**59370** (blank WebView2 windows) was closed as a duplicate of it.
`probe_dcomp.c` reproduced it on this build: Wine returned `E_NOTIMPL` from all
three create functions and **faulted** in
`__wine_spec_unimplemented_stub` for `DCompositionCreateSurfaceHandle`, where
Windows returns a real handle.

Before this work `dlls/dcomp` was a skeleton: `device.c` was 47 lines, every
function a `FIXME` returning `E_NOTIMPL`, and the spec was `@ stub` throughout.

## Before and after

Same binary (`tests/probes/probe_dcomp.exe`) on both platforms:

| call | Windows | Wine before | Wine after |
|---|---|---|---|
| `DCompositionCreateDevice` | `E_NOINTERFACE` (rejects `IID_IUnknown` with a NULL device) | `E_NOTIMPL` | **`S_OK`, non-null device** |
| `DCompositionCreateDevice2` | `E_NOINTERFACE` | `E_NOTIMPL` | **`S_OK`, non-null device** |
| `DCompositionCreateDevice3` | `E_NOINTERFACE` | `E_NOTIMPL` | **`S_OK`, non-null device** |
| `DCompositionCreateSurfaceHandle` | `S_OK`, real handle | **faults in the unimplemented stub** | `E_NOTIMPL` (fails cleanly) |

The remaining difference on the create functions is strictness, not capability:
Windows validates its arguments and refuses a NULL rendering device, where this
implementation accepts it. That is a deliberate gap to close later, not a
regression — the previous behaviour was `E_NOTIMPL` for every caller.

`DCompositionCreateSurfaceHandle` is not implemented. It now returns
`E_NOTIMPL` instead of faulting, so a caller fails predictably rather than
crashing the process.

`DCompositionWaitForCompositorClock` was **missing from the spec entirely**
while Chromium references it. It is now exported and waits on the caller's
handles; there is no compositor clock to synchronise with, which the
implementation documents.

## What is implemented

| object | state |
|---|---|
| `IDCompositionDevice` (v1) | full, 24 methods |
| `IDCompositionDevice2` / `3` | full, incl. `CreateSurfaceFactory` |
| `IDCompositionDesktopDevice` | full, `CreateTargetForHwnd` + surface accessors |
| `IDCompositionTarget` | `SetRoot` |
| `IDCompositionVisual` / `Visual2` | full: offsets, transform object, matrix, transform parent, effect, clip object, clip rect, content, child list, interpolation/border/composite/opacity/backface modes |
| `IDCompositionSurface` / `VirtualSurface` | real D3D11 texture backing with `D3D11_RESOURCE_MISC_SHARED`; `BeginDraw`/`EndDraw`/`SuspendDraw`/`ResumeDraw`; `Resize`/`Trim` |
| `IDCompositionSurfaceFactory` | full |
| transforms (translate, scale, matrix, skew, rotate) | full state, including `SetMatrixElement` |
| `IDCompositionRectangleClip` | all 24 setters |
| `IDCompositionEffectGroup` | `SetOpacity` (validated), `SetTransform3D` |
| `IDCompositionAnimation` | object with correct identity; curve segments accepted |

### Deliberately not implemented

- **3D transforms** and transform groups — `E_NOTIMPL`.
- **Filter effects** (Gaussian blur, colour matrix, …) — all 13 device3 creators
  return `E_NOTIMPL`.
- **Animation evaluation** — segments are accepted, no clock is driven, so
  animated output is the constant default rather than a computed value.
- **`DCompositionCreateSurfaceHandle` / `CreateSurfaceFromHandle` /
  `CreateSurfaceFromHWnd`** — cross-process and window-content surfaces.
- **Actual presentation.** `Commit` records device state; it does not composite
  to the screen. Nothing here blits a surface into a target window yet.

Each of these is reported through `FIXME`, not silently swallowed.

## Verification

Built for both architectures from one tree
(`--enable-archs=i386,x86_64`):

```
dlls/dcomp/i386-windows/dcomp.dll     615409 bytes
dlls/dcomp/x86_64-windows/dcomp.dll   763066 bytes
```

against 126995 / 150925 bytes for the previous stub — i.e. real code, not a
larger stub.

## What this did NOT fix

**The genuine Microsoft WebView2 runtime still fails.**
`probe_wv2_internal.exe`, which drives the installer's exact path
(`CreateWebViewEnvironmentWithOptionsInternal` from
`EBWebView\x86\EmbeddedBrowserWebView.dll`), still faults at the same place:

```
STEP2_load_x86_ok=1
STEP2_export_present=1
wine: Unhandled page fault on execute access to 00000000
```

with the environment still never created. So DirectComposition was a real gap
and is now closed, but it is **not** the only thing standing between Wine and
the real runtime. The NULL function pointer Chromium calls through is a separate
missing capability that has not yet been identified.

This is worth stating plainly because the upstream bug report implies
DirectComposition is *the* blocker; on this evidence it is necessary but not
sufficient.

## Implementation notes worth keeping

Three problems cost real time and will recur for anyone touching this module:

1. **The generated headers define `IDCompositionDevice3_Commit(This)` as a
   macro.** Any implementation with the same name is macro-expanded into
   nonsense. The implementations here are named `dcomp_device3_Commit` etc. and
   all calls go through `->lpVtbl->` explicitly rather than via the
   `IDL_Interface_Method()` wrappers.

2. **`<initguid.h>` cannot be used here.** It turns every `DEFINE_GUID` of every
   transitively included header into a *definition*, which then clashes with the
   copies in `libuuid` and `libdxguid` — hundreds of duplicate symbols. `guids.c`
   writes out the 26 GUIDs this module actually references, in full, and the
   module links no UUID or DXGUID library at all. `shobjidl.h` also pulls in
   `msxml.h`, whose two GUIDs are additionally suppressed because they clash
   with libuuid.

3. **Derived vtables use derived `This` types.** `IDCompositionDevice3Vtbl`'s
   inherited methods take `IDCompositionDevice3 *`, not
   `IDCompositionDevice2 *`, so the three exposed interfaces cannot share
   function pointers and are generated from a common body by macro.

## Files

- `dlls/dcomp/{device,target,surface,transform,misc,guids}.c`
- `dlls/dcomp/dcomp_private.h`, `Makefile.in`, `dcomp.spec`
- Patch: `patches/0004-dcomp-Implement-device-target-visual-surface-and-transforms.patch`
- Probe: `tests/probes/probe_dcomp.c`
- Evidence: `evidence/dcomp-divergence.txt`
