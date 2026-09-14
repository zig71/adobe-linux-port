# Engineering journal — Adobe Wine lab

Format per entry: symptom / evidence / Windows behavior / Wine behavior /
root cause / patch-test status / next step.

---

## 2026-09-11 — Lab bootstrap

**Symptom** — none (baseline).

**Evidence** — `inventory/windows.txt`, `inventory/linux.txt`,
`inventory/GPU_VERIFICATION.md`, `runs/20260911-2310-bootstrap/metadata.json`.

- Windows oracle: Win11-25H2 build 26200.9445, MSVC 2026 `cl`, Windows SDK
  28000, WPR/WPA, Sysinternals installed.
- Linux target: Kubuntu 26.04.1, kernel 7.0.0-31, Wine not installed (installed
  during bootstrap), no source trees, no Vulkan ICD for the passthrough GPU.
- Both guests expose the same physical RTX 3080 (UUID `GPU-c125cbbd-…`,
  driver 595.95, CUDA 13.2, 10240 MiB FB, CC 8.6).
- `tests/probe_gpu.c` prints byte-identical output on both (CUDA context OK on
  both). `tests/hello_win32.c` builds on Windows with MSVC and runs under Wine.

**Root cause** — n/a (setup).

**Patch/test status** — lab tree, differential harness (`scripts/run_diff.mjs`)
and probe infrastructure created. Upstream Wine
`788d90c4e1d628fab6672623f0c8094b984ea2fa` (wine-11.17-203) and wine-staging
`dc020173dc36007d06a88ff3fc5866de7faba826` cloned into `src/`.

**Next step** — build upstream Wine and start the differential loop.

---

## 2026-09-11 — Toolchain built

Upstream Wine configured with `--enable-win64` and built in `build64/`,
installed to `install/wine-upstream` as
`wine-11.17-203-g788d90c4e1`. A second configure run (same options, tests
enabled) produces `wine-11.17-204-g666b4f4dfa` once the fix below is applied.

Note for later: this build is **64-bit only**; the `advapi32:registry`
conformance test therefore reports 18 pre-existing failures in the
`KEY_WOW64_*` view tests (lines 3215–3312), which need a WoW64-enabled build.
They are unrelated to the registry behaviour under investigation.

---

## 2026-09-11 — MILESTONE 1 LANDED: `RegGetValueA/W` type restrictions

**Symptom** — `RegGetValueA`/`RegGetValueW` reject legitimate flag combinations
and report the wrong type-restriction error. Found by systematic differential
probing of an Adobe-relevant subsystem (registry semantics: installers,
licensing, per-user configuration), not from an application crash.

**Evidence**

- Reproducer: `tests/probes/probe_reggetvalue.c` (40-case matrix),
  `tests/probes/probe_registry.c`, `tests/probes/probe_rgv_size.c`.
- Pre-fix: `runs/20260911-221954-diff/` — 33 divergences.
- Post-fix: `runs/20260911-223652-diff/` — all return-code and type divergences
  eliminated.
- Conformance: `advapi32:registry` — **24 failures pre-fix (6 in
  `test_get_value`) → 18 failures post-fix (0 in `test_get_value`)**; the 18
  remaining are the pre-existing WOW64-view failures noted above.

**Windows behavior (oracle).** Type restrictions are applied to the type **as
returned**, i.e. after a `REG_EXPAND_SZ` value has been expanded to `REG_SZ`:

| value type | flags | Windows |
|---|---|---|
| `REG_EXPAND_SZ` | `RRF_RT_REG_SZ` | `ERROR_SUCCESS`, type `REG_SZ`, expanded |
| `REG_EXPAND_SZ` | `RRF_RT_REG_SZ\|RRF_RT_REG_EXPAND_SZ` | `ERROR_SUCCESS`, type `REG_SZ` |
| `REG_EXPAND_SZ` | `RRF_RT_REG_EXPAND_SZ` (no `RRF_NOEXPAND`) | `ERROR_UNSUPPORTED_TYPE` |
| `REG_EXPAND_SZ` | `RRF_RT_REG_DWORD` / `REG_MULTI_SZ` / `REG_NONE` | `ERROR_UNSUPPORTED_TYPE` |
| `REG_DWORD` | `RRF_RT_REG_EXPAND_SZ` | `ERROR_UNSUPPORTED_TYPE` |

On every failure Windows still fills `*pdwType` and `*pcbData`.

**Wine behavior (pre-fix).** `dlls/kernelbase/registry.c` short-circuited before
reading the value:

```c
if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
        ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
    return ERROR_INVALID_PARAMETER;
```

so those cases returned `ERROR_INVALID_PARAMETER` (0x57) leaving `*pdwType` and
`*pcbData` untouched, and the legitimate
`RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ` query failed outright.

**Root cause.** The flag combination was rejected up front instead of being
resolved by the post-expansion type check that already exists
(`apply_restrictions()`). The pre-check dates from the original 2019 import
(`c7548d6c4fc`, Alexandre Julliard). Wine's own test tolerated both values with
the comment "before win8: ERROR_INVALID_PARAMETER, win8:
ERROR_UNSUPPORTED_TYPE", which is why the divergence survived.

**Patch/test status**

- `patches/0001-kernelbase-Apply-RegGetValue-type-restrictions-to-th.patch`
  (`dlls/kernelbase/registry.c`): removes the pre-check from both entry points so
  `apply_restrictions()` decides; `NOTES` corrected.
- `dlls/advapi32/tests/registry.c`: existing `RRF_RT_REG_EXPAND_SZ` assertion
  tightened to the Windows 8+ value (`broken()` for older Windows), plus new
  coverage for `RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ`, non-matching restrictions on
  an expanded value, and a non-string value restricted to `REG_EXPAND_SZ`.
- Verified to fail pre-fix and pass post-fix (see the conformance numbers above).

**Next step** — divergence #2 below.

---

## 2026-09-11 — Divergence #2 opened: `RegGetValue` size reporting

**Resolved the same day — see MILESTONE 2 below.** Kept for the record.

**Symptom.** Size-only queries (`pvData = NULL`) return a different `*pcbData`
than Windows for string values. Surfaced by the same probes once the
return-code divergences were fixed.

**Windows behavior.** Non-expanded: stored size with NUL ensured, plus one extra
character (`REG_MULTI_SZ`: two). Expanded: never below the raw size, growing to
`expanded + 1` when the expansion lengthens the string.

**Wine behavior.** Non-expanded: stored size with NUL ensured, nothing extra.
Expanded: expanded length (+2 in the size-only form).

**Evidence.** `runs/20260911-223945-diff/` (`probe_rgv_slack.c`) and
`runs/20260911-224950-diff/` (`probe_rgv_slack2.c`) — an 11-row matrix across
`REG_SZ`, `REG_MULTI_SZ`, `REG_EXPAND_SZ` with and without `RRF_NOEXPAND`, and
values whose expansion is longer, shorter and equal to the raw form.

**Root cause.** Not yet established. Two candidate formulas were tried and
**refuted** by the `%SystemRoot%`-expanded row; a fit that covers all eleven rows
requires a special case, which is a sign it is not the real algorithm.

