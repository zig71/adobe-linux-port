# LAB_STATUS — Adobe Wine compatibility lab

Updated: 2026-09-13 (third revision). Installer runs, passes all gates, and
renders its sign-in form interactively under Wine; blocked on Adobe credentials.
Prior revisions below remain for the record but §§6–7 are superseded by §9.

## 1. Access — VERIFIED

| Target | Endpoint | Guest | Result |
|---|---|---|---|
| Windows oracle | `winnie@192.168.164.18` | Win11-25H2, Windows 11 Home build 26200.9445 | reachable, authenticated |
| Linux target | `kubuntu@192.168.166.252` | Kubuntu-26.04, Ubuntu 26.04.1, kernel 7.0.0-31-generic | reachable, authenticated |

Non-interactive automation uses `plink.exe`/`pscp.exe` with pinned host keys.
Helpers: `scripts/lab.mjs` (session-independent), `scripts/run_diff.mjs`
(differential harness).

## 2. Toolchain — READY

| Capability | Windows oracle | Linux target |
|---|---|---|
| Compiler | MSVC 14.51 (`cl`) + Windows SDK 10.0.28000 | gcc 15.2, clang 21.1, mingw-w64 13 |
| Build system | — | make 4.4, cmake 4.2, ninja, meson |
| Observability | WPR/WPA/xperf, WinDbgX, Sysinternals (ProcMon/ProcDump/ProcExp installed), Windows SDK Debuggers | gdb, lldb, strace, ltrace, perf, bpftrace, valgrind, coredumpctl |
| Wine | — | **built**: `wine-11.17-203-g788d90c4e1` (upstream) + patched `…-204-g666b4f4dfa`, installed at `~/adobe-wine-lab/install/wine-upstream` |
| Sources | — | `src/wine` @ `788d90c4e1d628fab6672623f0c8094b984ea2fa`, `src/wine-staging` @ `dc020173dc36007d06a88ff3fc5866de7faba826` |
| Adobe apps | **not installed** | — |

## 3. GPU — VERIFIED ON BOTH

Same physical RTX 3080 through Hyper-V GPU-P on both guests: UUID
`GPU-c125cbbd-f0a0-94ee-ef29-33f439bbc51b`, VBIOS `94.02.26.40.fc`, driver
595.95, CUDA 13.2, FB 10240 MiB, CC 8.6. `tests/probe_gpu.c` prints
byte-identical output on both (`cuCtxCreate=0`, `PROBE_RESULT=OK`).

See `inventory/GPU_VERIFICATION.md`. Outstanding gap: **no NVIDIA Vulkan ICD on
Linux** (`vulkaninfo` enumerates llvmpipe only), which blocks GPU-backed
D3D11/D3D12/DXVK work until resolved.

## 4. Differential harness — WORKING

`scripts/run_diff.mjs` cross-compiles each probe once with mingw-w64, deploys the
**same PE** to both guests, runs it on Windows and under Wine, and writes
`runs/<run-id>/{windows,linux,comparison}` plus `metadata.json`.

```
bun scripts/run_diff.mjs                       # all probes
bun scripts/run_diff.mjs probe_reggetvalue     # one probe
LAB_WINE=<path> LAB_PREFIX=<path> bun scripts/run_diff.mjs   # exercise a fork build
```

Probes implemented: `probe_osinfo`, `probe_registry`, `probe_reggetvalue`,
`probe_rgv_size`, `probe_rgv_slack`, `probe_com`, `probe_dxgi`,
`probe_directwrite`, `probe_mf`, `probe_crypto`, `probe_gpu`.

## 5. Findings

### Landed: divergence #1 — `RegGetValue` type restrictions

`RegGetValueA`/`RegGetValueW` rejected `RRF_RT_REG_EXPAND_SZ` without
`RRF_NOEXPAND` up front with `ERROR_INVALID_PARAMETER`, before reading the value.
Windows applies the type restriction to the type **as returned** (after
expansion), yielding `ERROR_UNSUPPORTED_TYPE` and still filling `*pdwType` and
`*pcbData`. Wine also wrongly rejected the legitimate
`RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ` query.

