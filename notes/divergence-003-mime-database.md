# Divergence #3 — MIME type database and the installer's browser stack

Status: **CLOSED — resolved; my initial root cause was wrong and is corrected below.**

## What I first claimed, and why it was wrong

I initially concluded that `loader/wine.inf.in` lacks the MIME content-type
database (`grep -c MIME loader/wine.inf.in` → `0`) and that this blocked the
installer. **That conclusion was wrong**, and the way it was wrong matters.

`probe_mime` reported the `MIME\Database\Content Type\text/html` key as missing.
But every Wine command I ran carried:

```
WINEDLLOVERRIDES='mscoree,mshtml='
```

That override **disables mshtml** — the very module that registers the MIME
database and the very CLSID that handles `text/html`. The override was inherited
from an early WebView2 test where mshtml needed suppressing, and it silently
masked the behaviour under investigation.

Measured properly:

| configuration | `HKCR\MIME\Database\Content Type` |
|---|---|
| Wine + `mshtml=` override | **empty** (0 keys) |
| Wine, **no override** (fresh prefix) | **33 content types**, `text/html` → `{25336920-…}` |

The registration mechanism was always present and working:
`dlls/mshtml/mshtml.inf` has a `[FileAssoc.Reg]` section with
`HKCR,"MIME\Database\Content Type\text/html","CLSID",,"%CLSID_HTMLDocument%"`
(and `text/plain`, `image/gif`, `image/jpeg`, `image/x-icon`, `text/xml`,
`application/xml`, `image/svg+xml`, `application/xhtml+xml`, …). It is applied
by `RegInstall()` from `DllRegisterServer` (`dlls/mshtml/main.c:750`), and
`wine.inf` invokes it via `[RegisterDllsSection]` → `11,,mshtml.dll,1`.

**Lesson recorded:** a debug override that disables a module changes what the
system does. Never measure registration or behaviour of a module while that
module is overridden. The harness no longer passes `mshtml=` (see
`scripts/run_diff.mjs`).

## The real dependency: Wine Gecko

Once mshtml was enabled, `CoCreateInstance` on the `text/html` handler **hung
indefinitely**. Cause: Wine's `mshtml` needs the Gecko add-on
(`dlls/mshtml/nsiface.idl` → `GECKO_VERSION "2.47.4"`), and the prefix had only
an empty `gecko/plugin/` directory. With no engine,
`install_wine_gecko()` raises a download prompt that blocks forever when there
is nobody to answer it — presenting as a hang rather than an error.

Installed `wine-gecko-2.47.4-{x86,x86_64}.msi` from `dl.winehq.org`. After
that:

| probe key | Windows | Wine |
|---|---|---|
| `MIME_text_html_open` | `found (0)` | `found (0)` |
| `MIME_text_html_clsid` | `{25336920-03F9-11CF-8FD0-00AA00686F13}` | `{25336920-03F9-11CF-8FD0-00AA00686F13}` |
| `MIME_text_html_parse` | `0x00000000` | `0x00000000` |
| `MIME_text_html_cocreate` | `0x00000000` | **`0x00000000`** |

`probe_mime` divergences: **22 → 5**.

The 5 remaining are real but not blocking:

| remaining divergence | Windows | Wine |
|---|---|---|
| `MIME_DB_subkeys` | 148 | 33 |
| `application/xhtml+xml` CLSID case | lowercase | uppercase |
| `application/xhtml+xml` `CoCreateInstance` | `S_OK` | `0x80040111` (CLASS_E_CLASSNOTAVAILABLE) |
| `text/html` CLSID case | lowercase | uppercase |
| `text/plain` CLSID case | lowercase | uppercase |

The CLSID case differences are cosmetic (Windows writes lowercase hex, Wine
uppercase). The real gaps are the 115 content types Wine does not register
(mostly `video/*` and `audio/*` — WMP/Media Foundation handlers Wine has no
implementation for) and `application/xhtml+xml` not being creatable.

## Result: the installer's browser stack now works

With WoW64, Gecko and the MIME database in place, the installer's IE fallback
binds successfully. Trace (`WINEDEBUG=+ieframe,+urlmon,+mshtml`):

```
ieframe:DllGetClassObject (CLSID_WebBrowser ...)
ieframe:create_webbrowser ... version=2
ieframe:create_shell_embedding_hwnd parent=00040084 hwnd=0003007C
ieframe:WebBrowser_put_RegisterAsBrowser
urlmon:CreateURLMonikerEx (L"file:///C:/users/kubuntu/AppData/Local/Temp/{8D87B4E8-…}/index.html")
ieframe:set_status_text => L"Start downloading .../index.html"
mshtml:nsIOServiceHook_NewURI ("jar:file:///C:/windows/syswow64/gecko/2.47.4/wine_gecko/omni.ja!/chrome/toolkit/res/html.css" ...)
mshtml:nsIOServiceHook_NewURI ("jar:.../counterstyles.css" ...)
```

A `WebBrowser` is created, a shell-embedding window is created, the local page
navigates and completes, and Gecko loads and parses its HTML/CSS. Compared with
the previous run, the failures

```
fixme:urlmon:create_object Could not find object for MIME L"text/html"
fixme:ieframe:bind_to_object BindToObject failed: 80040154
```

are **gone**.

## Remaining limitation

The installer still does not finish its workflow. Wine's `WAM.log` reaches
"Application initialized successfully", creates its host directory
`%TEMP%/{GUID}/`, and completes two `POST https://cc-api-data.adobe.io/ingest`
calls with HTTP 200 — but never logs the workflow states the oracle logs
(`ACQUIRE_LOCKS`, `CHECK_*`, `SHOW_WELCOME_SCREEN`, `START_SIGNIN_WORKFLOW`).

Timing is a strong suspect: the same HTTP sequence took **2 s** on Windows and
**31 s** under Wine, consistent with software rendering (llvmpipe, 1024×768,
Wayland/Xwayland). Whether the workflow eventually proceeds or is genuinely
stuck needs a longer scheduled run with the window actually mapped and visible;
X11 screen capture on this Wayland session returns only a black root window, so
screenshots are not a usable check here.

## Files

- Probe: `tests/probes/probe_mime.c`, `tests/probes/probe_webview2.c`
- Evidence: `runs/20260912-001415-diff/` (pre-Gecko), `runs/20260912-004707-diff/` (post-Gecko)
- Oracle MIME dump: `evidence/windows-mime-database.txt`
- Installer workflow log: `evidence/cc-install-oracle/WAM.log`