**Patch/test status.** Not patched. Full matrix, refuted hypotheses and the
candidate model recorded in `notes/divergence-002-reggetvalue-size.md`.

**Next step.** Determine the real quantity Windows tracks (most likely the size
of the scratch buffer it allocates for the raw value) with a probe that varies
whether the stored value carries a trailing NUL and where the expansion is
exactly one character longer than the raw form.

---

## 2026-09-11 — MILESTONE 2 LANDED: `RegGetValue` size reporting

**Symptom.** Size-only queries (`pvData = NULL`) returned a different `*pcbData`
than Windows for string values.

**Evidence**

- Probes: `probe_rgv_size.c`, `probe_rgv_slack.c`, `probe_rgv_slack2.c`,
  `probe_rgv_raw.c`, `probe_rgv_more.c`, `probe_rgv_inner.c`, `probe_expand.c`.
- Raw runs: `runs/20260911-224950-diff/` (11-row matrix),
  `runs/20260911-225158-diff/` (delta sweep), `runs/20260911-225239-diff/`
  (buffer-size sweep), `runs/20260911-225803-diff/` (`ExpandEnvironmentStrings`).
- Fix: `patches/0002-kernelbase-Report-the-value-size-RegGetValue-uses-no.patch`.

**Windows behavior.** `RegGetValue` reports the size of the buffer it worked
from, not the size of the data it returns. With `R` = stored size, `S` = `R`
with a terminator ensured, and `E` = expanded length:

- non-expanded: buffer form `max(S, 1)`; size-only form `S + 1`
  (`REG_MULTI_SZ`: `S + 2`), and `1` when the value has no data at all;
- expanded: `max(E + 1, S)`, plus one when `E + 1 > S`.

`*pdwType` and `*pcbData` are filled even on failure.

**Wine behavior (pre-fix).** Reported the size derived from the *returned* data:
the stored size with nothing extra, and the expanded length for expanded values —
both smaller than Windows, and larger in the case where the expansion shortens
the value.

**Root cause.** Wine recomputed the size from the returned data instead of the
raw buffer it read, and never reserved the terminator slack Windows does. A
second defect surfaced during the fix: `ExpandEnvironmentStringsA` returns
*needed + 1* when the destination cannot take the result (Wine reproduces this
correctly — `probe_expand` diverges 0), so its return value cannot be used
directly as a required size. My first attempt did exactly that and
over-corrected the expanded path; the oracle caught it.

**Patch/test status.** Fixed in `dlls/kernelbase/registry.c`; new
`test_get_value_size()` pins the matrix for both entry points; three stale
`todo_wine` markers removed; two size-only expectations in the string- and
multistring-termination tests corrected from `insize + 1` to `insize + 2`.

| probe | before | after |
|---|---|---|
| `probe_reggetvalue` | 33 | **0** |
| `probe_rgv_size` | 10 | **0** |
| `probe_rgv_slack` | 11 | **0** |
| `probe_rgv_slack2` | 11 | **0** |
| `probe_rgv_raw` | 8 | **0** |
| `probe_rgv_more` | 2 | **0** |
| `probe_rgv_inner` | 1 | **0** |

`advapi32:registry`: 24 failures pre-fix → **18**, with no `todo_wine` block
succeeding. The 18 are the pre-existing `KEY_WOW64_*` view failures of this
64-bit-only build.

**Method note.** Two candidate rules were proposed and **refuted by the data**
before the correct one was found; a third "fits everything" formula needed a
special case and was rejected as a curve fit. The rule was closed only by
identifying the real quantity — the raw buffer size — and confirming it across
lengths 0/1/9/10/20/21/25 and both entry points.

**Next step.** See standing gaps.

---

## 2026-09-12 — Creative Cloud installer: from dead to browser stack working

**Symptom.** The installer would not run at all under Wine:
`wine: failed to load L"\\??\\C:\\windows\\syswow64\\ntdll.dll" error c0000135`.

**Evidence.** `notes/creative-cloud-installer-analysis.md`,
`notes/divergence-003-mime-database.md`, `evidence/cc-wine-browser-trace.txt`,
`evidence/windows-mime-database.txt`, `evidence/cc-install-oracle/WAM.log`,
`runs/20260912-001415-diff/`, `runs/20260912-004707-diff/`.

**What the installer is.** `Creative_Cloud_Set-Up.exe`, SHA256
`bc0c63263f7a40f6886fa528598309c150cc068b0dd54a9d46d25f145cc886f8`, identical on
both VMs. PE32 **i386**, UPX-packed (10.8 MB unpacked), GUI subsystem. Its UI is
a React app hosted in **Microsoft Edge WebView2** (12→24 `msedgewebview2.exe`
processes on the oracle), with a fallback to the legacy IE WebBrowser control via
`ieframe`/`urlmon`. Wine has **zero** WebView2 support — not even a stub — so
under Wine the IE fallback is the path that must work.

**Oracle reference (Windows).** Reaches `ACQUIRE_LOCKS` →
`CHECK_GENERAL_SYSTEM_REQUIREMENTS` → `CHECK_FOR_PROXY` → `CHECK_FOR_NETWORK` →
`CHECK_ALREADY_INSTALLED_PRODUCT` → `SHOW_WELCOME_SCREEN` →
`START_SIGNIN_WORKFLOW`, then waits at the sign-in screen.

**Fixes applied this round**

1. **WoW64.** Rebuilt the fork with `--enable-archs=i386,x86_64`. The installer
   now executes.
2. **mingw C++ front-ends.** `i686-w64-mingw32-g++` was missing, so configure
   reported "PE compiler supporting C++17 not found, some modules won't be
   built" and Wine silently skipped its `msvcp*` runtime modules. Installed
   `g++-mingw-w64-{i686,x86-64}`, reconfigured: `CXX17_OK`, `msvcp140` etc. now
   built for both architectures.
3. **Vulkan.** Built Mesa Dozen (`-Dvulkan-drivers=microsoft-experimental`), so
   the GPU-P passthrough GPU is a usable Vulkan device — see
   `inventory/VULKAN_DRIVER.md`.
4. **Gecko 2.47.4.** Installed `wine-gecko-2.47.4-{x86,x86_64}.msi`. Without it
   mshtml cannot create its document object and `install_wine_gecko()` blocks
   forever on an unanswerable download prompt.
5. **Harness bug (mine).** `scripts/run_diff.mjs` passed
   `WINEDLLOVERRIDES='mscoree,mshtml='` to every Wine probe run. That disables
   mshtml — the module that registers the MIME database *and* handles
   `text/html` — so it masked the very behaviour `probe_mime` was measuring and
   produced a false divergence. Override removed.

**Result.** The installer's browser stack now works. With WoW64 + Gecko + MIME
database:

```
ieframe:create_webbrowser ... version=2
ieframe:create_shell_embedding_hwnd parent=00040084 hwnd=0003007C
ieframe:...NavigateComplete2
mshtml:nsIOServiceHook_NewURI ("jar:.../gecko/2.47.4/wine_gecko/omni.ja!/chrome/toolkit/res/html.css")
```

and the previous failures are gone:

```
fixme:urlmon:create_object Could not find object for MIME L"text/html"
fixme:ieframe:bind_to_object BindToObject failed: 80040154
```

