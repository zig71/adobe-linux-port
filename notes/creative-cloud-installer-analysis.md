# Creative Cloud installer — target analysis

Target: `Creative_Cloud_Set-Up.exe`
SHA256: `bc0c63263f7a40f6886fa528598309c150cc068b0dd54a9d46d25f145cc886f8`
Size: 3,314,952 bytes packed / 10,810,120 bytes unpacked
Identical on both VMs (verified by hash) — the differential method is valid.

## 1. What the binary is

| Property | Value |
|---|---|
| Format | `PE32 executable for MS Windows 5.01 (GUI), Intel i386` |
| Packer | UPX (30.7% ratio; `upx -d` restores 10,810,120 bytes) |
| Subsystem | Windows GUI (`IMAGE_SUBSYSTEM_WINDOWS_GUI`) |
| DllCharacteristics | `DYNAMIC_BASE`, `NX_COMPAT`, `TERMINAL_SERVICE_AWARE` |
| Imported DLLs (packed) | `KERNEL32.DLL`, `WS2_32.dll` |
| Linker | MSVC 14.33 |

Only two imported DLLs means everything else is resolved dynamically. The
unpacked image references these libraries by name:

```
ADVAPI32  atlthunk  bcrypt  COMCTL32  credui  CRYPT32  GDI32
IMSLib.dll  IPHLPAPI  KERNEL32  libcef.dll  msi  ole32  OLEAUT32
product.dll  PSAPI  Secur32  SHELL32  SHLWAPI  url.dll  USER32
VERSION  WINHTTP  WININET  WINTRUST  WS2_32  WTSAPI32
```

`IMSLib.dll` and `product.dll` are Adobe's own modules, fetched at runtime.

## 2. The UI is Microsoft Edge WebView2

The binary statically links the WebView2 loader and hosts it with Microsoft's
WRL. Symbols present:

```
ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
ICoreWebView2ExecuteScriptCompletedHandler
ICoreWebView2NavigationCompletedEventHandler
ICoreWebView2NavigationStartingEventHandler
ICoreWebView2NewWindowRequestedEventHandler
ICoreWebView2WebMessageReceivedEventHandler
ICoreWebView2BasicAuthenticationRequestedEventHandler
```

The UI itself is a bundled React application (Spectral CSS, `normalize.css`,
`AdobeMessagingClient.js`). `libcef.dll` appears in the string table but the
observed runtime is Edge WebView2, not CEF.

`WAM.log` records the feature flag `enableWebview2 : true`.

## 3. Oracle behaviour (Windows 11 26200.9445)

Launched in the interactive session (session 1). Observed:

- Installer process starts, then spawns **12 → 24 `msedgewebview2.exe`**
  processes from `C:\Program Files (x86)\Microsoft\EdgeWebView\Application\153.0.4234.32`.
- Creates WebView2 user-data directories
  `%LOCALAPPDATA%\Adobe\webview2\Creative_Cloud_Set-Up.exe\EBWebView\` and
  `%TEMP%\{4964307E-C020-4BBF-8FE6-A15AC36C4F0F}\EBWebView\`.
- Creates `%LOCALAPPDATA%\Adobe\{licflags,NGL,OOBE,webview2}`.
- Window created at 1016×669 (`WAM.log`: "possible snapped window case").
- Talks to `cc-api-data.adobe.io:443` (HTTP 200) and many Adobe endpoints over TLS.

Workflow states from `WAM.log` (`runs/cc-install-oracle/WAM.log`):

```
ACQUIRE_LOCKS                  (bootstrapper lock, LBS lock)
CHECK_GENERAL_SYSTEM_REQUIREMENTS
CHECK_FOR_PROXY                (GetIEProxyInfo / WPAD)
CHECK_FOR_NETWORK
CHECK_ALREADY_INSTALLED_PRODUCT
SHOW_WELCOME_SCREEN
START_SIGNIN_WORKFLOW          -> getSUSIUrl -> SUSI
```

Sign-in state at that point: `isDeviceTokenPresent, DT not found`,
`getUserGuid, Failed to get UserGuid` — i.e. it reached the sign-in screen and
is waiting for credentials. Installer version `2.14.0.82`; session SDK
`dunamis 1.41.0+20240103`.

The installer therefore **works**, up to the point where a human signs in. That
is the reference this lab must reproduce.

## 4. WebView2 discovery (measured, both views)

`tests/probes/probe_webview2.c` (built 32-bit, matching the installer) against
the oracle:

```
PROBE_BITS=32
HKLM_default_open=found (0)
HKLM_default_pv=153.0.4234.32
HKLM_default_location=C:\Program Files (x86)\Microsoft\EdgeWebView\Application
HKLM_default_name=Microsoft Edge WebView2 Runtime
HKLM_64view_open=missing (2)
HKLM_32view_open=found (0)
HKLM_32view_pv=153.0.4234.32
RUNTIME_DIR0_first_version=153.0.4234.32
RUNTIME_DIR0_version_dirs=2
msedgewebview2_exe=present
EmbeddedBrowserWebView_x86=present
EmbeddedBrowserWebView_x64=present
```

The runtime registers under the **32-bit view only**:
`HKLM\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}`,
value `pv` = the version. The 64-bit view has no such key — a 32-bit process
reading `HKLM\SOFTWARE\...` is redirected into `WOW6432Node`, which is why the
installer (32-bit) finds it and a 64-bit probe does not.

## 5. Wine status

| Requirement | Wine status |
|---|---|
| 32-bit PE execution (WoW64) | **absent** in the current build — the installer fails with `c0000135` on `syswow64\ntdll.dll`. Full-arch build in progress. |
| Edge WebView2 runtime | **completely absent** |
| `WebView2Loader.dll` | not provided |
| `ICoreWebView2*` COM interfaces | not implemented |
| `EdgeUpdate\Clients\{F3017226-…}` key | absent |

Verified: `grep -ri webview2 dlls/ include/` over the upstream Wine tree returns
**zero** matches. Wine has no WebView2 support of any kind, not even a stub.

## 6. Consequence

The installer's only UI is WebView2. Without a WebView2 implementation the
installer can start and run its pre-UI work but can never render a window, so it
cannot reach even `SHOW_WELCOME_SCREEN` — let alone the sign-in screen the
oracle reaches.

There is no feature-flag escape: `NglFeatureConfig` is a DPAPI-protected blob
(`AQAAANCMnd8BFdERjHoAwE/...`), i.e. machine/user-bound configuration that is
neither readable nor appropriate to tamper with.

## 7. Options

1. **Implement WebView2 support in Wine** (the generative fix, upstreamable).
   Provide `WebView2Loader.dll` + the `ICoreWebView2*` surface backed by an
   open-source engine, so `CreateCoreWebView2EnvironmentWithOptions` succeeds and
   a real window renders. Large, but it is the same code path every
   WebView2-based Windows application needs — Creative Cloud, many installers,
   and Microsoft's own tooling.
2. **Run the genuine Edge WebView2 runtime under Wine.** The runtime is a
   Chromium build requiring AppContainer sandboxing and a wide API surface; it is
   also not redistributable, and the installer would have to install it first.
3. **Headless install path.** If Adobe documents an unattended mode, the UI stops
   being a gate. No such switch is reachable from the static strings; it would
   need further investigation before it can be ruled in or out.

Option 1 is the one that matches the lab's purpose: it removes a real, general
Windows/Wine divergence rather than routing around it.
