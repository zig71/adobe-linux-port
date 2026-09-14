# adobe-wine-lab

Autonomous Windows/Wine compatibility engineering lab.

**Windows is the oracle.** Any candidate divergence is only real once a
standalone Windows executable run on *both* platforms shows different
observable behavior.

## Topology

| Role | Host | Guest | Notes |
|---|---|---|---|
| Behavioral reference | `winnie@192.168.164.18` | Win11-25H2, Windows 11 Home 26200.9445 | MSVC 2026 `cl`, Windows SDK 28000, WPR/WPA, Sysinternals |
| Implementation target | `kubuntu@192.168.166.252` | Kubuntu-26.04, Ubuntu 26.04.1, kernel 7.0.0-31 | Wine source build, GPU-P userspace in `/usr/lib/wsl/lib` |

Both guests share one physical RTX 3080 through Hyper-V GPU-P (identical GPU
UUID `GPU-c125cbbd-…`, driver 595.95, CUDA 13.2, 10240 MiB FB, CC 8.6).

Host-side VMs: `Kubuntu-26.04`, `Win11-25H2`. Host is Hyper-V on
Windows 11 Pro (AMD Ryzen 9 3900XT).

## Directory layout

```
adobe-wine-lab/
  README.md            this file
  AGENTS.md            working rules for automated agents
  inventory/           read-only machine inventory + GPU verification
  scripts/windows/     PowerShell / batch helpers for the oracle
  scripts/linux/       bash helpers for the target
  tests/               standalone cross-platform reproducers (.c / .cpp)
  reproducers/         minimized, promoted reproducers
  runs/<run-id>/       metadata.json + windows/ + linux/ + comparison/
  patches/             upstream-ready Wine/component patches
  notes/progress.md    engineering journal
  src/                 wine/ wine-staging/ vkd3d/ dxvk/ source trees
```

## Access

Interactive `ssh` from the host shell is unreliable (no PTY);
use PuTTY for automation:

```sh
plink -ssh -batch -hostkey "SHA256:ngVcxNV0+L2YFir1FkiQVO2KQY4IuYb8LlAZtYLNKMI" \
      -pw <pw> kubuntu@192.168.166.252 "<cmd>"
plink -ssh -batch -hostkey "SHA256:6ynQLHkmgIaN4ywFOymAF/5gc4gaUlNTeUNgfAbBhzk" \
      -pw <pw> winnie@192.168.164.18 "<cmd>"
```

`pscp.exe` for file transfer with the same `-hostkey`/`-pw` flags.
IPs come from the Hyper-V Default Switch NAT and can change across reboots.

## Loop

```
working Windows behavior
  -> observe equivalent Wine failure
  -> find earliest meaningful behavioral divergence
  -> reduce to a minimal standalone reproducer
  -> prove Windows and Wine differ (return/callback/output/crash)
  -> fix the open-source implementation
  -> add a regression/conformance test
  -> rebuild -> rerun the Adobe workflow -> repeat
```