`probe_mime` divergences went 22 → 5 (remaining: Wine registers 33 of Windows'
148 content types, `application/xhtml+xml` not creatable, and CLSID case).

**Root cause of the installer's halt (partially open).** Wine's `WAM.log`
reaches "Application initialized successfully", creates its host directory and
completes two `POST https://cc-api-data.adobe.io/ingest` calls with HTTP 200 —
but does not log the workflow states the oracle logs. The same HTTP sequence
took 2 s on Windows and 31 s under Wine, consistent with software rendering
(llvmpipe, 1024×768, Wayland/Xwayland). Whether it eventually proceeds or is
genuinely stuck needs a longer run with the window mapped; X11 capture on this
Wayland session returns only a black root window, so screenshots are not a
usable check.

**Next step.** Add a verbosity cap on `+mshtml` (the channel is extremely noisy
and produced an 18 MB trace), then re-run with a self-managed kwin/compositor
grab to capture pixels, and isolate whichever wait follows
`Application initialized successfully`.

---

## 2026-09-12 — Window metrics bug found and fixed (installer still blocked)

**Symptom.** The installer halts after "Application initialized successfully"
and never reaches `onWindowResize` or any workflow state, while the Windows
oracle reaches both ~20 ms later.

**Evidence.** `notes/divergence-004-window-metrics.md`,
`runs/20260912-011121-diff/`, `runs/20260912-011358-diff/`,
`runs/20260912-011529-diff/`.

**Hypothesis 1 — WRONG.** I inferred from the installer's own log that Wine was
not delivering the resize notification to its window. `probe_window.c` counted
the messages a top-level window actually receives: **identical on both
platforms** (WM_CREATE 1, WM_NCCALCSIZE 1, WM_GETMINMAXINFO 1, WM_SIZE 1,
WM_WINDOWPOSCHANGED 1, WM_SHOWWINDOW 1, WM_MOVE 1, WM_ACTIVATE 1) with identical
geometry. Message delivery is not the problem.

**Root cause of a real, separate bug.** `probe_metrics.c` showed Wine returning
`SM_CYFULLSCREEN = 786` for a 768-high screen — a full-screen client area taller
than the display, self-inconsistent regardless of any comparison.
`dlls/win32u/sysparams.c` derived the pair inconsistently: the X case used
`2 * SM_CXFRAME` but the Y case used `2 * SM_CYCAPTION`, and
`SM_CYFULLSCREEN` came from `SM_CYMAXIMIZED - SM_CYMIN`.

