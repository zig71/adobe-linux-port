# Divergence #4 — installer stalls waiting for its UI window's resize event

Status: **isolated to a precise point; root cause not yet fixed.**

## The stall, stated exactly

Wine's `WAM.log` stops after `Application initialized successfully` and never
reaches the workflow states the oracle reaches. It is **not** slow: the log held
at 58 lines for 4+ minutes with 3 processes alive. The trace is complete and
reproducible.

Side by side, from the same point in a run:

| step | Windows oracle | Wine |
|---|---|---|
| `CreateDirectoryAtPath` (Temp/{GUID}) | ✓ | ✓ |
| `getRegistryValue: RegOpenKeyExW failed with error 2` | ✓ | ✓ |
| **`ViewMediatorUIWin \| inside onWindowResize, possible snapped window case, width 1016 height 669`** | **✓ (t+20 ms)** | **✗ never** |
| `ACQUIRE_LOCKS` → `CHECK_*` → `SHOW_WELCOME_SCREEN` → `START_SIGNIN_WORKFLOW` | ✓ (t+2 s) | ✗ never |
| `POST cc-api-data.adobe.io/ingest` → HTTP 200 | ✓ | ✓ (matches) |

The last thing Wine logs before going silent is the ingest HTTP 200. After that
the process sits in its message loop.

Earlier in the same run Wine *did* build the browser host:

```
ieframe:create_webbrowser ... version=2
ieframe:create_shell_embedding_hwnd parent=00040084 hwnd=0003007C
ieframe:...NavigateComplete2
mshtml:nsIOServiceHook_NewURI ("jar:.../gecko/2.47.4/wine_gecko/omni.ja!/chrome/toolkit/res/html.css")
```

So the window object exists and Gecko renders. What is missing is the
**resize/creation notification** that the installer's `ViewMediatorUIWin` uses to
move from "initialized" into its workflow.

## Why this is the right characterisation

`onWindowResize` is a Win32 window-message handler — the application reacts to
`WM_SIZE` (and its own `WM_WINDOWPOSCHANGED`/visibility notifications) by
laying out its WebBrowser child and then starting the install workflow. If the
host window is created but never resized or shown, the handler never runs and
the app waits indefinitely.

Candidate causes, in order of likelihood:

1. The window is created without an explicit size and Wine never synthesises the
   resize notification Windows does.
2. The window is created hidden and `ShowWindow`/`SetWindowPos` never produces a
   `WM_SIZE`, so the app's layout callback is never invoked.
3. The app sizes the window through a path Wine does not implement
   (e.g. `WM_GETMINMAXINFO` handling, per-monitor DPI sizing, or
   `SetWindowPos` with flags Wine treats as a no-op).

Distinguishing these needs the message stream for the installer's own window,
which is the next experiment.

## Confirming experiment

Run Wine with `WINEDEBUG=+msg` restricted to the installer's window handle and
compare the sequence against the oracle's, specifically for:

- `WM_CREATE`, `WM_SIZE`, `WM_WINDOWPOSCHANGED`, `WM_SHOWWINDOW`, `WM_PAINT`
- `GetWindowRect`/`GetClientRect` return values at that point
- whether `ShowWindow(SW_SHOW)` is issued at all

A minimal standalone reproducer is straightforward: create a window the way the
installer does, host nothing in it, and log which of those messages Windows
sends and Wine does not. That converts this into a small, testable divergence
rather than an application-level observation.

## Context

This is downstream of the fixes that made the installer run at all:

- WoW64 (`--enable-archs=i386,x86_64`) — without it the installer dies with
  `c0000135` on `syswow64\ntdll.dll`
- Wine Gecko 2.47.4 — without it `CoCreateInstance` on the `text/html` handler
  hangs forever on an unanswerable download prompt
- MIME content-type database — provided by `mshtml.inf`, masked during early
  testing by a `mshtml=` DLL override in the probe harness

With those in place the browser stack works and the installer gets much further
than before; this windowing step is now the single thing standing between it and
its welcome screen.
