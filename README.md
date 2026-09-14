# adobe-linux-port — Wine compatibility work for Adobe Creative Cloud

Real Windows behavior is the oracle. Every patch here exists because one
small standalone probe printed different observable results on Windows and
under Wine (`Windows returns X, Wine returns Y`).

## Current status (2026-09-14)

- **Installer: works.** `Creative_Cloud_Set-Up` runs end to end to 100%
  (`LAUNCH_CCD` → `POST_PRODUCT_INSTALL` → `TERMINATE_LBS`, zero alerts),
  logs all 7 oracle workflow states (`ACQUIRE_LOCKS` …
  `START_SIGNIN_WORKFLOW`), and paints a fully interactive Adobe IMS
  sign-in form. User-verified click-through on `:0`.
- **Main app (CC Desktop): paints but does not finish init.** Backend is
  green (main, ADS, IPCBroker×2, CoreSync, UI helpers; NGL sync cycling,
  CDN flows active), and the UI renders real stages (Adobe "A" splash →
  "Initializing Creative Cloud…" spinner → update banner). Init parks on
  content/applet loading: `GSDKStore onRequestTimeout`,
  `ContentStore: CDN fallback disabled`,
  `Unable to initialize core library`,
  `RegisterObservers: No target applet found`. Transports exonerated by
  same-PE probes (pipes sync + overlapped, WinINET fetch ~3× native, locks
  slower-not-broken, full DXGI matrix incl. swapchain Present).
- Details: `STATUS.md`. Full lab record: `LAB_STATUS.md`, `notes/progress.md`.

## What is in this repo

| Path | Contents |
|---|---|
| `patches/` | 8 Wine patches + tests (0001/0002 RegGetValue, 0003-win32u metrics, 0003-winex11 Unmap/BadWindow, 0004 dcomp, 0005 FlsGetValue2, 0006 GetProductInfo + 0006b test) |
| `src-dcomp/` | Unpacked `dlls/dcomp` tree behind patch 0004 (device/target/surface/transform/misc/guids) |
| `tests/` `tests/probes/` | 36 minimal cross-platform reproducers (`.c`/`.cpp`, source only): registry, OS identity, D3D11/DXGI/swapchain, pipes, HTTP, locks, race, MIME, WebView2 slots, dcomp, window metrics |
| `scripts/` | Differential harness (`run_diff.mjs`), SSH helpers (`lab.mjs`), `pe_imports.py`, VM control stubs; `scripts/linux/` build/trace helpers from the lab VM |
| `run-scripts/` | 226 battle-tested run/trace recipes from the Sep 13–14 campaign (`lin_paint.sh`, `lin_run_cc*.sh`, `lin_cc*.sh`, `lin_cef*.sh`, `dxvk-lvp.conf`, oracle `*.ps1/*.bat`, `*.reg`) — passwords redacted to `${LAB_PW}` |
| `notes/` | Divergence analyses + engineering journal (`progress.md`) |
| `inventory/` | Machine/GPU/driver inventory + verification notes |
| `evidence/` | Small captured divergences (MIME DB, dcomp, regkeys, browser trace) |
| `LAB_STATUS.md`, `LAB_README.md`, `AGENTS.md` | Lab topology, loop, and agent operating rules |

What was deliberately left out, and how to reproduce it: `OMISSIONS.md`.

## Reproducing the working installer setup

Reference commits: upstream Wine `788d90c4e1` (wine-11.17-203), staging
`dc020173`; fork builds used `--enable-archs=i386,x86_64` (WoW64 — the
32-bit installer will not start without it).

1. Build Wine with all patches in `patches/` applied in numeric order
   (0003-winex11 `unmap.diff` applies to `dlls/winex11.drv/x11drv_main.c`).
2. Install `wine-gecko-2.47.4` (mshtml needs it; without it the installer hangs
   on an unanswerable download prompt) and `g++-mingw-w64-{i686,x86-64}`
   (without the C++17 PE compilers Wine silently skips `msvcp*`).
3. Use genuine Edge WebView2 runtime 153 + **DXVK 3.1** (1.10.3 cannot build
   any feature level on Dozen-1.2 + llvmpipe) with `run-scripts/dxvk-lvp.conf`
   (`dxvk.allowCpuDevices=True`, `DXVK_FILTER_DEVICE_NAME=llvmpipe`) and
   disable `d3d12`/`d3d12core` via `HKCU\Software\Wine\DllOverrides`.
4. Mirror the oracle OS identity (build/edition/`DisplayVersion`/UBR/
   `ReleaseId`, see `run-scripts/oracle_identity.reg` + `lin_h1/h2/h3*.sh` +
   patch 0006) or the requirements gate fails.
5. Run per `run-scripts/lin_paint.sh`; expect the 7 `state:` lines and zero
   `not supported` alerts in `WAM.log`.
6. Prove each subsystem first with the probes:
   `probe_d3d11.cpp` (fails pre-DXVK-fix, passes post-fix),
   `probe_pipe.c` (full echo `gle=0`), `probe_osinfo.c` (gate inputs).

Reference installer SHA256:
`bc0c63263f7a40f6886fa528598309c150cc068b0dd54a9d46d25f145cc886f8`
(`Creative_Cloud_Set-Up.exe`; later run used `Creative_Cloud_Set-Up_747d.exe`).
Binaries are **not** in this repo — bring your own Adobe download.

## Rules of this work

- Do not modify Adobe binaries; no auth/licensing/DRM bypass; no
  app-name checks where generic Windows-compatible behavior can be
  implemented (`AGENTS.md`).
- A patch needs a reproducer that runs unchanged on both platforms plus a
  captured deterministic difference (HRESULT / GetLastError / callback /
  output bytes).
- Guest VMs only; host/Hyper-V/GPU-P/checkpoints need explicit approval.

## Outstanding tracks (see `STATUS.md`)

1. CEF116 (Chrome/116) shared-image backing: `CreateSharedImage: could not
   create backing` → GPU exit 34 → crash loop; no failing DXGI call in the
   DXVK debug log — failure is inside ANGLE/Chromium logic.
2. Stale-parent `X_CreateWindow` BadWindow after explorer teardown
   (operational rule: `wineserver -k` after killing explorer); patch 0003
   covers only the Unmap-after-destroy race.
3. Prefetch pacing (~15,000× below oracle by volume, per-request costs
   ruled out): contended-lock probe, then RenderDoc ANGLE capture.