**Fix.** `patches/0003-win32u-System-metrics-for-maximized-and-fullscreen-windows.patch`.
Windows' own numbers give the formulas exactly — `SM_CYMAXIMIZED = SM_CYSCREEN +
2 * SM_CYFRAME` (768+16=784) and `SM_CYFULLSCREEN = SM_CYSCREEN - SM_CYCAPTION`
(768−23=745). After the fix Wine reports 776 and 742, and the invariants hold on
both platforms.

**Honest outcome.** The fix is real and verified, and **did not unblock the
installer** — it still halts at the same point. Recorded as such rather than
presented as the gate.

**Next hypotheses.** (1) the installer's window is created on a thread whose
message loop differs, or a `SetWindowPos`/`ShowWindow` it needs is never issued —
needs a trace scoped to its own window handle; (2) the halt is inside the
embedded browser, which the trace shows is rendering, waiting on a page-load or
script-completion callback; (3) the four `RegOpenKeyExW failed with error 2`
warnings immediately before the stall (the oracle fails on the same keys, so
likely benign but unverified).

**Also learned.** Wine's `+msg` channel does not trace window messages — it only
logs internal calls such as `BroadcastSystemMessageExW`. Counting messages inside
a probe (as `probe_window.c` does) is the reliable way to test delivery.

---

## 2026-09-12 — Installer halt root-caused: the WebView2 probe

**Symptom.** Installer halts after "Application initialized successfully";
never shows its main window, never logs `onWindowResize`, never enters any
workflow state.

**Hypotheses chased in order, each tested.**

### H1 — Wine drops the resize notification. REFUTED.

`probe_window.c` counts the messages a top-level window receives: identical on
both platforms (WM_CREATE 1, WM_NCCALCSIZE 1, WM_GETMINMAXINFO 1, WM_SIZE 1,
WM_WINDOWPOSCHANGED 1, WM_SHOWWINDOW 1, WM_MOVE 1, WM_ACTIVATE 1), identical
geometry. Wine does send `WM_SIZE` (`dlls/win32u/window.c:6133`).

### H1b — Wine never sends `WM_SHOWWINDOW` to the main window. REFUTED as a Wine bug.

`+message` (not `+msg`) gave the full spy: the installer's window 0x10058
receives `WM_ACTIVATE`, `WM_SETFOCUS`, one `WM_SIZE` (1104x720) and **no
`WM_SHOWWINDOW`** — but its browser *children* do receive it. `+relay` then
showed why: **the installer never calls `ShowWindow` on its main window.** Every
`ShowWindow` in the run targets other windows, and the only `SetWindowPos` on
0x10058 carries `SWP_HIDEWINDOW`. Wine is not dropping a request that was never
made.

### H2 — the embedded browser never becomes ready. REFUTED.

`probe_webbrowser.c` hosts `CLSID_WebBrowser` as an installer does. Only 2
divergences, neither about readiness:

| | Windows | Wine |
|---|---|---|
| `READYSTATE_COMPLETE_is_4` | **1** | **1** |
| activation callback sequence | identical | identical |
| `QI_IViewObjectEx` | `E_NOINTERFACE` | `E_NOINTERFACE` |
| `IPersistStreamInit_Load` | `E_FAIL` | `S_OK` |
| `host_client` | 1008x681 | 1016x686 |

The fallback browser works. (The size difference is the frame/caption metric
divergence already covered in divergence #4.)

### H3 → root cause. CONFIRMED.

The installer's warning text differs between platforms, and that was the lead:

```
Windows: getRegistryValue: RegQueryValueExW failed with error 2   (value absent)
Wine:    getRegistryValue: RegOpenKeyExW failed with error 2      (key absent)
```

Windows has **zero** key-open failures; Wine has four. Pairing `Call`/`Ret` by
return address, restricted to the installer's own module, names them:

```
FAIL rv=00000002  L"Software\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
FAIL rv=00000002  L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-...}"
FAIL rv=00000002  L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-...}"   (64-bit view)
FAIL rv=00000002  L"Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
FAIL rv=00000002  L"Software\\Adobe\\ALCID"                       x2
FAIL rv=00000002  L"SOFTWARE\\Microsoft\\SQMClient"               x2
```

Three of the four are the **Edge WebView2 runtime probe**. The installer reads
`enableWebview2 : true` and then looks for the runtime. On the oracle the key
resolves (`pv = 153.0.4234.32`); on Wine it does not exist.

**Root cause.** Wine has no WebView2 — `grep -ril webview2 dlls/ include/
programs/` returns zero matches, not even a stub. So the installer falls back to
its IE WebBrowser path, and on that path it never shows its main window (H1b),
never gets its UI-ready signal, and the workflow never starts.

This explains every observation: main window never shown, `onWindowResize`
never logged, no workflow state, yet browser children created, shown and
rendering.

**Patch/test status.** No patch. The fix is implementing WebView2 in Wine —
`WebView2Loader.dll` plus the `ICoreWebView2*` surface — which is general work
affecting every WebView2 application, and large. Documented in
`notes/divergence-005-installer-webview2-probe.md`.

**Secondary divergences** (real, not the gate): `HKLM\SOFTWARE\Microsoft\SQMClient`
and `HKLM\Software\Policies\Microsoft\Windows\CurrentVersion\Internet Settings`
exist on Windows and are absent on Wine.

**Tooling note.** Wine's message spy is on the **`message`** channel, not `msg`.
`+msg` logs only internal calls and produced an empty trace cycle before this was
understood. The full kit is documented in `inventory/DEBUGGING_KIT.md`.

---

## 2026-09-12 — Tested: can the Windows WebView2 runtime run under Wine?

**Question.** Rather than implement WebView2, can the genuine runtime be copied
from the Windows oracle to the Linux target and used as-is?

**Tested end to end, not reasoned about.** Full detail in
`notes/webview2-portability-test.md`.

**Result — three of four layers work, the last does not.**

| layer | outcome |
|---|---|
| copy the 680 MB runtime across | **works** (`tar`; `Compress-Archive` silently produced 0 bytes) |
| load its binaries under Wine | **works** — `msedge.dll` (332 MB x86_64 Chromium core), `msedge_elf.dll`, `EmbeddedBrowserWebView.dll` (i386) all load, error 0 |
| satisfy the installer's runtime probe | **works** — output now identical to the oracle (`pv=153.0.4234.32`) |
| initialise a WebView2 environment | **fails** — immediate NULL call |

The discovery half of divergence #5 is therefore **resolved**: staging the
runtime and writing the `EdgeUpdate\Clients\{F3017226-...}` key (which the
runtime's own installer writes on Windows) makes the installer find it.

The environment creation is where it dies. `probe_wv2_internal.c` replicates the
installer's exact path — it uses `CreateWebViewEnvironmentWithOptionsInternal`
from `EBWebView\x86\EmbeddedBrowserWebView.dll`, not the public loader:

```
STEP1_open=0x00000000 (found)          STEP2_load_x86_ok=1 err=0
STEP1_pv=153.0.4234.32                 STEP2_export_present=1
wine: Unhandled page fault on execute access to 00000000
code=c0000005 (EXCEPTION_ACCESS_VIOLATION)  info[0]=8 (execute)  eip=00000000
```

Chromium's host calls through an unpopulated function pointer and jumps to NULL.
Corroborating: no WebView2 user-data directory is created, and the installer
falls back to mshtml.

**Why it is more than one missing API.** `msedgewebview2.exe` is x64-only and
requires `--initial-client-data` (Mojo IPC handles), so it cannot run
standalone; hosting the runtime means creating pipe-backed Mojo channels,
spawning an x64 helper and passing it those handles, and running Chromium's
sandbox model. Also: shipping Microsoft's proprietary runtime inside a Wine
patch could never go upstream.

**Recommendation.** Treat WebView2 as its own workstream. The upstreamable
direction is implementing the `ICoreWebView2*` surface backed by Wine's existing
Gecko — the same engine mshtml already drives successfully to
`READYSTATE_COMPLETE`. Keeping the runtime-under-Wine idea alive is worth one
bounded investigation into which capability the NULL call is missing, but
proceeding on the assumption that copying the files suffices is now measured to
be wrong.

---

## 2026-09-12 — Prior art research: Vinegar, Kombucha, and Wine's own MR 7032

**Asked.** Vinegar (Roblox Studio on Linux) supposedly handles the EdgeWebView
problem, so maybe its work can be lifted.

**Two premises corrected.**

1. **Vinegar is not a Wine fork.** It is a Go bootstrapper (GPL-3.0). The actual
   customized Wine build is **Kombucha** (vinegarhq/kombucha, LGPL-2.1).
2. **Kombucha has no WebView2 work at all.** All 24 patches are cursor locking
   on Wayland, registry overrides, virtual desktop, bitmap-only fonts, ntsync,
   memory-allocation redirection, ucrtbase search, desktop close timeout. Nothing
   browser- or composition-related. Nothing to lift.

**Vinegar's approach is the one already tested here.** It downloads Microsoft's
own WebView2 runtime installer and runs it in the prefix
(`bootstrapper_setup_pfx.go`: `webview2.Stable.Runtime(...)` then
`webview2.Install(...)`). Its troubleshooting page lists the result as a **known
issue** — black/white/unresponsive WebView2 window — with a workaround of
disabling web pages and logging in via the browser. That independently
corroborates `notes/webview2-portability-test.md`.

**What *is* liftable: Wine's own MR 7032.**
[MR 7032](https://gitlab.winehq.org/wine/wine/-/merge_requests/7032), "Add
embeddedbrowserwebview aka. WebView2 DLL stub", Bernhard Kölbl (CodeWeavers),
Dec 2024. Closed, unmerged, and absent from the tree — but Wine's own code under
LGPL-2.1, so clean to adopt. It provides:

- an `embeddedbrowserwebview` stub DLL whose 8 exports **match the real DLL
  shipped from the oracle exactly** (verified ordinal by ordinal);
- **`include/webview2.idl`, 953 lines**, declaring `ICoreWebView2`,
  `ICoreWebView2Controller`, `ICoreWebView2Environment` and their event
  handlers — the single largest piece of the work, already written;
- `wine.inf` fake-DLL registration under
  `EdgeWebView\Application\<ver>\EBWebView\x32|x64`.

Its author's stated intent is the same architecture proposed earlier:
"dynamically load our custom Chromium fork from here (just like MSHTML and
wine-Gecko)."

**The real blocker named and reproduced: DirectComposition.** Upstream bug
**58921** attributes WebView2's failure to `DCompositionCreateDevice failed: Not
implemented (0x80004001)`; bug **59370** (blank windows) is a duplicate.
`probe_dcomp.c` reproduces it on the current build:

| call | Windows | Wine 11.17 |
|---|---|---|
| `DCompositionCreateDevice{,2,3}` | `E_NOINTERFACE` (real impl) | `E_NOTIMPL` |
| `DCompositionCreateSurfaceHandle` | `S_OK`, real handle | faults in `__wine_spec_unimplemented_stub` |

Source confirms a skeleton: `dlls/dcomp/device.c` is **47 lines**, every function
a FIXME returning `E_NOTIMPL`; the spec is `@ stub` throughout. Evidence:
`evidence/dcomp-divergence.txt`.

**Also tested: bug 58921's `win8` workaround.** Applied to
`HKCU\Software\Wine\AppDefaults\{msedgewebview2.exe,Creative_Cloud_Set-Up.exe}`
and verified present in the registry, then the installer re-run. **No effect** —
no workflow state, no `onWindowResize`, no WebView2 user-data directory, no
`msedgewebview2` process. Consistent with the earlier trace: the browser process
never launches because the 32-bit host faults first, so a workaround aimed at
that process cannot help.

**Reframed plan.** Not "implement WebView2" as a monolith, but three pieces of
very different size:

1. **Implement `dlls/dcomp`** — the documented blocker, a general gap beyond
   WebView2, and independently testable today via `probe_dcomp.c`. Highest
   value and most upstreamable.
2. **Adopt MR 7032's stub and `webview2.idl`** for the interface surface.
3. **Back it with an engine** — Wine's Gecko (which mshtml already drives to
   `READYSTATE_COMPLETE`) or a Chromium fork in its own repo.

**Licence.** Vinegar is GPL-3.0 and could not be merged into Wine (LGPL-2.1+)
even if it were relevant. Kombucha is LGPL-2.1 but has nothing relevant. MR 7032
is Wine's own, LGPL-2.1. Full write-up: `notes/prior-art-webview2.md`.

---

## Standing gaps

- **Installer workflow halts** after "Application initialized successfully";
  it does not reach the states the oracle reaches. Timing suggests software
  rendering; needs a longer instrumented run. This is the active blocker.
- **Wine has no WebView2.** Not even a stub: no `WebView2Loader.dll`, no
  `ICoreWebView2*` interfaces. The installer's primary UI path is unavailable;
  it currently proceeds via the IE fallback. This is the largest structural gap
  for Creative Cloud and for many other modern Windows applications.
- **Vulkan is 1.2 via Dozen**, not the native driver's 1.4. DXVK is plausible;
  vkd3d-proton needs per-feature measurement.
- **Adobe applications not installed** on either VM.
- **Open divergences outside the registry work**, all with captured evidence:
  - `probe_mime`: Wine registers 33 of Windows' 148 content types;
    `application/xhtml+xml` not creatable.
  - `probe_registry`: `HKCR_txt` default value, `RegQueryMultipleValues`
    size accounting, 32-bit view of `ProgramFilesDir`.
  - `probe_crypto`: `CryptProtectData` output size, `CryptFindOIDInfo`,
    certificate store contents.
  - `probe_osinfo`: reported build/edition/`DisplayVersion` — prefix
    configuration rather than API defects, but it steers installer branches.

---

## 2026-09-12/13 — Installer reaches its requirements gate; WebView2 live

**Symptom.** Creative Cloud installer stops at `CHECK_GENERAL_SYSTEM_REQUIREMENTS`
with `Current OS is not supported` (errorAlert 21), while the oracle proceeds
through 7 states to `START_SIGNIN_WORKFLOW`.

**Evidence.**
- Timestamped `+relay` trace attributes every check-window call: the check
  opens `EdgeUpdate\ClientState\{F3017226-...}`, reads `EBWebView`, verifies
  runtime files, spawns two `msedgewebview2.exe` (Mojo pipes, `ERROR_IO_PENDING`
  as on Windows), enumerates versions — every observable succeeds — then alerts.
- `probe_osinfo` 11 → **0 divergences** (build/edition/suite/product/registry
  all match the oracle; new `RTL_SUITEMASK`/`GVE_SUITEMASK` rows included).
- Adobe's own docs name compat-mode version misidentification as an error-21
  cause; the `win7` AppDefaults workaround was therefore REMOVED (it made the
  browser report NT 6.1). Env creation works without it now.
- Genuine WebView2 Evergreen install run under Wine (exit 0); full ClientState.
- Installer window renders pixel-perfect (error-21 dialog screenshot).
- Requirements gate still fails. Per-type census proves ONLY
  `--type=gpu-process` children die (respawn loop → full shutdown ~t+75s);
  renderers/browser/utilities survive. Same int3 (`msedge+0x89D900A`) across
  wined3d, DXVK/Dozen, DXVK/llvmpipe and D3D-disabled configs → not the D3D
  backend. DWrite-variant int3 also seen. OneAuth C++ throw (E06D7363 from
  missing WinRT `WebAuthenticationCoreManager`) caught live but tolerated.
- DXVK was cross-installed by its setup script (x32 into system32); repaired
  (x64→system32, x32→syswow64, native overrides). Dozen (Vulkan 1.2) rejects
  all DXVK feature levels; llvmpipe Vulkan gives working D3D11 (11_1).
- corefonts installed (DirectWrite probe 14 → 10 divergences).

**Patches landed since last entry.**
- `0005` FlsGetValue2 (`kernelbase/thread.c` + specs).
- `0004` dcomp device/target/visual/surface/transforms (real D3D11 backing;
  presentation deliberately deferred).
- `0006` RtlGetProductInfo reports the running edition (Core→101/3) +
  `wSuiteMask` derived from EditionId (Core→768) + conformance test that
  stages/restores EditionId (`dlls/ntdll/version.c`,
  `dlls/kernel32/tests/version.c`).

**Root cause of gate (open).** Unknown; all local inputs match. Leading model:
the check waits on the WebView2 child (Mojo handshake / ready signal) and the
GPU crash loop makes it time out. H7 run (no check evaluated, browser alive)
and the oracle's own stall in a locked session both fit readiness-gating.

**Next.** (1) Identify the GPU int3 (stack walker + import resolution in
progress). (2) Staging-build A/B experiment running (`src/wine-staging-work`,
same base + full staging patchset) to test whether the crash is already fixed
upstream-side. (3) WinRT broker stub only if OneAuth proves fatal (currently
tolerated).

---

## 2026-09-13 — MILESTONE: installer reaches START_SIGNIN_WORKFLOW (7/7 states)

**Symptom.** Months of error-21 investigation ended by the binary, not the OS:
the Sep-11 bootstrapper (`bc0c63...`, 3314952 B) fails its requirements gate
under Wine no matter what the OS reports; the current Adobe build
(`8f994e20...`, 3313624 B, fetched from Adobe's own download API) passes it.

**Evidence.** `Creative_Cloud_Set-Up_747d.exe` under `wine-arch` + prefix-wv2:
`ACQUIRE_LOCKS` → `CHECK_GENERAL_SYSTEM_REQUIREMENTS` → `CHECK_FOR_PROXY` →
`CHECK_FOR_NETWORK` → `CHECK_ALREADY_INSTALLED_PRODUCT` →
`SHOW_WELCOME_SCREEN` → `START_SIGNIN_WORKFLOW` — all 7 oracle states, zero
`not supported` lines. The old binary's gate was stale (its allowlist logic
predates the current OS/build matrix); every registry/API fix stands on its
own differential evidence regardless.

**Standing work.** Sign-in needs a human Adobe account (licensing boundary —
not automated). GPU-process int3 loop persists in the background (Chromium
CHECK, backend-independent); browser survives via respawn. OneAuth C++ throw
(WinRT broker absent) tolerated. All prior Wine patches/probes unchanged.

---

## 2026-09-13 — GPU crash is Win10+-gated; win8 browser mode stabilizes everything

**Symptom.** `--type=gpu-process` children int3-crash in a loop (5+ dumps/run,
full shutdown ~t+75s); requirements gate fails at 9s; sign-in page never paints
(white window on the desktop).

**Evidence.**
- Per-type 1s census: ONLY gpu-process dies; browser/renderer/utility survive.
- Crash is backend-independent (wined3d, DXVK/Dozen, DXVK/llvmpipe, D3D
  disabled) and config-independent (fonts, Vulkan ICD).
- Version bisection via `HKCU\Software\Wine\AppDefaults\msedgewebview2.exe`:
  win7 → stable, **win8 → stable (12 alive, 0 dumps/90s)**, default (11) →
  crash loop. The crashing path is Win10+-only.
- With a stable browser the gate PASSES: win8 run reached all 7 states with
  zero alerts (`runs/` + WAM.log). Desktop run reproduces: `START_SIGNIN_WORKFLOW`
  at t=10s, 11 browser children alive, no errors.
- The win8 lie affects ONLY the browser processes; the installer itself keeps
  reporting the true 26200/Home identity (probe: 0 divergences).

**Status: WORKAROUND, not a fix.** It masks the underlying Win10+ GPU CHECK
(`msedge+0x89D900A`, backend-independent int3) instead of implementing what it
needs. Candidates in the Win10+ GPU path: IDCompositionDevice3 usage,
DWrite3, DXGI 1.4+ (OfferResources1), MF protected path, Dawn/WebGPU init.
The int3 hunt continues; the workaround stays documented here until the real
behavior lands.

**Also fixed this session.** Kill-pattern audit: every `pkill/pgrep` in
`remote/*.sh` now matches the `747d` binary name (37 files); piled-up
instances had been fighting over the Bootstrapper Lock and producing
confounded runs (incl. spurious errorAlert 81s).
---

## 2026-09-13 — MILESTONE: sign-in page paints (white-window root cause found)

**Symptom.** Installer window mapped at the right size but painted pure white
(verified by `xwd` pixel capture on `:99` and on the real `:0` desktop); DXGI
never presented (no `DXVK_HUD`), yet all browser processes lived and the
renderer navigated the Adobe IMS login flow (History DB shows
`auth.services.adobe.com` → `ims-na1.adobelogin.com/authorize`).

**Evidence.** `artifacts/cc-paint.png` (painted sign-in form),
`artifacts/cc-win.png` / `cc-hud.png` / `cc-sw.png` (white),
`tests/probes/probe_d3d11.{c,cpp}` (same PE both platforms),
`remote/lin_paint.sh`, `remote/dxvk-lvp.conf`, `/tmp/probe{,2,3,4,5}.out`.

**Root cause (platform, not Wine).** `probe_d3d11` showed
`D3D11CreateDevice(HW,BGRA) → 0x80070057` ("Requested feature level not
supported"): DXVK 1.10.3 cannot build any feature level on this box. The only
Vulkan device besides llvmpipe is Dozen exposing **Vulkan 1.2.362**
non-conformant (`dzn is not a conformant Vulkan implementation`), below what
DXVK needs; llvmpipe is unconditionally skipped ("Skipping CPU adapter") and
1.10.3 has no `allowCpuDevices` knob (confirmed via strings on `dxgi.dll`).
With no D3D11 device, Chromium instantiates no DXGI factory (zero DXVK log
lines across all runs) and composites nothing — white window, healthy
processes. The earlier crash-loop / gate-flakiness reasoning is superseded:
the `win8` AppDefaults lie was removed (dishonest version reporting — the
mission forbids it), the MS `d3d12` pair stays disabled via
`HKCU\Software\Wine\DllOverrides` (`d3d12`/`d3d12core` empty; documented,
reversible; `d3d11`/`dxgi` remain native), and the Sep-11 installer's stale
gate is not chased.

**Fix (config, documented).** DXVK 1.10.3 → **3.1** (`d3d11.dll`+`dxgi.dll`,
x64+x32, old pair backed up to `/tmp/dxvk110-backup/`), plus
`dxvk.allowCpuDevices = True` (`remote/dxvk-lvp.conf`) and
`DXVK_FILTER_DEVICE_NAME=llvmpipe`. Probe: `D3D11CreateDevice → S_OK`,
level `0xb000`, adapter `llvmpipe (LLVM 21.1.8)`. Installer run
(`remote/lin_paint.sh`, fresh `Creative_Cloud_Set-Up_747d.exe`): all 7 oracle
workflow states, zero alerts, and the sign-in form renders pixel-perfect
(email field, Continue, Google/Microsoft/Apple, artwork, footer).

**Patch/test status.** New probe `tests/probes/probe_d3d11.cpp` (fails
pre-fix `0x80070057`, passes post-fix `S_OK`) — kept as the D3D-canary. No
Wine source changed this round. Uncommitted tree state untouched
(`version.c` diffs predate this session). Three installs intact
(`wine-arch` in use; `wine-staging`, `wine-upstream` reference).

**Next step.** Sign-in needs the user's Adobe credentials (licensing boundary;
session alive on `:99` awaiting input). Then: post-login download/install
phases under the same recipe.

---

## 2026-09-13 — Sign-in form fully interactive (awaiting user credentials)

**Symptom.** None — verification that the painted page is live, not a static frame.

**Evidence.** `artifacts/cc-typed.png` (`adobe.test` in the email field, caret
visible, focus ring), `artifacts/cc-cleared.png` (field cleared after
`ctrl+a`/`BackSpace`), `remote/lin_input_probe.sh` (xdotool click+type on `:99`,
field geometry derived from `getwindowgeometry`).

**Result.** Mouse focus, keyboard input, caret rendering and field editing all
work through Wine + WebView2 + DXVK 3.1/llvmpipe. Session alive on `:99`
(installer + 13 browser procs). Recipe hardened: `remote/lin_paint.sh` now
points `DXVK_CONFIG_FILE` at `~/adobe-wine-lab/dxvk-lvp.conf` (persistent, was
`/tmp`). `remote/dxvk-lvp.conf` holds `dxvk.allowCpuDevices = True`.

**Next step.** User's Adobe credentials → type via xdotool (select-all+delete
first), screenshot each auth step, continue into download/install phases.

## 2026-09-13 — Named-pipe check: mechanism sound, Adobe 536 fails safe

**Symptom.** `OtherInstallerHandler: Error initializing OtherInstaller IPC`
(`CommBridge: ... inPipe ... err = 536` = `ERROR_PIPE_LISTENING`) — assessing
whether Wine named pipes will block post-sign-in helper coordination.

**Evidence.** `tests/probes/probe_pipe.c` (two-process server/client message
echo): Wine passes every step (`create/connect/read/write`, all `gle=0`,
10-byte echo intact). The primitive is sound; Adobe's detector fails in the
safe direction (no sibling ⇒ proceed, no alert, session healthy).

**Next step.** None pre-login. Revisit only if a post-sign-in helper actually
fails to coordinate (then capture its pipe name via ProcMon-oracle comparison).

---

## 2026-09-13 — Installer COMPLETE (100%); CC app UI blocked on CEF shared images

**Installer: DONE.** User clicked through on `:0`: 100% → `LAUNCH_CCD` →
`POST_PRODUCT_INSTALL` → `TERMINATE_LBS`, zero alerts. Oracle timeline captured
(`C:\AdobeCap\cc.pml` 4GB + `cc-1.pml` 1.8GB + 23 timed screenshots): window
persists through 25% (ours vanishes = divergent, X-race below), Explorer blink
at 71%, 100% → CC Desktop with Welcome dialog (`artifacts/oracle-*.png`).

**Root causes pinned.**
- White window #1 (installer): DXVK 1.10.3 can't build any feature level here
  (Dozen Vulkan 1.2 + llvmpipe skipped) → DXVK 3.1 + `allowCpuDevices` + filter.
- Vanishing window: `X_UnmapWindow` on destroyed `0x400001`, 4× across 3
  programs (installer ×2, app ×2); sometimes fatal (Xlib default handler),
  sometimes survived. `+win` trace: 166 creates, 1 show, main hwnd paints into
  its surface, error lands after `ClipCursor` (async — sent earlier).
- `msedgewebview2.exe` + `Creative_Cloud_Set-Up.exe` still carry
  `Version=win8` (removal never stuck) — so gate/paint ran with the browser on
  the Win8 GPU path. WebView2 GPU int3-loop and CEF shared-image loop are BOTH
  Win10+-only: one bug shape, two symptoms.

**CC app: backend healthy, UI white.** Full stack alive (main, ADS,
IPCBroker×2, CoreSync, 5 UI helpers), manifests syncing, zero crashes later —
but CEF (Chrome/116) never paints: `CEF.log` =
`CreateSharedImage: could not create backing` → GPU exit 34 → crash loop →
`FATAL: GPU process isn't usable`. Oracle cmdlines captured
(`remote/oracle_ps.ps1`): CC launched with `--mode=CCDI --lbsWorkflowID=…
--showwindow=false --adsPrelaunched=true`; CEF children `--no-sandbox`.

**Ruled out (all evidenced).** NT-handle theory: my probe call was invalid —
oracle rejects it identically (`0x80070057` both). Plain-shared, keyed-mutex,
`AcquireSync` match Windows exactly. GL fallback (D3D disabled per-app):
worse (exit 1, network-service deaths) — reverted. wined3d instead of DXVK:
identical mailbox errors — NOT DXVK-specific. win8 for CEF children: no more
crash loop but still no mailboxes (silent backend, same white). DXVK debug log:
zero errors — failure is inside ANGLE/Chromium logic, no failing DXGI call.

**Standing fix list.** (1) x11drv Unmap-after-destroy race (reproducer +
patch). (2) Win10+-gated Chromium GPU path (differential win8-vs-11 ready).
(3) CEF116 shared-image backing with no failing DXGI call (needs ANGLE-side
error — no flag injection into Adobe's CEF; minimal-CEF repro or newer CEF?).
`tests/probes/probe_d3d11.cpp` now covers device/shared/keyedmutex/NT-attempt.
`remote/cef_*.reg`, `lin_cc{watch,dbg,vd,win}.sh`, `lin_glfb.sh`, `lin_cefw{8,3d}.sh`.

## 2026-09-13 (late) — X-race hypothesis narrowed to cross-thread Unmap vs Destroy

**Evidence.** `+win` trace of app startup (`/tmp/cc-wintrace.out`): 166 creates,
1 show, **zero `DestroyWindow` calls** — yet `X_UnmapWindow` hits a dead X id.
`+win,+x11drv` startup traces (`/tmp/cc-x11.out`, 2× ~1MB): error did NOT fire
under tracing (heisenbehavior — tracing slows the racy thread), X ids print as
`xwin whole/client` (`2600001/1a00018`), `destroy_whole_window` ×5 normal.
`tests/probes/probe_race.c` (4 threads × 1000 create/show/hide/destroy):
**clean** — same-thread teardown is ordered, not the bug.

**Hypothesis (code-located, unfixed).** `dlls/winex11.drv/window.c` withdraw
path (~line 1640) sends `XUnmapWindow(data->display, data->whole_window)` on a
NormalState→WithdrawnState transition, while `destroy_whole_window` (~2491)
`XDestroyWindow`s the same id — and Wine threads use **different Display
connections** (`thread_init_display`), so the two requests race server-side in
arrival order. Same-connection ordering would be legal; cross-connection is
not. The withdraw side appears to lack the destroyed-state check under the
lock that destroy holds. (Also noted: the `XSync` in destroy is on
`gdi_display`, not `data->display`.)

**Next session.** (1) Confirm by code inspection of the withdraw callers'
locking + add destroyed tracking (or skip-Unmap) under the win lock; rebuild
`winex11.drv` incrementally; verify with the app startup ×N. (2) If valgrind
is faster, run the app under it for UAF confirmation first. (3) CEF116
shared-image backing still unexplained at ANGLE level (no failing DXGI call
in DXVK debug log) — separate track from this race.

## 2026-09-14 — BREAKTHROUGH: CC Desktop app paints (spinner + update banner)

**Symptom.** App ran headless/hidden/white for hours; now shows a real window
with rendered UI (`artifacts/cc-vdtop.png`, `cc-init2.png`): titlebar
"Creative Cloud Desktop", hamburger menu, "Initializing Creative Cloud…"
spinner, blue "A new version of Creative Cloud is now available / Update"
banner, taskbar button.

**Run delta (`remote/lin_ccvdtop.sh`).** explorer.exe launched first
(`/desktop=Default,1280x960` — no desktop window appeared, but explorer runs),
then the app + second-launch nudge. Also in effect: corrected
`dxvk.nvapiHack=False` (was misspelled `dxgi.` — silently ignored before),
wined3d override reverted (DXVK 3.1 active), win8 kept for UI Helper.
Untracked which change mattered; shell-first is the prime suspect (tray/taskbar
integration gates the app's init path).

**Status.** Backend all green (NGL sync cycling, CoreSync transferring,
CDN flows active with queued bytes, Extensibility idle-healthy). UI thread
alive (spinner animates, one proc mildly active). Init has not completed after
~10 min — still downloading/waiting. Watching without input.

## 2026-09-14 — App renders logo splash; stage progression is slow, not stuck

**Evidence.** `artifacts/cc-restart.png`: clean-birth app shows the Adobe "A"
logo (rainbow gradient, perfectly rasterized) centered on white — GPU/raster
pipeline fully working. Prior instance had advanced splash → initializing
spinner + update banner (`cc-vdtop.png`, `cc-init2.png`). No input deadness
observed at splash stage; kill-respawn of a hot renderer changed nothing.

**Reading.** UI advances through real stages (splash → initializing → ?),
each taking minutes on llvmpipe. The "frozen" initializing may be slow
content sync, not a wedge (renderers idle-parked, spinner compositor-driven,
CDN flows active, NGL/CoreSync/IPC all cycling healthy).

## 2026-09-14 — PATCHED: ignore BadWindow on X_UnmapWindow (patches/0003)

**Symptom.** 4 silent deaths (installer ×2, CC app ×2): `X Error: BadWindow`
on `X_UnmapWindow` for an already-destroyed first window, delivered to Xlib's
default handler = instant process exit, no dialog.

**Root cause.** `ignore_error()` (`dlls/winex11.drv/x11drv_main.c`) swallows
BadWindow/BadMatch for SetInputFocus, ChangeWindowAttributes, ConfigureWindow
and SendEvent — but not UnmapWindow, and not on thread displays (only
gdi_display gets the create/destroy blanket). Wine threads use separate
Display connections, so a withdraw racing a destroy arrives out of order
server-side. Unmapping a dead window is a protocol no-op; Windows'
ShowWindow(HIDE) on one is equally silent.

**Patch.** One request added to the existing ignore group (+2 lines).
`tests/probes/probe_race.c` gained cross-thread feeder/reaper mode (`x`):
same-thread churn (4000 windows) and cross-thread Hide+Destroy (300, ±WM)
are clean — the trigger needs the real multi-window app pattern.

**Verification.** Incremental rebuild of `winex11.drv` only (top-level `make`
is broken by unrelated uncommitted `version.c` WIP — left alone); old
`winex11.so`/`.drv` backed up to `/tmp`. Sanity: Wine starts, probe clean.
Efficacy: 3 post-patch app launches, **0 X errors** (pre-patch rate ~2/3);
main-exe X windows now exist where none did. Stats weak (stochastic race) —
tally continues on every launch. Rollback: copy `/tmp/winex11.*.bak` back.

## 2026-09-14 — Overnight state: app paints, init pending on content prefetch

**Standing.** Installer 100% (user-verified). App backend green, splash logo +
spinner + update banner all render. Init has not completed; content prefetch
(GrowthSDK 12MB vs oracle 136MB, same account) trickles at ~KB/min while the
oracle filled in ~2 min on the same network. Per-request Wine WinINET cost is
modest (~0.5s vs 0.17s native, `tests/probes/probe_http.c`) — the gap is in
pacing/volume, not single-fetch speed. Prefetch holds 21 small DB files
(WAL churn), not thousands of tiny files.

**Correction (user).** A cache-seed from the oracle was staged and then
deleted unused — right call: the mission is fixing implementations, not
transplanting state. The pace gap is the divergence to chase, not bypass.
Oracle app relaunched after its snapshot (reference intact).

**Landed.** patches/0003 (Unmap/BadWindow ignore, verified 0/3 fatals),
`probe_http.c`, `probe_race.c` (+xthread), `probe_swapchain.cpp` (two-phase),
oracle ProcMon (6GB) + timeline shots + app cmdlines, DXVK 3.1 + config,
CEF diagnosis (shared-image backing, no failing DXGI call anywhere).
App runs left on for the night; morning check decides between patience
(prefetch completes) and the next instrumented push (request pacing).

## 2026-09-14 — Pace hunt: HTTP fine, locks slower, no contention proof

**Evidence.** `tests/probes/probe_lock.c`: 200 LockFileEx/UnlockFileEx rounds —
Wine 170µs/op vs oracle <0.5µs/op (wineserver roundtrips). Measurable but not
timeout-scale alone. `probe_http.c`: Wine WinINET ~0.5s vs native 0.17s per
fetch — same order, not the gap. GrowthSDK = 21 files (DB WAL churn), not a
file-count problem. `lsof`: ~24 GrowthSDK fds per Chromium process (pools,
not a leak); no multi-process contention observed on the WALs. Clocks agree
to 2s (no skew). No JS/resource errors in CEF.log.

**Standing open.** What paces the prefetch ~15,000× below oracle (per-request
costs ruled out; volume is small). Candidates left: lock *contention*
pathology under load (uncontended locks merely slow), server-side pacing of
this client, or an app-level gate misread as slowness. Next: contended-lock
probe (two processes), then RenderDoc ANGLE-usage capture if rendering
regresses; x11drv lock-side review for the withdraw path.

## 2026-09-14 — Input finding: KDE remote-control portal gates xdotool

**Evidence.** `artifacts/cc-burger.png`: KDE "Remote Control" dialog —
"An application is asking for special privileges: Control input devices".
Synthetic X input (xdotool) is intercepted by xdg-desktop-portal; physical
input is unaffected. Earlier installer typing worked (pre-policy or
pre-enforcement); current probes may have been silently swallowed. NO further
synthetic input until the user approves/denies at the console. Test scripts
using xdotool (`lin_input_probe.sh`, `lin_burger.sh`, sign-in stages) are
suspect for the current session — results predating the dialog stand, newer
clicks need re-validation after the decision.

## 2026-09-14 — Oracle timeline complete; init gate is content/applet loading

**Reference (oracle shots).** Sign-in → progress % + "Downloading and
installing" subtitle → role survey 1/2 (per-ACCOUNT — answered once on
Windows, correctly absent on kubuntu: not divergent) → "Opening Creative
Cloud…" spinner screen (ABSENT on ours — window died at the handoff; patch
0003 targets exactly this) → CC app + Welcome dialog.

**Ours.** `CreativeCloudSet-Up.exe` (Utils) opens the same app (no separate
repair UI). Init parked: `GSDKStore onRequestTimeout`, `ContentStore: CDN
fallback disabled`, `Unable to initialize core library`,
`RegisterObservers: No target applet found` — applets never load, UI waits.
Rendering/backend/auth/network all proven; the gate is content-fetch
timeouts, not pixels.

**Awaiting user (morning).** (1) Approve/Deny the KDE Remote Control prompt
(gates my input automation). (2) Click Update? (newer build may obsolete
this whole chase — but rewrites the install). (3) Attempt Photoshop install?
(needs working CC UI or HDBox-direct path).

## 2026-09-14 — Transports exonerated; two reversed divergences; warm-cache theory

**Proven sound (same PE both sides).** Named pipes sync (200/200), overlapped
(200/200 small + 20/20 × 1MB in 8s), WinINET fetch (~3× native), file locks
(170µs vs <0.5µs — slower, not broken), full DXGI matrix (device, shared all
formats, keyedmutex+acquire, RTV/SRV, swapchain hidden/shown + Present,
fences QI-match). No transport explains the app.

**Two reversed divergences (Wine accepts, Windows rejects).**
- Shared RGBA8/RGBA16F/RGB10A2/NV12 texture creation: S_OK vs E_INVALIDARG.
  Runtime use (clear/flush) works, so likely harmless — but a capability
  probe could steer ANGLE down a path Windows never takes. Suspect, unproven.
- Legacy GetSharedHandle on swapchain buffers: S_OK+NULL vs E_INVALIDARG.
  Same class. Chromium uses NT handles (matched), so probably unused.

**Theory for morning.** Every batch does slightly better than the last
(hidden → splash → spinner across runs/states): shader/font/SQLite/DNS
caches warm up, so warm restarts may eventually beat the fetch timeout that
cold runs always miss. Test: restart cycle with WAL trajectory per run.
Alternative remains the Update button (user decision).

## 2026-09-14 — Virtual desktop removed; stale-desktop fatal found + fixed by reset

**Incident.** After tearing down the VD experiment (explorer killed, Desktops
key deleted), fresh app launches died instantly: `X_CreateWindow` BadWindow on
`0x1200007` (the dead desktop id, 4×) — a NEW signature patch 0003 does not
cover (it handles Unmap only). Root cause: wineserver retains the desktop
mapping across explorer death; new windows parent to the corpse.
Fix: `wineserver -k` before relaunch (operational rule: killing explorer
requires a wineserver restart). App back to 10 procs, splash logo painting
directly on the Kubuntu desktop (`artifacts/cc-plain2.png`).

**Follow-up (not yet patched).** Stale-parent CreateWindow failure may deserve
its own ignore-or-fix; creating on a dead parent is arguably a real error
(the window cannot exist), unlike unmapping one (a no-op) — needs a decision,
not a hasty extension of 0003.
