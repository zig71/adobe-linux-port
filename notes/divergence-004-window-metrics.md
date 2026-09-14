# Divergence #4 — window sizing metrics (fixed) and the installer's remaining stall

Status: metric bug **fixed and verified**; it was **not** the installer's blocker.
My first hypothesis for this divergence was **wrong** and is recorded below.

## Hypothesis 1 (WRONG): "Wine never sends WM_SIZE to the installer's window"

I inferred from the installer's logs that `ViewMediatorUIWin`'s `onWindowResize`
never fires under Wine, and guessed Wine was not delivering the resize
notification. **Refuted by measurement.**

`tests/probes/probe_window.c` creates a top-level window, counts the messages it
receives, and reports its geometry. Same binary on both platforms:

| counter | Windows | Wine |
|---|---|---|
| `AFTER_CREATE_create` | 1 | 1 |
| `AFTER_CREATE_nccalcsize` | 1 | 1 |
| `AFTER_CREATE_getminmaxinfo` | 1 | 1 |
| `AFTER_CREATE_size` | 0 | 0 |
| `AFTER_SHOW_size` | 1 | 1 |
| `AFTER_SHOW_windowposchanged` | 1 | 1 |
| `AFTER_SHOW_showwindow` | 1 | 1 |
| `AFTER_SHOW_move` | 1 | 1 |
| `AFTER_SHOW_activate` | 1 | 1 |
| `AFTER_PUMP_size` | 1 | 1 |

Window geometry was identical too (`2,24,1022,689`, size `1020x665`, client
`1020x665`). Message delivery is **not** the problem. Evidence:
`runs/20260912-011121-diff/`.

## Hypothesis 2 (CONFIRMED): system metrics for maximized/fullscreen windows

`tests/probes/probe_metrics.c` dumps the metric catalogue. The decisive rows:

| metric | Windows | Wine (before fix) | Wine (after fix) |
|---|---|---|---|
| `SM_CXSCREEN` / `SM_CYSCREEN` | 1024 / 768 | 1024 / 768 | 1024 / 768 |
| `SM_CYCAPTION` | 23 | 26 | 26 |
| `SM_CXSIZEFRAME` | 8 | 4 | 4 |
| **`SM_CYFULLSCREEN`** | **745** | **786** | **742** |
| **`SM_CYMAXIMIZED`** | **784** | **820** | **776** |
| `fullscreen − screen` | **−23** | **+18** | **−26** |

**Wine returned `SM_CYFULLSCREEN = 786` for a 768-high screen.** A full-screen
window's client area cannot be taller than the display; the value was
self-inconsistent regardless of any comparison with Windows.

### Root cause

`dlls/win32u/sysparams.c` derived the pair inconsistently:

```c
case SM_CXMAXIMIZED:  return SM_CXSCREEN + 2 * SM_CXFRAME;     /* symmetric  */
case SM_CYMAXIMIZED:  return SM_CYSCREEN + 2 * SM_CYCAPTION;   /* wrong term */
case SM_CXFULLSCREEN: return SM_CXMAXIMIZED - 2 * SM_CXFRAME;  /* == screen  */
case SM_CYFULLSCREEN: return SM_CYMAXIMIZED - SM_CYMIN;        /* > screen   */
```

The Y case used `SM_CYCAPTION` where the X case used `SM_CXFRAME` — an
asymmetry with no justification, and the comment ("at least this formulation is
correct") shows the author was unsure.

Windows' own numbers give the formula exactly:

- `SM_CYMAXIMIZED = SM_CYSCREEN + 2 * SM_CYFRAME` → `768 + 2(8) = 784` ✓
- `SM_CYFULLSCREEN = SM_CYSCREEN - SM_CYCAPTION` → `768 - 23 = 745` ✓

### Fix

`patches/0003-win32u-System-metrics-for-maximized-and-fullscreen-windows.patch`

```c
case SM_CXMAXIMIZED:  return SM_CXSCREEN + 2 * SM_CXFRAME;
case SM_CYMAXIMIZED:  return SM_CYSCREEN + 2 * SM_CYFRAME;      /* was CAPTION */
case SM_CXFULLSCREEN: return SM_CXSCREEN;
case SM_CYFULLSCREEN: return SM_CYSCREEN - SM_CYCAPTION;        /* was MAXIMIZED - MIN */
```

Verified against the oracle: after the fix the invariants hold on both
platforms — maximized = screen + 2×frame, fullscreen = screen − caption, and
`fullscreen − screen` is negative as it must be. Evidence:
`runs/20260912-011529-diff/`.

## What did NOT happen

**This fix did not unblock the installer.** Re-running it still halts at the
same point: after `Application initialized successfully`, its host-directory
creation and the `getRegistryValue` warning, it never logs `onWindowResize` and
never enters `ACQUIRE_LOCKS` or any later workflow state. The metric divergence
was real and worth fixing on its own merits, but it is not the gate.

## Remaining differences and next hypotheses

The metric catalogue still diverges in 26 places. The load-bearing ones are
non-client values, which on this comparison are partly a theme artifact —
Windows was measured over SSH in session 0 (classic metrics, no interactive
desktop), while Wine renders with its own defaults:

| metric | Windows (session 0) | Wine |
|---|---|---|
| `SM_CXSIZEFRAME` / `SM_CYSIZEFRAME` | 8 | 4 |
| `SM_CYCAPTION` | 23 | 26 |
| `SM_CXSMSIZE` | 22 | 17 |
| `WORKAREA` | 1024x768 | 1024x714 |

These must be re-measured in the **interactive** Windows session before being
treated as divergences, since session 0 does not use the themed metrics a real
desktop does.

Next hypotheses for the installer's halt, in order:

1. The installer's window is created on a thread whose message loop differs, or
   the window is created but a subsequent `SetWindowPos`/`ShowWindow` it depends
   on is not issued — needs a trace scoped to the installer's own window handle.
2. The halt is inside the embedded browser: the trace showed Gecko loading
   `html.css`/`counterstyles.css`, so the page is rendering, but the installer
   may be waiting on a page-load or script-completion callback that never
   arrives.
3. The `getRegistryValue: RegOpenKeyExW failed with error 2` immediately before
   the stall — four occurrences, on keys the oracle also fails to find, so
   probably benign, but unverified.

Note that Wine's message channel does not trace window messages
(`+msg` only logs internal calls such as `BroadcastSystemMessageExW`), so
`probe_window`-style counting is the reliable way to test message delivery.

## Files

- Probes: `tests/probes/probe_window.c`, `tests/probes/probe_metrics.c`
- Evidence: `runs/20260912-011121-diff/` (messages), `runs/20260912-011358-diff/`
  (metrics before), `runs/20260912-011529-diff/` (metrics after)
- Patch: `patches/0003-win32u-System-metrics-for-maximized-and-fullscreen-windows.patch`
