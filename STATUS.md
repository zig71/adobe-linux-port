# STATUS — 2026-09-14

Source: lab journal `notes/progress.md` (Sep 13–14 entries) + `LAB_STATUS.md` §9.
Nothing here is inferred beyond those files.

## Installer: WORKS (user-verified)

- Fresh `Creative_Cloud_Set-Up_747d.exe` under `wine-arch` + `prefix-wv2`
  logs all 7 oracle workflow states (`ACQUIRE_LOCKS` …
  `START_SIGNIN_WORKFLOW`), zero alerts, paints the Adobe IMS sign-in form
  pixel-perfect and fully interactive (click + type + caret).
- Click-through on `:0`: 100% → `LAUNCH_CCD` → `POST_PRODUCT_INSTALL` →
  `TERMINATE_LBS`, zero alerts. Oracle timeline captured for comparison
  (`cc.pml` 4 GB + `cc-1.pml` 1.8 GB + 23 timed screenshots — not in repo,
  see `OMISSIONS.md`).
- Recipe that made it work: genuine Edge WebView2 153 (not a shim) + DXVK
  **3.1** + `dxvk.allowCpuDevices=True` + llvmpipe filter +
  oracle-parity OS identity (patch 0006) + `d3d12`/`d3d12core` disabled via
  `DllOverrides`. Root cause of the white window was platform, not Wine:
  only Vulkan devices were Dozen-at-1.2.362 (non-conformant, below DXVK's
  needs) and llvmpipe, which old DXVK unconditionally skips. Proven by
  `tests/probes/probe_d3d11.cpp` (fails pre-fix, passes post-fix).
  Named-pipe IPC also cleared (`tests/probes/probe_pipe.c`, full echo `gle=0`).
- Entry point: `run-scripts/lin_paint.sh` with
  `run-scripts/dxvk-lvp.conf`; sign-in automation staged in
  `run-scripts/lin_signin_stage{1,2}.sh`.

## Main app (CC Desktop): paints, init blocked on content/applet loading

- Backend all green: main, ADS, IPCBroker×2, CoreSync, 5 UI helpers;
  manifests syncing; NGL sync cycling; CDN flows active with queued bytes.
- UI renders real stages, each taking minutes on llvmpipe (slow, not stuck):
  Adobe "A" logo splash (perfectly rasterized — GPU/raster pipeline works) →
  "Initializing Creative Cloud…" spinner + blue update banner. No input
  deadness at splash stage.
- Init parks here (`CEF.log` / app logs): `GSDKStore onRequestTimeout`,
  `ContentStore: CDN fallback disabled`,
  `Unable to initialize core library`,
  `RegisterObservers: No target applet found` — applets never load, UI waits.
  GrowthSDK 12 MB vs oracle 136 MB (same account), trickling at ~KB/min
  while the oracle filled in ~2 min on the same network.
- Ruled out with same-PE probes (all in `tests/probes/`): named pipes sync
  (200/200) + overlapped (200/200 small + 20/20 × 1 MB in 8 s), WinINET
  fetch (~3× native — pacing, not speed), file locks (Wine 170 µs/op vs
  oracle <0.5 µs/op — slower, not broken), full DXGI matrix (device, shared
  all formats, keyedmutex+acquire, RTV/SRV, swapchain hidden/shown +
  Present, fences QI-match). Two *reversed* divergences noted (Wine accepts,
  Windows rejects: shared RGBA8/RGBA16F/RGB10A2/NV12 creation; legacy
  GetSharedHandle on swapchain buffers) — likely harmless, unproven.
- CEF diagnosis (Chrome/116): `CreateSharedImage: could not create backing`
  → GPU exit 34 → crash loop → `FATAL: GPU process isn't usable`. DXVK debug
  log shows zero errors — failure is inside ANGLE/Chromium logic, no failing
  DXGI call. Ruled out: NT-handle theory (oracle rejects the probe call
  identically), GL fallback (worse — reverted), wined3d instead of DXVK
  (identical mailbox errors — NOT DXVK-specific), win8 for CEF children
  (no more crash loop but still no mailboxes).

## Landed fixes in this snapshot

1. **patches/0003-winex11-unmap.diff** — ignore BadWindow on
   `X_UnmapWindow` (withdraw racing destroy across thread Display
   connections). 3 post-patch app launches, 0 X errors (pre-patch ~2/3
   fatal). Incremental `winex11.drv`-only rebuild; rollback is
   `/tmp/winex11.*.bak` on the lab VM (not in repo).
2. **DXVK 3.1 + config** (`run-scripts/dxvk-lvp.conf`, corrected
   `dxvk.nvapiHack=False`), llvmpipe filter, `d3d12` disable.
3. **Oracle-parity OS identity** (patch 0006 + `0006b` test +
   `run-scripts/oracle_identity.reg`, `lin_h1/h2/h3*.sh`).
4. Probes added: `probe_http.c`, `probe_lock.c`, `probe_race.c` (+xthread),
   `probe_swapchain.cpp` (two-phase), `probe_d3d11.cpp`, `probe_pipe*.c`.
5. Operational rule: killing explorer requires `wineserver -k` before
   relaunch (wineserver retains the dead desktop mapping; new windows
   parent to the corpse → `X_CreateWindow` BadWindow, a NEW signature
   patch 0003 does not cover).

## Open tracks

1. **CEF116 shared-image backing** — needs ANGLE-side error; no flag
   injection into Adobe's CEF exists. Options: minimal-CEF repro or newer CEF.
2. **Stale-parent CreateWindow failure** — arguably a real error (window
   cannot exist), unlike unmapping one (a no-op). Needs a decision, not a
   hasty extension of 0003.
3. **Prefetch pacing** — per-request costs ruled out; volume is small (21
   DB files, WAL churn, ~24 fds/process pools, no leak, clocks agree to 2 s,
   no CEF.log JS errors). Next: contended-lock probe (two processes), then
   RenderDoc ANGLE-usage capture if rendering regresses; x11drv lock-side
   review for the withdraw path.
4. **KDE Remote Control portal** gates xdotool (synthetic input intercepted;
   physical input unaffected). No further synthetic input until the console
   decision. Scripts using xdotool (`lin_input_probe.sh`, `lin_burger.sh`,
   sign-in stages) need re-validation after the decision.

## Awaiting user decisions

1. Approve/Deny the KDE Remote Control prompt at the console.
2. Click Update? (newer build may obsolete this chase — but rewrites the install).
3. Attempt Photoshop install? (needs working CC UI or HDBox-direct path).
4. Standing lab questions (`LAB_STATUS.md` §7): Adobe version/epoch to pin on
   the oracle; whether the Linux Vulkan path may use the WSL NVIDIA ICD;
   approval to install packages inside guests / run the real GB-scale install.
