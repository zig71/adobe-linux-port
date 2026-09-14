# Debugging kit

What is available on each guest for this work, what was missing, and what was
added. Everything below is verified by running `command -v` / `Get-Command` on
the guests, not assumed.

## Linux target (Kubuntu-26.04)

| category | tools |
|---|---|
| Debuggers | `gdb`, `lldb`, `winedbg` |
| Tracers | `strace`, `ltrace`, `perf`, `bpftrace`, `valgrind`, `trace-cmd` |
| Crash analysis | `coredumpctl`, `eu-stack`, `addr2line`, `objdump`, `readelf`, `nm` |
| Wine | `winedbg`, `winedump`, `winebuild`, `wmc`, `wrc`, `winegcc`, `wineg++`, `winemaker`, `wineserver`, `winetricks`, `winepath` |
| Graphics | `vulkaninfo`, `glxinfo`, `clinfo`, Mesa Dozen ICD |
| Network | `tcpdump`, `socat`, `nc` |
| Windows UI | `xdotool`, `wmctrl`, `xwd`, ImageMagick (`import`, `convert`) |
| Other | `ffmpeg`, `upx`, `strings`, `python3`, `jq`, `inotify-tools` |

Added during this work: `xdotool`, `wmctrl`, `x11-apps`, `imagemagick`,
`inotify-tools`, `upx-ucl`, and the Dozen Vulkan driver.

Kernel knobs relaxed from defaults that blocked profiling:

```
kernel.perf_event_paranoid = 4 -> 1     # perf/bpftrace previously denied unprivileged
kernel.yama.ptrace_scope   = 1 -> 0     # gdb/ltrace attach to a non-child previously denied
```

Still absent: `renderdoc`, `wireshark`/`tshark`, `mitmproxy`, `sysdig`,
`fatrace`.

### Wine debug channels that matter here

- **`+message`** drives the message spy (`dlls/win32u/spy.c`: `spy_init()`
  returns FALSE unless `TRACE_ON(message)`). This prints every
  `WM_*` with hwnd, window class, wParam and lParam, and is the only way to see
  messages.
  **`+msg` does not trace messages** — it logs internal calls such as
  `BroadcastSystemMessageExW` only. Using `+msg` cost a full trace cycle.
- `+reg` — registry, but dominated by crypto OID lookups; for a specific key use
  `+relay` filtered to `RegOpenKeyExW`.
- `+relay` — every cross-module call. Enormous; always filter live with `grep`.
  Pair `Call`/`Ret` by return address to attribute a call to a module.
- `+win` — window creation (`WIN_CreateWindowEx`, `NtUserCreateWindowEx`).
- `+ieframe`, `+urlmon`, `+mshtml` — the embedded browser stack.
- Wine's registry `HKCU\Software\Wine\Debug` values `SpyInclude` / `SpyExclude`
  narrow the message spy if `+message` is too noisy.

### Screenshots are unreliable here

The desktop is Wayland with Xwayland. `ffmpeg -f x11grab` (and `xwd`, `import`)
return only the black root window, so visual confirmation of a Wine window is
not currently possible this way. `probe_window.c` style instrumentation is the
reliable substitute: report geometry, visibility and message counts from inside
the process.

## Windows oracle (Win11-25H2)

| category | tools |
|---|---|
| Sysinternals | `Procmon`, `procdump`, `procexp`, `vmmap`, `handle`, `ListDlls`, `Autoruns`, `sigcheck`, `strings`, `livekd`, `Dbgview`, `pipelist`, `tcpview`, `PsExec`, `accesschk`, `pslist`, `psloglist`, `pssuspend`, `Testlimit` |
| Performance | `wpr`, `wpa`, `xperf` (Windows Performance Toolkit), 1078 ETW providers |
| Compilers | MSVC 14.51 (`cl`), Windows SDK 10.0.28000 |
| Session control | `PsExec64 -i 1` to launch into the interactive session; `desktops.exe` |
| Other | `WinDbgX`, `vsjitdebugger`, Python 3.14, git, OpenSSH |

**Gap: no command-line debugger.** `cdb`/`kd`/`ntsd` are not installed — only
the Store `WinDbgX` app and the redistributable `dbghelp`/`srcsrv` DLLs. The SDK
installer (`adksetup.exe /features OptionId.WindowsDesktopDebuggers /quiet`)
returned exit 1001, which is an elevation failure. Installing it needs an
elevated shell in the guest; until then, kernel-mode and scripted user-mode
debugging on the oracle is unavailable, and Procmon/WPR/PsExec carry the load.

Also worth noting: SSH lands in **session 0**, which is non-interactive. GUI
programs must be launched into session 1 via PsExec. Metrics measured from
session 0 are classic (unthemed) and must not be compared naively against a real
desktop's values.

## Techniques that earned their place

1. **Same binary, both platforms.** `scripts/run_diff.mjs` cross-compiles once
   with mingw-w64 and runs the identical PE on Windows and under Wine. Removes
   the toolchain as a variable.
2. **Match the application's architecture.** The Creative Cloud bootstrapper is
   PE32 i386, so its probes are built with `i686-w64-mingw32-gcc`. A 64-bit probe
   would have missed the WOW64 registry-view behaviour entirely.
3. **Count in-process rather than trust a tracer.** Counting messages inside the
   probe settled a question `+msg` could not.
4. **Pair `Call`/`Ret` by return address** to attribute a relayed call to the
   application rather than to Wine's own internals — this is what named the
   WebView2 key.
5. **Compare the *API that failed*, not just that something failed.** Windows
   logged `RegQueryValueExW failed`; Wine logged `RegOpenKeyExW failed`. Same
   warning text, different failure point — that difference was the lead.