- Fix: `patches/0001-kernelbase-Apply-RegGetValue-type-restrictions-to-th.patch`
  (`dlls/kernelbase/registry.c`, removes the pre-check in both entry points).
- Test: `dlls/advapi32/tests/registry.c` extended; the existing
  `RRF_RT_REG_EXPAND_SZ` assertion tightened to the Windows 8+ value.
- Verified: `runs/20260911-223652-diff/` — all 33 return-code/type divergences
  from `runs/20260911-221954-diff/` are gone; `advapi32:registry` failures
  dropped from 24 to 18 (the 18 are the pre-existing WOW64 view failures).

### Landed: divergence #2 — `RegGetValue` size reporting

`RegGetValueA`/`RegGetValueW` reported the size derived from the data they
*return*, where Windows reports the size of the buffer it *read* — and Windows
additionally reserves terminator slack. With `R` = stored size, `S` = `R` with a
terminator ensured, `E` = expanded length:

- non-expanded: buffer form `max(S, 1)`; size-only form `S + 1`
  (`REG_MULTI_SZ`: `S + 2`), and `1` for a value with no data at all;
- expanded: `max(E + 1, S)`, plus one when `E + 1 > S`.

- Fix: `patches/0002-kernelbase-Report-the-value-size-RegGetValue-uses-no.patch`.
- Test: new `test_get_value_size()` pins the matrix for both entry points; three
  stale `todo_wine` markers removed; two size-only expectations in the
  string/multistring termination tests corrected to `insize + 2`.
- Verified: `probe_reggetvalue` 33 → **0**, `probe_rgv_size` 10 → **0**,
  `probe_rgv_slack` 11 → **0**, `probe_rgv_slack2` 11 → **0**,
  `probe_rgv_raw` 8 → **0**, `probe_rgv_more` 2 → **0**,
  `probe_rgv_inner` 1 → **0**;
  `advapi32:registry` 24 failures → **18**, none inside a `todo_wine` block.

Two candidate rules were refuted by the oracle before the correct one was
established; details and the refutations are in
`notes/divergence-002-reggetvalue-size.md`.

### Open: other divergences with captured evidence

`probe_registry` (7), `probe_crypto` (8) and `probe_osinfo` (11) still diverge.
None is investigated yet. `probe_osinfo`'s differences are largely prefix
configuration (reported build, `DisplayVersion`, `UBR`, edition) rather than API
defects, but they materially change which code paths an installer takes, so they
are worth reconciling deliberately.

## 6. Current focus: the Creative Cloud installer

`Creative_Cloud_Set-Up.exe`
(SHA256 `bc0c63263f7a40f6886fa528598309c150cc068b0dd54a9d46d25f145cc886f8`,
identical on both VMs) is a 32-bit UPX-packed GUI bootstrapper whose UI is a
React app in **Microsoft Edge WebView2**, with an **IE WebBrowser** fallback via
`ieframe`/`urlmon`. Under Wine the fallback is the path that must work, since
Wine has no WebView2 support.

Progress: from **not starting at all** (`c0000135` on `syswow64\ntdll.dll`) to
its browser stack **working**.

| change | effect |
|---|---|
| `--enable-archs=i386,x86_64` (WoW64) | installer executes |
| `g++-mingw-w64-{i686,x86-64}` | `msvcp*` modules no longer silently skipped (`CXX17_OK`) |
| Mesa Dozen (`microsoft-experimental`) | GPU-P GPU usable as a Vulkan device |
| wine-gecko 2.47.4 | mshtml can create documents; no more hang |

Working now, from `WINEDEBUG=+ieframe,+urlmon,+mshtml`:

```
ieframe:create_webbrowser ... version=2
ieframe:create_shell_embedding_hwnd parent=00040084 hwnd=0003007C
ieframe:...NavigateComplete2
mshtml:nsIOServiceHook_NewURI ("jar:.../gecko/2.47.4/wine_gecko/omni.ja!/chrome/toolkit/res/html.css")
```

