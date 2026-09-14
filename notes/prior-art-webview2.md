# Prior art: Vinegar, Kombucha, and what Wine itself already has

Researched in response to "Vinegar deals with the EdgeWebView stuff, we can
probably lift stuff from there". The premise needed correcting, but the trail
led somewhere much better.

## Correction 1: Vinegar is not a Wine fork

| project | what it is | licence |
|---|---|---|
| [vinegarhq/vinegar](https://github.com/vinegarhq/vinegar) | Go bootstrapper/manager for Roblox Studio on Linux | **GPL-3.0** |
| [vinegarhq/kombucha](https://github.com/vinegarhq/kombucha) | the actual customized **Wine** build Vinegar uses | **LGPL-2.1** |

Vinegar is the front end; **Kombucha** is the Wine fork.

## Correction 2: Kombucha has no WebView2 patches

All 24 of Kombucha's patches, by subject:

```
stable/   0001 winex11 Xfixes query            0008 Revert winex11 ConfigureNotify
          0002 winex11 hide cursor on warp     0009 server persistence
          0003 winex11 Xwayland check          0010 explorer virtual desktop
          0004 wine.inf Kombucha overrides     0011 server ntsync notify
          0005 wineboot WINEBOOT_HIDE_DIALOG   0012 ntdll memory redirect (mimalloc)
          0006 win32u bitmap-only TTF fonts    0013 ntdll ucrtbase search
          0007 ntdll LFH block groups          0014 win32u/server desktop close timeout
proton-unstable/ 0001-0010  ProtonOverrides, KombuchaOverrides, ConfigureNotify,
                            virtual desktop, mimalloc, steamuser, fonts,
                            persistence, window decorations, EGL
```

No WebView2, no DirectComposition, nothing browser-related. **There is nothing to
lift for this problem.** The README confirms the scope: "a bug related to cursor
locking on Wayland-based desktops, among other things."

## Correction 3: Vinegar's WebView2 approach is the one already tested here — and it is known not to work well

Vinegar does not implement WebView2. It **downloads Microsoft's own WebView2
runtime installer** and runs it in the prefix
(`cmd/vinegar/bootstrapper_setup_pfx.go`):

```go
d, err := webview2.Stable.Runtime(b.cfg.Studio.WebView, "x64")
...
return webview2.Install(b.pfx, inst)
```

That is the same class of approach as staging the runtime by hand. And Vinegar's
own troubleshooting page lists the outcome as a **known issue**:

> Microsoft Edge WebView2 does not always render correctly under Wine. A black,
> white, or unresponsive login window is a known Vinegar issue.

with the workaround being to disable web pages and log in through the browser
instead. **This independently corroborates the finding in
`notes/webview2-portability-test.md`** — installing the genuine runtime does not
make WebView2 work under Wine.

## What *is* worth lifting: Wine's own MR 7032

[MR 7032](https://gitlab.winehq.org/wine/wine/-/merge_requests/7032) — *"Add
embeddedbrowserwebview aka. WebView2 DLL stub."* by Bernhard Kölbl (CodeWeavers),
Dec 2024, branch `besentv/wip/edge-webview`. **Not merged** (closed after the
code freeze) and **absent from the current tree**, but it is Wine's own code
under LGPL-2.1 — clean to adopt.

It contains exactly the interface surface we would otherwise have to write:

- `dlls/embeddedbrowserwebview/` — stub DLL whose 8 exports **match the real
  `EmbeddedBrowserWebView.dll` exactly**. Verified against the runtime shipped
  from the oracle:

  | ordinal | export | real DLL | MR 7032 |
  |---|---|---|---|
  | 6 | `CreateWebViewEnvironmentWithOptionsInternal` | present | present |
  | 7 | `DllCanUnloadNow` | present | present |
  | 8 | `GetHandleVerifier` | present | present |
  | 1-5 | `IDataFieldVisitor@telemetry_client` mangled names | present | present |

- **`include/webview2.idl` — 953 lines** declaring `ICoreWebView2`,
  `ICoreWebView2Controller`, `ICoreWebView2Environment`, settings, navigation
  and web-resource interfaces and their event handlers. This is the single
  largest piece of the work and it already exists.
- `wine.inf` fake-DLL registration under
  `Microsoft\EdgeWebView\Application\<ver>\EBWebView\x32|x64`, matching the
  layout the real runtime uses.
- An `.rgs` writing `HKLM\Software\Wow6432Node\Microsoft\EdgeUpdate\ClientState\{F3017226-...}\EBWebView`.

The author's stated intent is the same architecture proposed here:

> I think we want to have this DLL living in Wine for easier development, and
> probably dynamically load our custom Chromium fork from here. (just like
> MSHTML and wine-Gecko). The code for that fork could then be created in its
> own repo.

## The actual blocker, now reproduced: DirectComposition

Upstream bug **[58921](https://bugs.winehq.org/show_bug.cgi?id=58921)** —
*"WebView2 does not work with Windows version setting 8.1 or newer"* — attributes
the failure to DirectComposition:

```
DCompositionCreateDevice failed: Not implemented (0x80004001)
```

Bug **59370** (blank WebView2 windows, Chromium's D3D11 shared-texture
compositor path) was closed as a duplicate of it.

`tests/probes/probe_dcomp.c` reproduces this on the current build:

| call | Windows oracle | Wine 11.17 |
|---|---|---|
| `DCompositionCreateDevice` | `0x80004002` **E_NOINTERFACE** (real implementation, rejecting the IID) | `0x80004001` **E_NOTIMPL** |
| `DCompositionCreateDevice2` | `0x80004002` | `0x80004001` |
| `DCompositionCreateDevice3` | `0x80004002` | `0x80004001` |
| `DCompositionCreateSurfaceHandle` | `0x00000000` **S_OK**, real handle | **faults in `__wine_spec_unimplemented_stub`** |

And the source confirms it is a skeleton:

```
dlls/dcomp/device.c    47 lines, every function a FIXME returning E_NOTIMPL
dlls/dcomp/dcomp.spec  @ stub for DCompositionCreateSurfaceHandle and ~15 others
```

## Also tested: the documented `win8` workaround

Bug 58921 suggests forcing the browser process to report Windows 8, which makes
Chromium avoid the DirectComposition path:

```
[HKEY_CURRENT_USER\Software\Wine\AppDefaults\msedgewebview2.exe]
"Version"="win8"
```

Applied and confirmed present in the registry, then the installer re-run.

**It does not help.** No workflow state progressed, no `onWindowResize`, no
WebView2 user-data directory, and no `msedgewebview2` process. The reason is
consistent with the earlier trace: `msedgewebview2.exe` never launches at all,
because the 32-bit in-process host faults first. The workaround addresses the
browser process, which this application never reaches.

## What this changes

The target is **not** "implement WebView2" as a monolith. It decomposes into
three pieces of very different size, and the first is both the documented
blocker and independently testable:

1. **Implement `dlls/dcomp`.** Currently a 47-line stub. This is the named cause
   of WebView2 failing under Wine, and it is a general gap — any
   DirectComposition application needs it, not just WebView2. Highest value,
   most upstreamable, and `probe_dcomp.c` gives it a pass/fail test today.
2. **Adopt MR 7032's `embeddedbrowserwebview` stub and `webview2.idl`** (LGPL,
   Wine's own) for the interface surface and DLL registration.
3. **Back it with an engine** — Wine's existing Gecko, which mshtml already
   drives to `READYSTATE_COMPLETE`, or as MR 7032's author plans, a Chromium
   fork in its own repo.

## Licence note

Worth stating plainly for the "commit to wine staging" goal:

- **Vinegar is GPL-3.0.** GPL-3.0 code cannot be incorporated into Wine
  (LGPL-2.1-or-later). Even if it had a WebView2 implementation worth taking, it
  could not be merged.
- **Kombucha is LGPL-2.1** — compatible with Wine. But it has nothing relevant.
- **MR 7032 is Wine's own**, LGPL-2.1. Clean.

## Sources

- https://github.com/vinegarhq/vinegar (GPL-3.0, Go bootstrapper)
- https://github.com/vinegarhq/kombucha (LGPL-2.1, the Wine fork; 24 patches, none WebView2)
- https://vinegarhq.org/Vinegar/Troubleshooting.html (WebView2 black-window known issue)
- https://gitlab.winehq.org/wine/wine/-/merge_requests/7032 (WebView2 DLL stub + webview2.idl)
- Wine bug 58921 (DirectComposition; E_NOTIMPL) and bug 59370 (blank windows, duplicate)
- Wine bug 56378 (Edge/WebView2 `--no-sandbox`, resolved Jan 2026)
