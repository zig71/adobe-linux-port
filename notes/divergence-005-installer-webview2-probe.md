# Divergence #5 — the installer's WebView2 probe is the gate

Status: **root cause of the installer's halt, named and evidenced.** Fixing it
means implementing WebView2 in Wine, which is a large piece of work.

## The divergence, stated as a call and two results

The installer reads a feature flag `enableWebview2 : true`, then looks for the
Edge WebView2 runtime in the registry and chooses its UI backend accordingly.

```
RegOpenKeyExW( HKEY_LOCAL_MACHINE,
               L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\"
               L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}", 0, KEY_READ, &hk )
```

| | result |
|---|---|
| **Windows oracle** | `ERROR_SUCCESS`; key resolves into `WOW6432Node`, `pv = 153.0.4234.32` |
| **Wine** | `ERROR_FILE_NOT_FOUND (2)` |

The installer also probes the explicit `WOW6432Node` path and the
`KEY_WOW64_64KEY` view; all three fail on Wine.

## Evidence

Wine (`getRegistryValue` warnings in its own log — the API that fails differs
from Windows, which is what made this findable):

```
WARN | OSUtils | getRegistryValue: RegOpenKeyExW failed with error 2   ×4
```

The Windows oracle log contains **zero** `RegOpenKeyExW` failures — every key
open succeeds there. Its only registry warning is a missing *value*:

```
WARN | OSUtils | getRegistryValue: RegQueryValueExW failed with error 2
```

Pairing each `Call` with its `Ret` by return address, restricted to calls made
from the installer's own module, gives the exact failing keys
(`evidence/regkey-failures.txt`):

```
FAIL ret=006ae44a rv=00000002  L"Software\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
FAIL ret=006ae44a rv=00000002  L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
FAIL ret=006ae44a rv=00000002  L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
FAIL ret=006ae44a rv=00000002  L"Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
FAIL ret=006c2c89 rv=00000002  L"Software\\Adobe\\ALCID"                                   ×2
FAIL ret=007c49be rv=00000002  L"SOFTWARE\\Microsoft\\SQMClient"                           ×2
```

Three of the four key-open failures are the WebView2 runtime probe. On the
oracle that same key is present:

```
HKLM\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-...} pv=153.0.4234.32
HKLM\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-...}             MISSING (64-bit view)
```

A 32-bit process reading `HKLM\SOFTWARE\...` is redirected into `WOW6432Node`,
which is why the installer finds it on Windows and why the 64-bit view being
empty is harmless there.

## Why this explains everything observed

1. WebView2 is present on Windows, so the installer renders its UI in
   WebView2 and proceeds normally: it reaches `ViewMediatorUIWin |
   inside onWindowResize` and then `ACQUIRE_LOCKS` … `START_SIGNIN_WORKFLOW`.
2. On Wine the probe fails, so the installer takes its **IE WebBrowser
   fallback**. That fallback genuinely works — `probe_webbrowser.c` shows it
   reaching `READYSTATE_COMPLETE` with an identical activation-callback
   sequence on both platforms.
3. But on the fallback path the installer never shows its main window — the
   API trace shows the only `SetWindowPos` on the main window carries
   `SWP_HIDEWINDOW`, and `ShowWindow` is never called on it. It waits for its
   UI backend to report ready, which WebView2 does and the fallback does not,
   and the workflow therefore never starts.

This is consistent with every observation gathered: the main window never
shown, `onWindowResize` never logged, no workflow state reached, yet the
browser children created, shown and rendering.

## What was ruled out along the way

Both earlier hypotheses were tested and **refuted**, which is what narrowed the
search to the UI backend:

| hypothesis | verdict | evidence |
|---|---|---|
| Wine drops the window resize notification | **refuted** | `probe_window.c`: message counts and geometry identical on both platforms. Wine *does* send `WM_SIZE` (dlls/win32u/window.c:6133) |
| Wine never sends `WM_SHOWWINDOW` to the main window | **refuted as a Wine bug** | API trace: the installer never calls `ShowWindow` on it; the only `SetWindowPos` is `SWP_HIDEWINDOW`. Wine is not dropping a request that was never made |
| the embedded browser never becomes ready | **refuted** | `probe_webbrowser.c`: `READYSTATE_COMPLETE_is_4=1` on **both**, identical callback sequence, identical `QI` results (including `IViewObjectEx` = `E_NOINTERFACE` on both) |

## Secondary divergences found

These also fail on Wine and are worth reconciling, though they are not the gate:

- `HKCU\Software\Adobe\ALCID` — absent on both (the oracle does not have it
  either), so benign.
- `HKLM\SOFTWARE\Microsoft\SQMClient` — present on Windows, absent on Wine.
  Read for `MachineId`.
- `HKLM\Software\Policies\Microsoft\Windows\CurrentVersion\Internet Settings` —
  present on Windows, absent on Wine.

## Fix shape

The gate is Wine's complete lack of WebView2. Verified:

```
$ grep -ril 'webview2\|CoreWebView2' dlls/ include/ programs/   # zero matches
```

Nothing exists — not even a stub. What the installer needs, in order:

1. `WebView2Loader.dll` exporting `CreateCoreWebView2EnvironmentWithOptions`
   and `GetAvailableCoreWebView2BrowserVersionString`.
2. An `ICoreWebView2Environment` / `ICoreWebView2Controller` / `ICoreWebView2`
   implementation. The browser backend could be Wine's existing Gecko (as
   mshtml uses) or a bundled engine.
3. The `ICoreWebView2*` COM interfaces and their event handlers.

This is the same surface every WebView2-based Windows application needs, so it
is general work rather than an Adobe special case — it is simply large.

## Files

- Probes: `tests/probes/probe_webbrowser.c`, `tests/probes/probe_window.c`,
  `tests/probes/probe_metrics.c`
- Evidence: `evidence/regkey-failures.txt`, `evidence/cc-wine-browser-trace.txt`,
  `evidence/cc-install-oracle/WAM.log`
- Traces: `runs/20260912-011121-diff/`, `runs/20260912-012800-diff/`