Those previously-fatal lines are gone:

```
fixme:urlmon:create_object Could not find object for MIME L"text/html"
fixme:ieframe:bind_to_object BindToObject failed: 80040154
```

**Active blocker.** Wine's `WAM.log` reaches "Application initialized
successfully", creates its host directory and completes two
`POST https://cc-api-data.adobe.io/ingest` calls with HTTP 200, but does not log
the workflow states the oracle logs (`ACQUIRE_LOCKS` … `START_SIGNIN_WORKFLOW`).
The same HTTP sequence took 2 s on Windows and 31 s under Wine, consistent with
software rendering. Needs a longer instrumented run.

## 7. Next steps

1. Capture pixels from the Wine session to see whether the window really renders
   (X11 grab on this Wayland session returns only the black root window).
2. Isolate the wait after "Application initialized successfully".
3. Work the remaining `probe_mime` gaps (33 of 148 content types;
   `application/xhtml+xml` not creatable).
4. Consider WebView2 support in Wine — the largest structural gap, and generic
   to many applications.
5. Install Adobe applications on the oracle at a pinned version once the
   installer completes end to end.

## 7. Decisions needed from the human

- Adobe version/epoch to install on the oracle (the oracle is the reference and
  must stay reproducible; a checkpoint after install is advisable).
- Whether the Linux Vulkan path may use the WSL NVIDIA Vulkan ICD
  (`VK_ICD_FILENAMES`) or must be solved inside Wine/Mesa.
- Approval to install packages inside the guests (nothing has been installed on
  the host, and nothing needs to be).

## 8. Host changes

**None made. None required so far.** Hyper-V, GPU-P, host networking and
checkpoints are untouched.
## 9. Milestone 2026-09-13 — installer works up to sign-in (credentials needed)

Fresh `Creative_Cloud_Set-Up_747d.exe` under `wine-arch` + `prefix-wv2` logs all
7 oracle workflow states (`ACQUIRE_LOCKS` … `START_SIGNIN_WORKFLOW`), zero
alerts, and paints the Adobe IMS sign-in form pixel-perfect
(`artifacts/cc-paint.png`); the form is fully interactive (xdotool click + type
+ caret, `artifacts/cc-typed.png`, cleared after).

| change | effect |
|---|---|
| Genuine Edge WebView2 153 (not a shim) + DXVK for `d3d11`/`dxgi` | real browser stack |
| Oracle-parity OS identity (build/edition/`DisplayVersion`/UBR/`ReleaseId`/…) | requirements gate passes |
| `d3d12`/`d3d12core` disabled via `HKCU\Software\Wine\DllOverrides` (documented, reversible) | avoids unusable-Dozen path |
| DXVK 1.10.3 → **3.1** + `dxvk.allowCpuDevices=True` + llvmpipe filter | `D3D11CreateDevice S_OK` (was `0x80070057`) → frames present |

Root cause of the white window (platform, not Wine): only Vulkan devices are
Dozen-at-1.2.362 (non-conformant, below DXVK's needs) and llvmpipe, which old
DXVK unconditionally skips. Proven by `tests/probes/probe_d3d11.cpp` (fails
pre-fix, passes post-fix). Named-pipe IPC also cleared
(`tests/probes/probe_pipe.c`, full echo `gle=0`); Adobe's `OtherInstaller` 536
fails safe by design.

Recipe: `remote/lin_paint.sh` (`DXVK_CONFIG_FILE=~/adobe-wine-lab/dxvk-lvp.conf`,
`DXVK_FILTER_DEVICE_NAME=llvmpipe`), config in `remote/dxvk-lvp.conf`,
sign-in automation staged (`remote/lin_signin_stage{1,2}.sh`). Live session
parked on `:99` awaiting input. **Blocked: Adobe email + password** (2FA
heads-up if enabled) and approval to run the real install (GBs, writes real
Adobe files into the prefix). Details in `notes/progress.md` (2026-09-13
entries).
